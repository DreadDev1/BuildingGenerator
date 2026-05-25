//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommandVoxel.h"

#include "Core/Utils/DungeonNoiseTexture3D.h"
#include "Core/Volumes/DAVoxelVolume.h"
#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommandCore.h"
#include "Frameworks/Voxel/Assets/DAChunkMeshCollection.h"
#include "Frameworks/Voxel/Assets/DAVoxelStaticMeshBaker.h"
#include "Frameworks/Voxel/Chunk/VoxelChunkDataCPU.h"
#include "Frameworks/Voxel/SDFModels/VoxelSDFModel.h"
#include "Frameworks/Voxel/Shaders/VoxelDensityComputeShader.h"
#include "Frameworks/Voxel/Utils/VoxelInfluenceZoneCollector.h"
#include "Frameworks/Voxel/Utils/VoxelLib.h"

#include "Async/ParallelFor.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathSSE.h"
#include "RHIGPUReadback.h"
#include "RHIStaticStates.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "Stats/Stats.h"
#include "SystemTextures.h"

DECLARE_STATS_GROUP(TEXT("DA Voxel"), STATGROUP_DAVoxel, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("DAVoxel Total"), STAT_DAVoxel_Total, STATGROUP_DAVoxel);
DECLARE_CYCLE_STAT(TEXT("DAVoxel SDF Generation"), STAT_DAVoxel_SDFGeneration, STATGROUP_DAVoxel);
DECLARE_CYCLE_STAT(TEXT("DAVoxel Grid Set"), STAT_DAVoxel_GridSet, STATGROUP_DAVoxel);
DECLARE_CYCLE_STAT(TEXT("DAVoxel Noise Texture"), STAT_DAVoxel_NoiseTexture, STATGROUP_DAVoxel);
DECLARE_CYCLE_STAT(TEXT("DAVoxel Coarse Pass"), STAT_DAVoxel_CoarsePass, STATGROUP_DAVoxel);
DECLARE_CYCLE_STAT(TEXT("DAVoxel Fine Pass"), STAT_DAVoxel_FinePass, STATGROUP_DAVoxel);
DECLARE_CYCLE_STAT(TEXT("DAVoxel Shape Init"), STAT_DAVoxel_ShapeInit, STATGROUP_DAVoxel);

