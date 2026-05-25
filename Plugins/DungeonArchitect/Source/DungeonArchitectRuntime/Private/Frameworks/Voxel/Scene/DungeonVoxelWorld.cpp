//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Voxel/Scene/DungeonVoxelWorld.h"

#include "Core/Utils/MathUtils.h"
#include "Frameworks/Voxel/Data/ChunkDensityData.h"
#include "Frameworks/Voxel/Meshing/Blocky/BlockMeshTools.h"
#include "Frameworks/Voxel/Meshing/MarchingCube/VoxelMeshBuilder.h"

#include "Components/DynamicMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"

DEFINE_LOG_CATEGORY_STATIC(LogVDB, Log, All);

ADungeonVoxelWorld::ADungeonVoxelWorld() {
	MeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>("Mesh");
	MeshComponent->SetMobility(EComponentMobility::Movable);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	MeshComponent->SetMaterial(0, UMaterial::GetDefaultMaterial(MD_Surface));
	MeshComponent->CollisionType = CTF_UseComplexAsSimple;
	MeshComponent->bEnableComplexCollision = true;
	
	SetRootComponent(MeshComponent);
	
	if (Spheres.Num() == 0) {
		FDAVoxelSphere Sphere1;
		Sphere1.WorldLocation = FVector(100, 100, 100);
		Sphere1.WorldRadius = 80.0f;
		Sphere1.MaterialID = 1;
		Sphere1.Operation = EDAVoxelCSGOperation::Add;
		Spheres.Add(Sphere1);
		
		FDAVoxelSphere Sphere2;
		Sphere2.WorldLocation = FVector(150, 100, 100);
		Sphere2.WorldRadius = 50.0f;
		Sphere2.MaterialID = 2;
		Sphere2.Operation = EDAVoxelCSGOperation::Add;
		Spheres.Add(Sphere2);
		
		FDAVoxelSphere Sphere3;
		Sphere3.WorldLocation = FVector(100, 100, 120);
		Sphere3.WorldRadius = 30.0f;
		Sphere3.MaterialID = 3;
		Sphere3.Operation = EDAVoxelCSGOperation::Subtract;
		Spheres.Add(Sphere3);
		
		FDAVoxelSphere Sphere4;
		Sphere4.WorldLocation = FVector(100, 180, 100);
		Sphere4.WorldRadius = 40.0f;
		Sphere4.MaterialID = 4;
		Sphere4.Operation = EDAVoxelCSGOperation::Add;
		Spheres.Add(Sphere4);
	}
}

void ADungeonVoxelWorld::BuildWorld() {
	using namespace UE::Geometry;
	using FLeafNode = DA::VDB::FNodeLOD0;

	GenerateDensity();
	GenerateMesh();
}

void ADungeonVoxelWorld::RemoveBlock(const FVector& InLocation, const FVector& InNormal, float Radius) {
	const float GridRadius = Radius / VoxelSize;
	
	const FVector LocalLocation = GetActorTransform().InverseTransformPosition(InLocation);
	const FVector LocalNormal = GetActorTransform().InverseTransformVector(InNormal);
	const FVector Offset = (-VoxelSize * 0.5f) * LocalNormal;
	const FVector VoxelLocation = LocalLocation + Offset;
	const FIntVector GridCoord = FDAMathUtils::FloorToInt(VoxelLocation / VoxelSize);
	const int32 Extent = FMath::Floor(GridRadius);
	
	if (Extent == 0) {
		VoxelGrid.Set(GridCoord, { 0 });
	}
	else {
		const FVector Origin = FDAMathUtils::ToVector(GridCoord);
		const float GridRadiusSq = GridRadius * GridRadius;
		
		for (int x = -Extent; x <= Extent; x++) {
			for (int y = -Extent; y <= Extent; y++) {
				for (int z = -Extent; z <= Extent; z++) {
					FVector LocalCoord = FVector(x, y, z);
					const float DistSq = LocalCoord.SizeSquared();
					if (DistSq < GridRadiusSq) {
						bool bShouldClear = true;
						VoxelGrid.Set(GridCoord + FIntVector(x, y, z), { 0 });
					}
				}
			}
		}
	}
	
	GenerateMesh();
}