DECLARE_STATS_GROUP(TEXT("DA Voxel GPU"), STATGROUP_DAVoxelGPU, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Voxel GPU Dispatch"), STAT_VoxelGPUDispatch, STATGROUP_DAVoxelGPU);
DECLARE_CYCLE_STAT(TEXT("Voxel GPU Readback"), STAT_VoxelGPUReadback, STATGROUP_DAVoxelGPU);


void FDungeonSceneProviderCommand_GenerateVoxelMasterWorld::ExecuteImpl(UWorld* World) {
	if (World) {
        // If baking to a collection asset in the editor, clear existing entries before regenerating
        if (!World->IsGameWorld() && Descriptor.IsValid() && Descriptor->VoxelMeshSettings.BakeCollectionAsset) {
            Descriptor->VoxelMeshSettings.BakeCollectionAsset->StaticMeshes.Empty();
            Descriptor->VoxelMeshSettings.BakeCollectionAsset->MarkPackageDirty();
        }

        TArray<ADAVoxelVolume*> VoxelVolumes;
		{
			TArray<AActor*> VoxelVolumesList;
        	UGameplayStatics::GetAllActorsOfClass(World, ADAVoxelVolume::StaticClass(), VoxelVolumesList);
        	for (AActor* VolumeActor : VoxelVolumesList) {
        		if (ADAVoxelVolume* VoxelVolume = Cast<ADAVoxelVolume>(VolumeActor)) {
        			VoxelVolumes.Add(VoxelVolume);
        		}
        	}
		}

		VoxelVolumes.Sort([](const ADAVoxelVolume& A, const ADAVoxelVolume& B) {
			return A.Priority > B.Priority;
		});

		for (AActor* Actor : VoxelVolumes) {
			if (ADAVoxelVolume* Volume = Cast<ADAVoxelVolume>(Actor)) {
				FDAVoxelShape VolumeShape = Volume->CreateVoxelShape();
				Descriptor->RegisterShapeWithChunks(VolumeShape);
			}
		}

		FDAVoxelInfluenceZoneCollector::CollectInfluenceZones(World, Descriptor->NoiseInfluenceZones);
	}

	// Allow the SDF model to inject preprocessing commands (e.g., Z=0 slice generation for islands)
	if (Descriptor->SDFModel) {
		FDungeonSceneProviderCommandList PreprocessCommands;
		Descriptor->SDFModel->InjectPreprocessingCommands(PreprocessCommands, Descriptor);

		// Add all preprocessing commands with high priority (they execute first)
		for (const TSharedPtr<FDungeonSceneProviderCommand>& PreprocessCommand : PreprocessCommands.GetCommands()) {
			AdditionalEmittedCommands.Add(PreprocessCommand);
		}
		
		Descriptor->SDFModel->Initialize(Context.Dungeon.Get());
	}
	
	for (const auto& ChunkPair : Descriptor->Chunks) {
		const FIntVector& ChunkCoord = ChunkPair.Key;

		TSharedPtr<FDungeonSceneProviderCommand> Command;
		if (Descriptor->VoxelMeshSettings.bUseGPU) {
			Command = MakeShareable(new FDungeonSceneProviderCommand_GenerateVoxelChunkGPU(Context, Descriptor, ChunkCoord));
		} else {
			Command = MakeShareable(new FDungeonSceneProviderCommand_GenerateVoxelChunkCPU(Context, Descriptor, ChunkCoord));
		}

		AdditionalEmittedCommands.Add(Command);
	}
}

void FDungeonSceneProviderCommand_GenerateVoxelChunkBase::UpdateExecutionPriority(const FVector& BuildPosition) {
	const FDAVoxelChunkInfo* ChunkInfo = Descriptor ? Descriptor->Chunks.Find(ChunkCoord) : nullptr;
	if (ChunkInfo) {
		FVector ChunkCenter = ChunkInfo->WorldBounds.GetCenter();
		UpdateExecutionPriorityByDistance(BuildPosition, ChunkCenter);
		ExecutionPriority += 1000000;
	}
}

bool FDungeonSceneProviderCommand_GenerateVoxelChunkBase::CanGenerateGeometry(const TArray<float>& DensityArray) const {
	if (DensityArray.Num() > 0) {
		bool bFirstSign = DensityArray[0] > 0;
			
		for (int32 i = 0; i < DensityArray.Num(); i++) {
			bool bCurrentSign = DensityArray[i] > 0;
			if (bCurrentSign != bFirstSign) {
				return true;
			}
		}
			
		bool bLastSign = DensityArray.Last() > 0;
		if (bLastSign != bFirstSign) {
			return true;
		}
	}
	return false;
}

void FDungeonSceneProviderCommand_GenerateVoxelChunkBase::GenerateMesh(UWorld* InWorld, const FDAVoxelChunkDataCPU& ChunkData) {
	const int32 ChunkVoxelSize = Descriptor->VoxelMeshSettings.VoxelChunkSize;
	
	FIntVector CoordMin = FIntVector(
		ChunkCoord.X * ChunkVoxelSize,
		ChunkCoord.Y * ChunkVoxelSize,
		ChunkCoord.Z * ChunkVoxelSize
	);
	FIntVector CoordMax = CoordMin + FIntVector(ChunkVoxelSize - 1);
	
	TSharedPtr<DA::FDungeonMeshGeometry> MeshGeometry = MakeShared<DA::FDungeonMeshGeometry>();
	DA::MarchingCubes::FSettings MeshSettings;
	MeshSettings.VoxelSize = Descriptor->VoxelMeshSettings.VoxelSize;
	MeshSettings.LODLevel = 0;
	MeshSettings.UVScale = Descriptor->VoxelMeshSettings.UVScale;
	
	const FIntVector MinBounds = ChunkData.VoxelMin;
	const FIntVector MaxBounds = ChunkData.VoxelMax;
	
	auto Getter = [&ChunkData, MinBounds, MaxBounds](const FIntVector& Coord, DA::VDB::FVoxelData& OutData) -> bool {
		if (Coord.X < MinBounds.X || Coord.X > MaxBounds.X ||
			Coord.Y < MinBounds.Y || Coord.Y > MaxBounds.Y ||
			Coord.Z < MinBounds.Z || Coord.Z > MaxBounds.Z) {
			return false;
			}
		
		OutData.SDF = ChunkData.GetDensity(Coord.X, Coord.Y, Coord.Z);
		OutData.Material = 0;
		return true;
	};

	TMap<uint8, DA::FDungeonMeshGeometry> MeshSections;
	DA::FDungeonMeshGeometry& MeshData = MeshSections.FindOrAdd(0);
	DA::MarchingCubes::FMeshBuilder::GenerateMeshBlock(MeshSettings, CoordMin, CoordMax, Getter, MeshData);
	
#if WITH_EDITOR
		const bool bDoStaticBake = (InWorld && !InWorld->IsGameWorld() && Descriptor->VoxelMeshSettings.BakeCollectionAsset);
#else
		const bool bDoStaticBake = false;
#endif

		if (MeshSections.Num() > 0) {
			FVector ChunkBasePosition = FVector(ChunkCoord) * (ChunkVoxelSize * Descriptor->VoxelMeshSettings.VoxelSize);

			if (bDoStaticBake) {
				for (auto& Section : MeshSections) {
					uint8 MaterialID = Section.Key;
					DA::FDungeonMeshGeometry& SectionGeometry = Section.Value;

					if (SectionGeometry.Triangles.Num() == 0) {
						continue;
					}

					for (auto& Vertex : SectionGeometry.Vertices) {
						Vertex.Position -= ChunkBasePosition;
					}

					TSharedPtr<DA::FDungeonMeshGeometry> SectionMeshPtr = MakeShared<DA::FDungeonMeshGeometry>(MoveTemp(SectionGeometry));

					Context.Transform = FTransform(ChunkBasePosition);

					UMaterialInterface* SectionMaterial = Descriptor->VoxelMeshSettings.Material;

					bool bDidStaticBake = false;
#if WITH_EDITOR
                    if (UStaticMesh* Baked = DA::Voxel::BakeStaticMeshForChunk(
                        Descriptor->VoxelMeshSettings.BakeCollectionAsset,
                        ChunkCoord,
                        *SectionMeshPtr,
                        SectionMaterial,
                        Context.Transform.GetLocation(),
                        Descriptor->VoxelMeshSettings.UVScale)) {
						FActorSpawnParameters Params;
						Params.OverrideLevel = Context.LevelOverride.Get();
						Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
						AStaticMeshActor* SMA = InWorld->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Context.Transform, Params);
						if (SMA) {
							SMA->Tags.Add(DA::FVoxelConstants::VoxelChunkActorTag);
							UStaticMeshComponent* SMC = SMA->GetStaticMeshComponent();
							SMC->SetStaticMesh(Baked);
							if (SectionMaterial) {
								SMC->SetMaterial(0, SectionMaterial);
							}
							PostInitializeActor(SMA);
							bDidStaticBake = true;
						}
					}
#endif // WITH_EDITOR

					if (!bDidStaticBake) {
						FDungeonSceneProviderCommand_AddMeshGeometry::FSettings MeshCommandSettings;
						MeshCommandSettings.Geometry = SectionMeshPtr;
						MeshCommandSettings.Material = SectionMaterial;
						MeshCommandSettings.bEnableCollision = Descriptor->VoxelMeshSettings.bEnableCollision;
						MeshCommandSettings.UVScale = Descriptor->VoxelMeshSettings.UVScale;
						MeshCommandSettings.ActorTags.Add(DA::FVoxelConstants::VoxelChunkActorTag);

						FDungeonSceneProviderCommand_AddMeshGeometry AddMeshCommand(Context, MeshCommandSettings);
						AddMeshCommand.Execute(InWorld);
					}
				}
			}
			else {
				// Dynamic path: only emit section 0
				DA::FDungeonMeshGeometry* Section0 = MeshSections.Find(0);
				DA::FDungeonMeshGeometry* Chosen = Section0;
				if (!Chosen) {
					for (auto& Pair : MeshSections) { Chosen = &Pair.Value; break; }
				}
				if (Chosen && Chosen->Triangles.Num() > 0) {
					for (auto& Vertex : Chosen->Vertices) {
						Vertex.Position -= ChunkBasePosition;
					}
					TSharedPtr<DA::FDungeonMeshGeometry> SectionMeshPtr = MakeShared<DA::FDungeonMeshGeometry>(MoveTemp(*Chosen));
					Context.Transform = FTransform(ChunkBasePosition);
					UMaterialInterface* SectionMaterial = Descriptor->VoxelMeshSettings.Material;

					FDungeonSceneProviderCommand_AddMeshGeometry::FSettings MeshCommandSettings;
					MeshCommandSettings.Geometry = SectionMeshPtr;
					MeshCommandSettings.Material = SectionMaterial;
					MeshCommandSettings.bEnableCollision = Descriptor->VoxelMeshSettings.bEnableCollision;
					MeshCommandSettings.UVScale = Descriptor->VoxelMeshSettings.UVScale;
					MeshCommandSettings.ActorTags.Add(DA::FVoxelConstants::VoxelChunkActorTag);

					FDungeonSceneProviderCommand_AddMeshGeometry AddMeshCommand(Context, MeshCommandSettings);
					AddMeshCommand.Execute(InWorld);
				}
			}
		}
}

void FDungeonSceneProviderCommand_GenerateVoxelChunkCPU::ExecuteImpl(UWorld* World) {
	SCOPE_CYCLE_COUNTER(STAT_DAVoxel_Total);
	
	if (!Descriptor || !World) {
		return;
	}
	
	const FDAVoxelChunkInfo* ChunkInfo = Descriptor->Chunks.Find(ChunkCoord);
	if (!ChunkInfo || ChunkInfo->ShapeIndices.Num() == 0) {
		return;
	}

	FDAVoxelChunkDataCPU ChunkData;
	GenerateDensity(*ChunkInfo, ChunkData);
	
	bool bHasSignChange = CanGenerateGeometry(ChunkData.DensityData);
	if (!bHasSignChange) {
		return;
	}

	GenerateMesh(World, ChunkData);
}

float FDungeonSceneProviderCommand_GenerateVoxelChunkCPU::CalculateNoise(const FVector& WorldPos) const {
	if (Descriptor->NoiseSettings.NoiseAmplitude <= 0) {
		return 0.0f;
	}
	
	float Noise = 0;
	float Amplitude = Descriptor->NoiseSettings.NoiseAmplitude;
	float Scale = Descriptor->NoiseSettings.GetNoiseScale(false);
	
	for (int32 Octave = 0; Octave < Descriptor->NoiseSettings.NoiseOctaves; Octave++) {
		const FVector NoisePos = (WorldPos + Descriptor->NoiseSettings.NoiseOffset) / Scale;
		Noise += FMath::PerlinNoise3D(NoisePos) * Amplitude;
		
		Amplitude *= 0.5f;
		Scale *= 0.5f;
	}
	
	return Noise;
}