static DA::VDB::FVoxelData ApplyCSGOperation(const DA::VDB::FVoxelData& Existing, float PrimitiveSDF, EDAVoxelCSGOperation Operation) {
	DA::VDB::FVoxelData Result;
	Result.Material = Existing.Material;
	
	switch (Operation) {
		case EDAVoxelCSGOperation::Add:
			Result.SDF = FMath::Min(Existing.SDF, PrimitiveSDF);
			break;
		case EDAVoxelCSGOperation::Subtract:
			Result.SDF = FMath::Max(Existing.SDF, -PrimitiveSDF);
			break;
		case EDAVoxelCSGOperation::Intersect:
			Result.SDF = FMath::Max(Existing.SDF, PrimitiveSDF);
			break;
	}
	
	return Result;
}

static float BoxSDF(const FVector& Point, const FVector& BoxCenter, const FVector& BoxExtent, const FRotator& BoxRotation) {
	FTransform BoxTransform(BoxRotation, BoxCenter);
	FVector LocalPoint = BoxTransform.InverseTransformPosition(Point);
	
	FVector D = FVector::ZeroVector;
	D.X = FMath::Max(0.0f, FMath::Abs(LocalPoint.X) - BoxExtent.X);
	D.Y = FMath::Max(0.0f, FMath::Abs(LocalPoint.Y) - BoxExtent.Y);
	D.Z = FMath::Max(0.0f, FMath::Abs(LocalPoint.Z) - BoxExtent.Z);
	
	// Distance to box surface
	float OutsideDistance = D.Size();
	float InsideDistance = FMath::Min(FMath::Max(D.X, FMath::Max(D.Y, D.Z)), 0.0f);
	
	return OutsideDistance + InsideDistance;
}