void FDungeonSceneProviderCommand_GenerateVoxelChunkCPU::GenerateDensity(const FDAVoxelChunkInfo& ChunkInfo, FDAVoxelChunkDataCPU& ChunkData) const {
	const int32 ChunkVoxelSize = Descriptor->VoxelMeshSettings.VoxelChunkSize;
	const float VoxelSize = Descriptor->VoxelMeshSettings.VoxelSize;
	
	const int32 EdgePadding = 1;
	const int32 TotalPadding = EdgePadding;
	
	const FIntVector VoxelMin = FIntVector(
		ChunkCoord.X * ChunkVoxelSize - TotalPadding,
		ChunkCoord.Y * ChunkVoxelSize - TotalPadding,
		ChunkCoord.Z * ChunkVoxelSize - TotalPadding
	);
	const FIntVector VoxelMax = FIntVector(
		(ChunkCoord.X + 1) * ChunkVoxelSize + TotalPadding,
		(ChunkCoord.Y + 1) * ChunkVoxelSize + TotalPadding,
		(ChunkCoord.Z + 1) * ChunkVoxelSize + TotalPadding
	);

	ChunkData.Initialize(ChunkCoord, VoxelMin, VoxelMax);
	
	TArray<FDAVoxelShape> Shapes;
	for (int32 ShapeIndex : ChunkInfo.ShapeIndices) {
		if (ShapeIndex >= 0 && ShapeIndex < Descriptor->Shapes.Num()) {
			Shapes.Add(Descriptor->Shapes[ShapeIndex]);
		}
	}
	
	FDungeonNoiseTexture3D& NoiseTexture = FDungeonNoiseTexture3D::Get();
	{
		SCOPE_CYCLE_COUNTER(STAT_DAVoxel_NoiseTexture);
		NoiseTexture.Initialize(128); 
	}
	
	const int32 SizeX = VoxelMax.X - VoxelMin.X + 1;
	const int32 SizeY = VoxelMax.Y - VoxelMin.Y + 1;
	const int32 SizeZ = VoxelMax.Z - VoxelMin.Z + 1;
		
	const float WallThickness = Descriptor->VoxelMeshSettings.WallThickness;
	
	float MinFeatureSize = WallThickness;
	for (const FDAVoxelShape& Shape : Shapes) {
		if (Shape.Type == EDAVoxelShapeType::Wall) {
			MinFeatureSize = FMath::Min(MinFeatureSize, Shape.Thickness);
		}
	}
	
	const float MinFeatureVoxelSpan = MinFeatureSize / VoxelSize;
	const float SafetyMargin = 0.75f;
	const float MaxSampleDistance = MinFeatureSize * SafetyMargin;
	
	int32 SkipSize = FMath::FloorToInt(MaxSampleDistance / VoxelSize);
	SkipSize = FMath::Clamp(SkipSize, 2, 4);
	
	const int32 CoarseSizeX = (SizeX + SkipSize - 1) / SkipSize;
	const int32 CoarseSizeY = (SizeY + SkipSize - 1) / SkipSize;
	const int32 CoarseSizeZ = (SizeZ + SkipSize - 1) / SkipSize;
	
	TArray<float> CoarseGrid;
	CoarseGrid.SetNum(CoarseSizeX * CoarseSizeY * CoarseSizeZ);

	FBox ChunkWorldBounds = Descriptor->GetChunkWorldBounds(ChunkInfo.ChunkCoord);
	ChunkWorldBounds = ChunkWorldBounds.ExpandBy(Descriptor->BoundsExpansion * 0.1f);

	FDAVoxelNoiseSettings& GlobalNoiseSettings = Descriptor->NoiseSettings;
	
	{
		SCOPE_CYCLE_COUNTER(STAT_DAVoxel_CoarsePass);
		
		ParallelFor(CoarseGrid.Num(), [&](int32 Index) {
			const int32 CZ = Index / (CoarseSizeX * CoarseSizeY);
			const int32 Remainder = Index % (CoarseSizeX * CoarseSizeY);
			const int32 CY = Remainder / CoarseSizeX;
			const int32 CX = Remainder % CoarseSizeX;
			
			const int32 WorldX = VoxelMin.X + CX * SkipSize;
			const int32 WorldY = VoxelMin.Y + CY * SkipSize;
			const int32 WorldZ = VoxelMin.Z + CZ * SkipSize;

			const FIntVector VoxelCoord(WorldX, WorldY, WorldZ);
			const FVector VoxelWorldPos(WorldX * VoxelSize, WorldY * VoxelSize, WorldZ * VoxelSize);
			
			uint8 MaterialID = 0;
			float BaseSDF = DA::FVoxelLib::CalculateBaseSDF(VoxelWorldPos, Shapes, VoxelSize, GlobalNoiseSettings,
				Descriptor->NoiseInfluenceZones, Descriptor->SDFModel.Get(), Descriptor.Get(), ChunkCoord, ChunkWorldBounds, VoxelCoord, MaterialID);

			CoarseGrid[Index] = BaseSDF;
		});
	}
	
	auto IsNearSurface = [&](int32 X, int32 Y, int32 Z) -> bool {
		const int32 CX = X / SkipSize;
		const int32 CY = Y / SkipSize;
		const int32 CZ = Z / SkipSize;
		
		const float SurfaceThreshold = VoxelSize * 2.0f;
		
		for (int32 dz = FMath::Max(0, CZ - 1); dz <= FMath::Min(CoarseSizeZ - 1, CZ + 1); dz++) {
			for (int32 dy = FMath::Max(0, CY - 1); dy <= FMath::Min(CoarseSizeY - 1, CY + 1); dy++) {
				for (int32 dx = FMath::Max(0, CX - 1); dx <= FMath::Min(CoarseSizeX - 1, CX + 1); dx++) {
					const int32 CoarseIndex = dx + dy * CoarseSizeX + dz * CoarseSizeX * CoarseSizeY;
					const float CoarseSDF = CoarseGrid[CoarseIndex];
					
					if (FMath::Abs(CoarseSDF) < SurfaceThreshold) {
						return true;
					}
					
					if (dx > CX && dy == CY && dz == CZ) {
						const int32 PrevIndex = (dx - 1) + dy * CoarseSizeX + dz * CoarseSizeX * CoarseSizeY;
						if (CoarseSDF * CoarseGrid[PrevIndex] < 0) return true;
					}
					if (dy > CY && dx == CX && dz == CZ) {
						const int32 PrevIndex = dx + (dy - 1) * CoarseSizeX + dz * CoarseSizeX * CoarseSizeY;
						if (CoarseSDF * CoarseGrid[PrevIndex] < 0) return true;
					}
					if (dz > CZ && dx == CX && dy == CY) {
						const int32 PrevIndex = dx + dy * CoarseSizeX + (dz - 1) * CoarseSizeX * CoarseSizeY;
						if (CoarseSDF * CoarseGrid[PrevIndex] < 0) return true;
					}
				}
			}
		}
		
		const int32 CoarseIndex = CX + CY * CoarseSizeX + CZ * CoarseSizeX * CoarseSizeY;
		const float CoarseValue = CoarseGrid[CoarseIndex];
		
		return FMath::Abs(CoarseValue) < SurfaceThreshold * 3.0f;
	};
	
	{
		SCOPE_CYCLE_COUNTER(STAT_DAVoxel_FinePass);
	
		const int32 BlockSize = 8;
		const int32 BlocksX = (SizeX + BlockSize - 1) / BlockSize;
		const int32 BlocksY = (SizeY + BlockSize - 1) / BlockSize;
		const int32 BlocksZ = (SizeZ + BlockSize - 1) / BlockSize;
		const int32 TotalBlocks = BlocksX * BlocksY * BlocksZ;
		
		ParallelFor(TotalBlocks, [&](int32 BlockIndex) {
			const int32 BlockZ = BlockIndex / (BlocksX * BlocksY);
			const int32 BlockRemainder = BlockIndex % (BlocksX * BlocksY);
			const int32 BlockY = BlockRemainder / BlocksX;
			const int32 BlockX = BlockRemainder % BlocksX;
			
			const int32 StartX = BlockX * BlockSize;
			const int32 StartY = BlockY * BlockSize;
			const int32 StartZ = BlockZ * BlockSize;
			const int32 EndX = FMath::Min(StartX + BlockSize, SizeX);
			const int32 EndY = FMath::Min(StartY + BlockSize, SizeY);
			const int32 EndZ = FMath::Min(StartZ + BlockSize, SizeZ);
			
			const int32 VoxelsPerBatch = 4;
			
			for (int32 z = StartZ; z < EndZ; z++) {
				for (int32 y = StartY; y < EndY; y++) {
					for (int32 x = StartX; x < EndX; x += VoxelsPerBatch) {
						float WorldXArray[4];
						float WorldYArray[4];
						float WorldZArray[4];
						float SDFArray[4];
						bool NearSurfaceArray[4];
						int32 ValidCount = 0;
						
						for (int32 i = 0; i < VoxelsPerBatch && (x + i) < EndX; i++) {
							WorldXArray[i] = (float)(VoxelMin.X + x + i);
							WorldYArray[i] = (float)(VoxelMin.Y + y);
							WorldZArray[i] = (float)(VoxelMin.Z + z);
							NearSurfaceArray[i] = IsNearSurface(x + i, y, z);
							ValidCount++;
						}
						
						for (int32 i = ValidCount; i < VoxelsPerBatch; i++) {
							WorldXArray[i] = WorldXArray[0];
							WorldYArray[i] = WorldYArray[0];
							WorldZArray[i] = WorldZArray[0];
							NearSurfaceArray[i] = false;
						}
						
						bool bAllFarFromSurface = true;
						for (int32 i = 0; i < ValidCount; i++) {
							if (NearSurfaceArray[i]) {
								bAllFarFromSurface = false;
								break;
							}
						}
						
						if (bAllFarFromSurface && ValidCount == 4) {
							VectorRegister VecX = VectorLoad(WorldXArray);
							VectorRegister VecY = VectorLoad(WorldYArray);
							VectorRegister VecZ = VectorLoad(WorldZArray);
							
							alignas(16) float CoarseXArray[4];
							alignas(16) float CoarseYArray[4];
							alignas(16) float CoarseZArray[4];
							
							for (int32 i = 0; i < 4; i++) {
								CoarseXArray[i] = (float)((x + i) / SkipSize);
								CoarseYArray[i] = (float)(y / SkipSize);
								CoarseZArray[i] = (float)(z / SkipSize);
							}
							
							for (int32 i = 0; i < 4; i++) {
								const int32 CX = (int32)CoarseXArray[i];
								const int32 CY = (int32)CoarseYArray[i];
								const int32 CZ = (int32)CoarseZArray[i];
								const int32 CoarseIndex = CX + CY * CoarseSizeX + CZ * CoarseSizeX * CoarseSizeY;
								SDFArray[i] = CoarseGrid[CoarseIndex];
							}
							
							for (int32 i = 0; i < ValidCount; i++) {
								const int32 WorldX = VoxelMin.X + x + i;
								const int32 WorldY = VoxelMin.Y + y;
								const int32 WorldZ = VoxelMin.Z + z;
								ChunkData.SetDensity(WorldX, WorldY, WorldZ, SDFArray[i]);
								ChunkData.SetMaterial(WorldX, WorldY, WorldZ, 0);
							}
						} else {
							for (int32 i = 0; i < ValidCount; i++) {
								const int32 WorldX = VoxelMin.X + x + i;
								const int32 WorldY = VoxelMin.Y + y;
								const int32 WorldZ = VoxelMin.Z + z;
								
								float FinalSDF;
								uint8 MaterialID = 0;
								
								if (NearSurfaceArray[i]) {
									const FIntVector VoxelCoord(WorldX, WorldY, WorldZ);
									const FVector VoxelWorldPos(WorldX * VoxelSize, WorldY * VoxelSize, WorldZ * VoxelSize);
									
									float BaseSDF = DA::FVoxelLib::CalculateBaseSDF(VoxelWorldPos, Shapes, VoxelSize, GlobalNoiseSettings,
										Descriptor->NoiseInfluenceZones, Descriptor->SDFModel.Get(), Descriptor.Get(), ChunkCoord, ChunkWorldBounds, VoxelCoord, MaterialID);
									FinalSDF = BaseSDF;
								} else {
									const int32 CX = (x + i) / SkipSize;
									const int32 CY = y / SkipSize;
									const int32 CZ = z / SkipSize;
									const int32 CoarseIndex = CX + CY * CoarseSizeX + CZ * CoarseSizeX * CoarseSizeY;
									FinalSDF = CoarseGrid[CoarseIndex];
								}
								
								ChunkData.SetDensity(WorldX, WorldY, WorldZ, FinalSDF);
								ChunkData.SetMaterial(WorldX, WorldY, WorldZ, MaterialID);
							}
						}
					}
				}
			}
		});
	}
}

void FDungeonSceneProviderCommand_GenerateVoxelChunkCPU::GenerateDensity_SimpleSlow(const FDAVoxelChunkInfo& ChunkInfo, FDAVoxelChunkDataCPU& ChunkData) const {
	const int32 ChunkVoxelSize = Descriptor->VoxelMeshSettings.VoxelChunkSize;
	const float VoxelSize = Descriptor->VoxelMeshSettings.VoxelSize;
	
	const int32 EdgePadding = 1;
	const int32 TotalPadding = EdgePadding;
	
	const FIntVector VoxelMin = FIntVector(
		ChunkCoord.X * ChunkVoxelSize - TotalPadding,
		ChunkCoord.Y * ChunkVoxelSize - TotalPadding,
		ChunkCoord.Z * ChunkVoxelSize - TotalPadding
	);
	const FIntVector VoxelMax = FIntVector(
		(ChunkCoord.X + 1) * ChunkVoxelSize + TotalPadding,
		(ChunkCoord.Y + 1) * ChunkVoxelSize + TotalPadding,
		(ChunkCoord.Z + 1) * ChunkVoxelSize + TotalPadding
	);
	
	ChunkData.Initialize(ChunkCoord, VoxelMin, VoxelMax);
	
	TArray<FDAVoxelShape> Shapes;
	for (int32 ShapeIndex : ChunkInfo.ShapeIndices) {
		if (ShapeIndex >= 0 && ShapeIndex < Descriptor->Shapes.Num()) {
			Shapes.Add(Descriptor->Shapes[ShapeIndex]);
		}
	}
	
	FDAVoxelNoiseSettings& GlobalNoiseSettings = Descriptor->NoiseSettings;
	FBox ChunkWorldBounds = Descriptor->GetChunkWorldBounds(ChunkInfo.ChunkCoord);
	ChunkWorldBounds = ChunkWorldBounds.ExpandBy(Descriptor->BoundsExpansion);

	const int32 SizeX = VoxelMax.X - VoxelMin.X + 1;
	const int32 SizeY = VoxelMax.Y - VoxelMin.Y + 1;
	const int32 SizeZ = VoxelMax.Z - VoxelMin.Z + 1;
	const int32 TotalVoxels = SizeX * SizeY * SizeZ;
	
	for (int Index = 0; Index < TotalVoxels; Index++) {
		const int32 Z = Index / (SizeX * SizeY);
		const int32 Remainder = Index % (SizeX * SizeY);
		const int32 Y = Remainder / SizeX;
		const int32 X = Remainder % SizeX;
		
		const int32 WorldX = VoxelMin.X + X;
		const int32 WorldY = VoxelMin.Y + Y;
		const int32 WorldZ = VoxelMin.Z + Z;
		
		const FIntVector VoxelCoord(WorldX, WorldY, WorldZ);
		const FVector VoxelWorldPos(WorldX * VoxelSize, WorldY * VoxelSize, WorldZ * VoxelSize);
		
		uint8 MaterialID = 0;
		
		float BaseSDF = DA::FVoxelLib::CalculateBaseSDF(VoxelWorldPos, Shapes, VoxelSize, GlobalNoiseSettings,
			Descriptor->NoiseInfluenceZones, Descriptor->SDFModel.Get(), Descriptor.Get(), ChunkCoord, ChunkWorldBounds, VoxelCoord, MaterialID);

		ChunkData.SetDensity(WorldX, WorldY, WorldZ, BaseSDF);
		ChunkData.SetMaterial(WorldX, WorldY, WorldZ, MaterialID);
	}
}