void ADungeonVoxelWorld::GenerateDensity() {
	VoxelGrid.Clear();
	
	const float NarrowBandWidth = VoxelSize * 3.0f;
	
	// Set VDB background value for far exterior
	VoxelGrid.RootNode.BackgroundValue.SDF = NarrowBandWidth;
	VoxelGrid.RootNode.BackgroundValue.Material = 0;
	
	// Process each sphere
	for (const FDAVoxelSphere& Sphere : Spheres) {
		const FIntVector MinGrid = FIntVector(
			FMath::FloorToInt((Sphere.WorldLocation.X - Sphere.WorldRadius) / VoxelSize) - 1,
			FMath::FloorToInt((Sphere.WorldLocation.Y - Sphere.WorldRadius) / VoxelSize) - 1,
			FMath::FloorToInt((Sphere.WorldLocation.Z - Sphere.WorldRadius) / VoxelSize) - 1
		);
		
		const FIntVector MaxGrid = FIntVector(
			FMath::CeilToInt((Sphere.WorldLocation.X + Sphere.WorldRadius + NarrowBandWidth) / VoxelSize) + 1,
			FMath::CeilToInt((Sphere.WorldLocation.Y + Sphere.WorldRadius + NarrowBandWidth) / VoxelSize) + 1,
			FMath::CeilToInt((Sphere.WorldLocation.Z + Sphere.WorldRadius + NarrowBandWidth) / VoxelSize) + 1
		);
		
		for (int x = MinGrid.X; x <= MaxGrid.X; x++) {
			for (int y = MinGrid.Y; y <= MaxGrid.Y; y++) {
				for (int z = MinGrid.Z; z <= MaxGrid.Z; z++) {
					const FIntVector GridCoord(x, y, z);
					const FVector VoxelWorldPos = FVector(GridCoord) * VoxelSize;
					const float Distance = FVector::Dist(VoxelWorldPos, Sphere.WorldLocation);
					const float SphereSDF = Distance - Sphere.WorldRadius;
					
					// Get existing voxel or use background
					DA::VDB::FVoxelData ExistingData;
					if (!VoxelGrid.Get(GridCoord, ExistingData)) {
						ExistingData.SDF = NarrowBandWidth;
						ExistingData.Material = 0;
					}
					
					// Apply CSG operation
					DA::VDB::FVoxelData NewData = ApplyCSGOperation(ExistingData, SphereSDF, Sphere.Operation);
					NewData.SDF = FMath::Clamp(NewData.SDF, -NarrowBandWidth, NarrowBandWidth);
					
					// Determine storage strategy based on result
					const float AbsSDF = FMath::Abs(NewData.SDF);
					
					if (AbsSDF <= NarrowBandWidth) {
						// Narrow band - store as active
						VoxelGrid.Set(GridCoord, NewData, true);
					}
					else if (NewData.SDF < 0.0f) {
						// Deep interior - store as inactive tile
						VoxelGrid.Set(GridCoord, NewData, false);
					}
					else if (Sphere.Operation == EDAVoxelCSGOperation::Subtract && SphereSDF <= 0.0f) {
						// Subtraction carved out interior - store as inactive exterior
						NewData.SDF = NarrowBandWidth;
						VoxelGrid.Set(GridCoord, NewData, false);
					}
					// else: Far exterior, don't write (use background)
				}
			}
		}
	}
	
	// Process each box
	for (const FDAVoxelBox& Box : Boxes) {
		// Calculate axis-aligned bounding box for rotated box
		FTransform BoxTransform(Box.WorldRotation, Box.WorldLocation);
		
		// Get the 8 corners of the box in world space
		TArray<FVector> Corners;
		for (int dx = -1; dx <= 1; dx += 2) {
			for (int dy = -1; dy <= 1; dy += 2) {
				for (int dz = -1; dz <= 1; dz += 2) {
					FVector LocalCorner = FVector(dx * Box.WorldExtent.X, dy * Box.WorldExtent.Y, dz * Box.WorldExtent.Z);
					FVector WorldCorner = BoxTransform.TransformPosition(LocalCorner);
					Corners.Add(WorldCorner);
				}
			}
		}
		
		// Find AABB of rotated box
		FVector MinBounds = Corners[0];
		FVector MaxBounds = Corners[0];
		for (const FVector& Corner : Corners) {
			MinBounds = FVector::Min(MinBounds, Corner);
			MaxBounds = FVector::Max(MaxBounds, Corner);
		}
		
		// Expand bounds by narrow band
		MinBounds -= FVector(NarrowBandWidth);
		MaxBounds += FVector(NarrowBandWidth);
		
		// Convert to grid coordinates
		const FIntVector MinGrid = FIntVector(
			FMath::FloorToInt(MinBounds.X / VoxelSize) - 1,
			FMath::FloorToInt(MinBounds.Y / VoxelSize) - 1,
			FMath::FloorToInt(MinBounds.Z / VoxelSize) - 1
		);
		
		const FIntVector MaxGrid = FIntVector(
			FMath::CeilToInt(MaxBounds.X / VoxelSize) + 1,
			FMath::CeilToInt(MaxBounds.Y / VoxelSize) + 1,
			FMath::CeilToInt(MaxBounds.Z / VoxelSize) + 1
		);
		
		// Process voxels in AABB
		for (int x = MinGrid.X; x <= MaxGrid.X; x++) {
			for (int y = MinGrid.Y; y <= MaxGrid.Y; y++) {
				for (int z = MinGrid.Z; z <= MaxGrid.Z; z++) {
					const FIntVector GridCoord(x, y, z);
					const FVector VoxelWorldPos = FVector(GridCoord) * VoxelSize;
					
					// Calculate SDF for box
					const float BoxSDFValue = BoxSDF(VoxelWorldPos, Box.WorldLocation, Box.WorldExtent, Box.WorldRotation);
					
					// Get existing voxel or use background
					DA::VDB::FVoxelData ExistingData;
					if (!VoxelGrid.Get(GridCoord, ExistingData)) {
						ExistingData.SDF = NarrowBandWidth;
						ExistingData.Material = 0;
					}
					
					// Apply CSG operation
					DA::VDB::FVoxelData NewData = ApplyCSGOperation(ExistingData, BoxSDFValue, Box.Operation);
					
					// Override material if adding
					if (Box.Operation == EDAVoxelCSGOperation::Add && BoxSDFValue <= 0.0f) {
						NewData.Material = Box.MaterialID;
					}
					
					// Determine storage strategy based on result
					const float AbsSDF = FMath::Abs(NewData.SDF);
					
					if (AbsSDF <= NarrowBandWidth) {
						// Narrow band - store as active
						VoxelGrid.Set(GridCoord, NewData, true);
					}
					else if (NewData.SDF < 0.0f) {
						// Deep interior - store as inactive tile
						NewData.SDF = -NarrowBandWidth;
						VoxelGrid.Set(GridCoord, NewData, false);
					}
					else if (Box.Operation == EDAVoxelCSGOperation::Subtract && BoxSDFValue <= 0.0f) {
						// Subtraction carved out interior - store as inactive exterior
						NewData.SDF = NarrowBandWidth;
						VoxelGrid.Set(GridCoord, NewData, false);
					}
					// else: Far exterior, don't write (use background)
				}
			}
		}
	}
	
	UE_LOG(LogVDB, Log, TEXT("Generated %d spheres and %d boxes with CSG operations"), Spheres.Num(), Boxes.Num());
}