FDungeonSceneProviderCommand_GenerateVoxelChunkGPU::FDungeonSceneProviderCommand_GenerateVoxelChunkGPU(
	const FDungeonSceneProviderContext& InContext,
	const TSharedPtr<FDAVoxelChunkDescriptor>& InDescriptor,
	const FIntVector& InChunkCoord)
	: FDungeonSceneProviderCommand_GenerateVoxelChunkBase(InContext, InDescriptor, InChunkCoord)
{
	Hints.bIsLatent = true;
}

FDungeonSceneProviderCommand_GenerateVoxelChunkGPU::~FDungeonSceneProviderCommand_GenerateVoxelChunkGPU() {
	ReleaseGPUResources();
}

void FDungeonSceneProviderCommand_GenerateVoxelChunkGPU::ExecuteImpl(UWorld* World) {
	PrepareShapeData();
	DispatchComputeShader();
	GPUState = EGPUState::ComputeDispatched;
	bIsFinished = false;
}

void FDungeonSceneProviderCommand_GenerateVoxelChunkGPU::Tick(UWorld* World, float DeltaTime) {
	switch (GPUState) {
		case EGPUState::ComputeDispatched:
			InitiateReadback();
			GPUState = EGPUState::ReadbackInitiated;
			break;
			
		case EGPUState::ReadbackInitiated:
			GPUState = EGPUState::ReadbackPending;
			break;
			
		case EGPUState::ReadbackPending:
			if (IsReadbackReady()) {
				ProcessReadbackData();
				GPUState = EGPUState::DataProcessing;
			}
			break;
			
		case EGPUState::DataProcessing:
			if (IsDataProcessed()) {
				GPUState = EGPUState::MeshGenerating;
			}
			break;
			
		case EGPUState::MeshGenerating:
			ProcessPhaseGenerateMesh(World);
			ReleaseGPUResources();
			GPUState = EGPUState::Completed;
			bIsFinished = true;
			break;
			
		default:
			break;
	}
}

void FDungeonSceneProviderCommand_GenerateVoxelChunkGPU::PrepareShapeData() {
	if (!Descriptor) return;
	
	const FDAVoxelChunkInfo* ChunkInfo = Descriptor->Chunks.Find(ChunkCoord);
	if (!ChunkInfo) return;
	
	PrimaryCarveShapes.Empty();
	AddGeometryShapes.Empty();
	
	for (int32 ShapeIndex : ChunkInfo->ShapeIndices) {
		if (Descriptor->Shapes.IsValidIndex(ShapeIndex)) {
			const FDAVoxelShape& CPUShape = Descriptor->Shapes[ShapeIndex];
			FDAVoxelShapeGPU GPUShape = FDAVoxelShapeGPU::ConvertToGPU(CPUShape);
			
			switch (CPUShape.Layer) {
				case EVoxelShapeLayer::PrimaryCarve:
					PrimaryCarveShapes.Add(GPUShape);
					break;
				case EVoxelShapeLayer::AddGeometry:
					AddGeometryShapes.Add(GPUShape);
					break;
			}
		}
	}
}

void FDungeonSceneProviderCommand_GenerateVoxelChunkGPU::DispatchComputeShader() {
	SCOPE_CYCLE_COUNTER(STAT_VoxelGPUDispatch);
	
	if (!Descriptor) return;
	
	const FDAVoxelChunkInfo* ChunkInfo = Descriptor->Chunks.Find(ChunkCoord);
	if (!ChunkInfo || ChunkInfo->ShapeIndices.Num() == 0) return;
	
	const int32 ChunkVoxelSize = Descriptor->VoxelMeshSettings.VoxelChunkSize;
	const int32 EdgePadding = 1;
	const int32 TotalPadding = EdgePadding;
	const int32 TotalSize = ChunkVoxelSize + TotalPadding * 2;
	
	const FIntVector VoxelMin(
		ChunkCoord.X * ChunkVoxelSize - TotalPadding,
		ChunkCoord.Y * ChunkVoxelSize - TotalPadding,
		ChunkCoord.Z * ChunkVoxelSize - TotalPadding
	);
	const FIntVector VoxelMax(
		(ChunkCoord.X + 1) * ChunkVoxelSize + TotalPadding,
		(ChunkCoord.Y + 1) * ChunkVoxelSize + TotalPadding,
		(ChunkCoord.Z + 1) * ChunkVoxelSize + TotalPadding
	);
	
	ChunkData = MakeShared<FDAVoxelChunkDataCPU>();
	ChunkData->Initialize(ChunkCoord, VoxelMin, VoxelMax);
	
	TArray<FDAVoxelShapeGPU> AllShapes;
	AllShapes.Append(PrimaryCarveShapes);
	AllShapes.Append(AddGeometryShapes);
	
	TArray<FNoiseInfluenceZoneGPU> InfluenceZonesGPU;
	for (const FDANoiseInfluenceZone& Zone : Descriptor->NoiseInfluenceZones) {
		if (Zone.bEnabled) {
			InfluenceZonesGPU.Add(FNoiseInfluenceZoneGPU::ConvertToGPU(Zone));
		}
	}
	
	ENQUEUE_RENDER_COMMAND(VoxelDensityCompute)(
		[this, AllShapes, InfluenceZonesGPU, VoxelMin, VoxelMax, TotalSize](FRHICommandListImmediate& RHICmdList) {
			FRDGBuilder GraphBuilder(RHICmdList);
			
			const FIntVector ChunkSize = VoxelMax - VoxelMin + FIntVector(1, 1, 1);
			const uint32 TotalVoxels = ChunkSize.X * ChunkSize.Y * ChunkSize.Z;
			
			// Use pooled buffers for better memory management
			FRDGBufferDesc DensityDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(float), TotalVoxels);
			DensityDesc.Usage = EBufferUsageFlags::UnorderedAccess | EBufferUsageFlags::SourceCopy;
			FRDGBufferRef DensityBufferRDG = GraphBuilder.CreateBuffer(
				DensityDesc,
				TEXT("VoxelDensityBuffer"),
				ERDGBufferFlags::None
			);

			FRDGBufferDesc MaterialDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(uint32), TotalVoxels);
			MaterialDesc.Usage = EBufferUsageFlags::UnorderedAccess | EBufferUsageFlags::SourceCopy;
			FRDGBufferRef MaterialBufferRDG = GraphBuilder.CreateBuffer(
				MaterialDesc,
				TEXT("VoxelMaterialBuffer"),
				ERDGBufferFlags::None
			);
			
			FRDGBufferUAVRef DensityUAV = GraphBuilder.CreateUAV(DensityBufferRDG, PF_R32_FLOAT);
			FRDGBufferUAVRef MaterialUAV = GraphBuilder.CreateUAV(MaterialBufferRDG, PF_R32_UINT);
			
			AddClearUAVFloatPass(GraphBuilder, DensityUAV, 1.0f);
			AddClearUAVPass(GraphBuilder, MaterialUAV, 0u);
			
			FRDGBufferRef ShapeBufferRDG;
			if (AllShapes.Num() == 0) {
				FDAVoxelShapeGPU DummyShape;
				FMemory::Memzero(&DummyShape, sizeof(FDAVoxelShapeGPU));
				ShapeBufferRDG = CreateStructuredBuffer(
					GraphBuilder,
					TEXT("VoxelShapes"),
					TConstArrayView<FDAVoxelShapeGPU>(&DummyShape, 1)
				);
			} else {
				ShapeBufferRDG = CreateStructuredBuffer(GraphBuilder, TEXT("VoxelShapes"), AllShapes);
			}
			
			FRDGBufferRef InfluenceZoneBufferRDG;
			if (InfluenceZonesGPU.Num() == 0) {
				FNoiseInfluenceZoneGPU DummyZone;
				FMemory::Memzero(&DummyZone, sizeof(FNoiseInfluenceZoneGPU));
				InfluenceZoneBufferRDG = CreateStructuredBuffer(
					GraphBuilder,
					TEXT("InfluenceZones"),
					TConstArrayView<FNoiseInfluenceZoneGPU>(&DummyZone, 1)
				);
			} else {
				InfluenceZoneBufferRDG = CreateStructuredBuffer(GraphBuilder, TEXT("InfluenceZones"), InfluenceZonesGPU);
			}
			
			FRDGTextureRef NoiseTextureRDG = nullptr;
			if (GSystemTextures.PerlinNoise3D.GetReference())
			{
				NoiseTextureRDG = GraphBuilder.RegisterExternalTexture(
					CreateRenderTarget(GSystemTextures.PerlinNoise3D->GetRHI(), TEXT("PerlinNoise3D"))
				);
			}
			else
			{
				FRDGTextureDesc NoiseTextureDesc = FRDGTextureDesc::Create3D(
					FIntVector(16, 16, 16),
					PF_A32B32G32R32F,
					FClearValueBinding::Black,
					TexCreate_ShaderResource
				);
				NoiseTextureRDG = GraphBuilder.CreateTexture(NoiseTextureDesc, TEXT("DummyNoiseTexture"));
			}
			
			TShaderMapRef<FVoxelDensityComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
			
			FIntVector DispatchSize(
				FMath::DivideAndRoundUp(ChunkSize.X, 8),
				FMath::DivideAndRoundUp(ChunkSize.Y, 8),
				FMath::DivideAndRoundUp(ChunkSize.Z, 8)
			);
			
			auto* Parameters = GraphBuilder.AllocParameters<FVoxelDensityComputeShader::FParameters>();
			
			Parameters->ChunkCoord = ChunkCoord;
			Parameters->VoxelMin = VoxelMin;
			Parameters->VoxelMax = VoxelMax;
			Parameters->VoxelSize = Descriptor->VoxelMeshSettings.VoxelSize;
			// Get world-specific vertical offsets for GPU shader parameters
			FDungeonVoxelVerticalOffsets VerticalOffsets;
			if (Descriptor->SDFModel) {
				VerticalOffsets = Descriptor->SDFModel->GetVerticalOffsets();
			}
			Parameters->CeilingHeightOffset = VerticalOffsets.TopOffset;
			Parameters->WallThickness = Descriptor->VoxelMeshSettings.WallThickness;
			
			Parameters->NoiseOffset = FVector3f(Descriptor->NoiseSettings.NoiseOffset);
			Parameters->NoiseAmplitude = Descriptor->NoiseSettings.NoiseAmplitude;
			Parameters->NoiseScale = Descriptor->NoiseSettings.GetNoiseScale(true);
			Parameters->NoiseChannelSeparation = Descriptor->NoiseSettings.NoiseChannelSeparation;
			Parameters->FloorCeilingTransitionHeight = Descriptor->NoiseSettings.FloorCeilingTransitionHeight;
			Parameters->NoiseFloorScale = Descriptor->NoiseSettings.NoiseFloorScale;
			Parameters->NoiseCeilingScale = Descriptor->NoiseSettings.NoiseCeilingScale;
			Parameters->NoiseOctaves = Descriptor->NoiseSettings.NoiseOctaves;
			
			Parameters->NoiseScaleVector = FVector3f(Descriptor->NoiseSettings.NoiseScaleVector);
			
			Parameters->bEnableDomainWarp = Descriptor->NoiseSettings.bEnableDomainWarp ? 1 : 0;
			Parameters->DomainWarpStrength = Descriptor->NoiseSettings.DomainWarpStrength;

			if (IsValid(Descriptor->SDFModel)) {
				Descriptor->SDFModel->WriteShaderParameters(Parameters);
			}

			Parameters->NoiseTexture = NoiseTextureRDG;
			Parameters->NoiseTextureSampler = TStaticSamplerState<SF_Trilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();
			
			Parameters->NumShapes = AllShapes.Num();
			Parameters->ShapeBuffer = GraphBuilder.CreateSRV(ShapeBufferRDG);
			
			Parameters->NumInfluenceZones = InfluenceZonesGPU.Num();
			Parameters->InfluenceZoneBuffer = GraphBuilder.CreateSRV(InfluenceZoneBufferRDG);
			
			Parameters->DensityBuffer = DensityUAV;
			Parameters->MaterialBuffer = MaterialUAV;
			
			FComputeShaderUtils::AddPass(
				GraphBuilder,
				RDG_EVENT_NAME("VoxelDensity_SinglePass"),
				ComputeShader,
				Parameters,
				DispatchSize
			);

			GraphBuilder.QueueBufferExtraction(DensityBufferRDG, &DensityBuffer);
			GraphBuilder.QueueBufferExtraction(MaterialBufferRDG, &MaterialBuffer);
			
			GraphBuilder.Execute();
		}
	);
}