void ADungeonVoxelWorld::GenerateMesh() {
	using namespace UE::Geometry;
	
	// Use marching cubes for smooth mesh
	MeshComponent->EditMesh([this](FDynamicMesh3& EditMesh) {
		DA::FDungeonMeshGeometry VoxelGeometry;

		if (bUseMarchingCubes) {
			// Generate marching cubes mesh using VDB
			DA::MarchingCubes::FSettings Settings;
			Settings.VoxelSize = VoxelSize;
			Settings.LODLevel = MeshLODLevel;
			DA::MarchingCubes::FMeshBuilder::GenerateMeshWorld(Settings, VoxelGrid, VoxelGeometry);
		}
		else {
			// Surface Nets
			DA::SurfaceNets::FSettings Settings;
			Settings.VoxelSize = VoxelSize;
			Settings.LODLevel = MeshLODLevel;
			DA::SurfaceNets::FMeshBuilder::GenerateMeshWorld(Settings, VoxelGrid, VoxelGeometry);
		}
		
		// Convert to DynamicMesh3
		EditMesh.Clear();
		EditMesh.EnableTriangleGroups();
		EditMesh.EnableAttributes();
		EditMesh.Attributes()->EnablePrimaryColors();
		EditMesh.Attributes()->EnableMaterialID();
		
		FDynamicMeshUVOverlay* UVOverlay = EditMesh.Attributes()->PrimaryUV();
		FDynamicMeshNormalOverlay* NormalOverlay = EditMesh.Attributes()->PrimaryNormals();
		FDynamicMeshColorOverlay* ColorOverlay = EditMesh.Attributes()->PrimaryColors();
		
		// Add vertices
		TArray<int32> VertexIndices;
		for (const FVertexInfo& Vertex : VoxelGeometry.Vertices) {
			const int32 VertIdx = EditMesh.AppendVertex(Vertex.Position);
			if (VertIdx >= 0) {
				VertexIndices.Add(VertIdx);
				UVOverlay->AppendElement(Vertex.UV);
				NormalOverlay->AppendElement(Vertex.Normal);
				ColorOverlay->AppendElement(Vertex.Color);
			}
		}
		
		UE_LOG(LogVDB, Log, TEXT("Mesh Verts: %d, Tris: %d"), VoxelGeometry.Vertices.Num(), VoxelGeometry.Triangles.Num());
			
		// Add triangles
		for (const FIndex3i& Triangle : VoxelGeometry.Triangles) {
			const FIndex3i MappedTri(VertexIndices[Triangle.A], VertexIndices[Triangle.B], VertexIndices[Triangle.C]);
			const int32 TriId = EditMesh.AppendTriangle(MappedTri, 0);
			
			if (TriId >= 0) {
				UVOverlay->SetTriangle(TriId, MappedTri);
				NormalOverlay->SetTriangle(TriId, MappedTri);
				ColorOverlay->SetTriangle(TriId, MappedTri);
			}
		}
	});

	MeshComponent->Modify();

	{
		/*
		// Use blocky mesh
		MeshComponent->EditMesh([this](FDynamicMesh3& EditMesh) {
			FDAVoxelMeshGenSettings Settings;
			Settings.VoxelSize = VoxelSize;
			Settings.CoordMin = CoordMin;
			Settings.CoordMax = CoordMax;
			Settings.bOptimizeMesh = bOptimizeMesh;
			Settings.Seed = Seed;
	
			FDAVoxelMeshGenerator::GenerateBlockMesh(EditMesh, VoxelGrid, Settings);
		});
		*/
	}

	MeshComponent->UpdateCollision(false);
}