void FDungeonSceneProviderCommand_GenerateVoxelChunkGPU::InitiateReadback() {
	ENQUEUE_RENDER_COMMAND(VoxelInitiateReadback)(
		[this](FRHICommandListImmediate& RHICmdList) {
			if (DensityBuffer) {
				DensityReadback = new FRHIGPUBufferReadback(TEXT("VoxelDensityReadback"));
				DensityReadback->EnqueueCopy(RHICmdList, DensityBuffer->GetRHI());
			}
			
			if (MaterialBuffer) {
				MaterialReadback = new FRHIGPUBufferReadback(TEXT("VoxelMaterialReadback"));
				MaterialReadback->EnqueueCopy(RHICmdList, MaterialBuffer->GetRHI());
			}
		}
	);
}

bool FDungeonSceneProviderCommand_GenerateVoxelChunkGPU::IsReadbackReady() {
	return DensityReadback && DensityReadback->IsReady() &&
	       MaterialReadback && MaterialReadback->IsReady();
}

void FDungeonSceneProviderCommand_GenerateVoxelChunkGPU::ProcessReadbackData() {
	SCOPE_CYCLE_COUNTER(STAT_VoxelGPUReadback);
	
	if (!ChunkData || !DensityReadback || !MaterialReadback) return;
	
	bDataCopied.store(false);

	TWeakPtr<FDungeonSceneProviderCommand_GenerateVoxelChunkGPU> WeakThisPtr = SharedThis(this);
	ENQUEUE_RENDER_COMMAND(VoxelProcessReadback)(
		[WeakThisPtr](FRHICommandListImmediate& RHICmdList) {
			if (TSharedPtr<FDungeonSceneProviderCommand_GenerateVoxelChunkGPU> This = WeakThisPtr.Pin()) {
				
				{
					const int32 TotalVoxels = This->ChunkData->DensityData.Num();
					const uint32 NumBytes = TotalVoxels * sizeof(float);
					const float* SrcData = static_cast<const float*>(This->DensityReadback->Lock(NumBytes));
				
					if (SrcData) {
						FMemory::Memcpy(This->ChunkData->DensityData.GetData(), SrcData, NumBytes);
						This->DensityReadback->Unlock();
					}
				}
			
				{
					const int32 TotalVoxels = This->ChunkData->MaterialData.Num();
					const uint32 NumBytes = TotalVoxels * sizeof(uint8);
					const uint8* SrcData = static_cast<const uint8*>(This->MaterialReadback->Lock(NumBytes));
				
					if (SrcData) {
						FMemory::Memcpy(This->ChunkData->MaterialData.GetData(), SrcData, NumBytes);
						This->MaterialReadback->Unlock();
					}
				}
			
				This->bDataCopied.store(true);
			}
		}
	);
}

void FDungeonSceneProviderCommand_GenerateVoxelChunkGPU::ProcessPhaseGenerateMesh(UWorld* World) {
	if (!ChunkData || !Descriptor) return;

	if (!CanGenerateGeometry(ChunkData->DensityData)) {
		return;
	}
	
	GenerateMesh(World, *ChunkData);
}

bool FDungeonSceneProviderCommand_GenerateVoxelChunkGPU::IsDataProcessed() const {
	return bDataCopied.load();
}

void FDungeonSceneProviderCommand_GenerateVoxelChunkGPU::ReleaseGPUResources() {
	if (DensityReadback) {
		delete DensityReadback;
		DensityReadback = nullptr;
	}
	
	if (MaterialReadback) {
		delete MaterialReadback;
		MaterialReadback = nullptr;
	}
	
	DensityBuffer.SafeRelease();
	MaterialBuffer.SafeRelease();
	PrimaryCarveBuffer.SafeRelease();
	AddGeometryBuffer.SafeRelease();
}

