//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/PCG/Nodes/PCGNodeGetSDFTexture.h"

#include "Core/Dungeon.h"
#include "Core/DungeonConfig.h"
#include "Core/Utils/HeatmapColorRamp.h"
#include "Frameworks/PCG/Data/PCGSGFTextureData.h"
#include "Frameworks/PCG/DungeonPCGAttribute.h"
#include "Frameworks/PCG/DungeonPCGLib.h"

#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "Helpers/PCGHelpers.h"
#include "Helpers/PCGSettingsHelpers.h"
#include "PCGComponent.h"
#include "PCGContext.h"
#include "PCGCrc.h"
#include "Serialization/ArchiveObjectCrc32.h"

#define LOCTEXT_NAMESPACE "PCGDungeonSDFTextureSampler"

DEFINE_LOG_CATEGORY_STATIC(LogDungeonPCGSDF, Log, All);

UPCGDungeonSDFTextureSamplerSettings::UPCGDungeonSDFTextureSamplerSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	HeatmapColorRamp = TSoftObjectPtr<UHeatmapColorRamp>(FSoftObjectPath(TEXT("/DungeonArchitect/Core/Features/PCG/SDFHeatmapColorRamp.SDFHeatmapColorRamp")));
	
#if WITH_EDITORONLY_DATA
	DebugSettings.MaterialOverride = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Script/Engine.MaterialInstanceConstant'/PCG/DebugObjects/PCG_DebugMaterial_Color.PCG_DebugMaterial_Color'")));
#endif // WITH_EDITORONLY_DATA
}

#if WITH_EDITOR
FText UPCGDungeonSDFTextureSamplerSettings::GetDefaultNodeTitle() const {
	return NSLOCTEXT("PCGDungeonSDFSamplerSettings", "NodeTitle", "Get Dungeon SDF Sampler");
}

FText UPCGDungeonSDFTextureSamplerSettings::GetNodeTooltipText() const
{
	return LOCTEXT("NodeTooltip", "Sample the dungeon layout's SDF texture.");
}
#endif // WITH_EDITOR

TArray<FPCGPinProperties> UPCGDungeonSDFTextureSamplerSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> Properties;
	Properties.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Texture);

	return Properties;
}

FPCGElementPtr UPCGDungeonSDFTextureSamplerSettings::CreateElement() const
{
	return MakeShared<FPCGDungeonSDFTextureSamplerElement>();
}

bool FPCGDungeonSDFTextureSamplerElement::PrepareDataInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGDungeonSDFTextureSamplerElement::PrepareDataInternal);

	FPCGDungeonSDFTextureSamplerContext* Context = static_cast<FPCGDungeonSDFTextureSamplerContext*>(InContext);
	check(Context);

	Context->bInitiatedPointDataGeneration = false;
	Context->bInitiatedFloorTexGeneration = false;
	
	return true;
}

bool FPCGDungeonSDFTextureSamplerElement::ExecuteInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGDungeonSDFTextureSamplerElement::Execute);
	FPCGDungeonSDFTextureSamplerContext* Context = static_cast<FPCGDungeonSDFTextureSamplerContext*>(InContext);
	check(Context);

	const UPCGDungeonSDFTextureSamplerSettings* Settings = Context->GetInputSettings<UPCGDungeonSDFTextureSamplerSettings>();
	check(Settings);

	if (const UPCGComponent* SourceComponent = Cast<UPCGComponent>(InContext->ExecutionSource.Get())) {
		if (ADungeon* Dungeon = Cast<ADungeon>(SourceComponent->GetOwner())) {
			if (Dungeon->PCGCache) {
				if (Dungeon->PCGCache->SDFPointDataLoadState == EDungeonPCGDataLoadState::Loaded) {
					TArray<FPCGTaggedData>& Outputs = InContext->OutputData.TaggedData;
					FPCGTaggedData& Output = Outputs.Emplace_GetRef();
					Output.Data = Dungeon->PCGCache->SDFPointData;
					return true;
				}

				if (Dungeon->PCGCache->SDFPointDataLoadState == EDungeonPCGDataLoadState::Loading) {
					if (!Context->bInitiatedPointDataGeneration) {
						// This is being loaded from another node. Wait for the load to complete
						return false; 
					}
				}
		
				Dungeon->PCGCache->SDFPointDataLoadState = EDungeonPCGDataLoadState::Loading;
				Context->bInitiatedPointDataGeneration = true;
			}
		}
	}

	if (Context->bIsPaused) {
		return false;
	}
	
	
	UDungeonCanvasComponent* CanvasComponent = FDungeonPCGLib::GetCanvasComponent(Context);
	if (!CanvasComponent) {
		UE_LOG(LogDungeonPCGSDF, Log, TEXT("Failed to find canvas component"));
		return true;
	}
	
	if (!Context->bInitiatedFloorTexGeneration) {
		Context->bInitiatedFloorTexGeneration = true;
		CanvasComponent->GenerateFloorTextures(Settings->FloorIndex);
		Context->FloorTextureRenderFence.BeginFence();
	}

	if (!Context->FloorTextureRenderFence.IsFenceComplete()) {
		return false;
	}
	
	UCanvasRenderTarget2D* TextureSDF{};
	UCanvasRenderTarget2D* TextureHeight{};
	float GroundHeightMin{};
	float GroundHeightMax{};
	float GroundHeightBias{};
	
	if (const FDungeonCanvasLayoutFloorTextures* FloorTextures = CanvasComponent->GetFloorTextures(Settings->FloorIndex)) {
		TextureSDF = FloorTextures->SDF;
		TextureHeight = FloorTextures->LayoutFill;
		GroundHeightMin = FloorTextures->GroundHeightMin;
		GroundHeightMax = FloorTextures->GroundHeightMax;
		GroundHeightBias = FloorTextures->GroundHeightBias;
	}
	
	if (!TextureSDF || !TextureHeight) {
		UE_LOG(LogDungeonPCGSDF, Log, TEXT("Dungeon SDF Texture could not be loaded"));
		return true;
	}
	
	FDungeonCanvasViewportTransform FullDungeonTransform = CanvasComponent->GetFullDungeonViewTransform();
	const FTransform Transform = FullDungeonTransform.GetLocalToWorld();
	FVector FullDungeonScale = Transform.GetScale3D();
	float DungeonSize = FMath::Max(FullDungeonScale.X, FullDungeonScale.Y);
	if (DungeonSize <= 0) {
		DungeonSize = 1;
	}

	const float TextureSize = FMath::Max(TextureSDF->SizeX, TextureSDF->SizeY);
	const float SampleGridSize = Settings->SampleGridSize > 0 ? Settings->SampleGridSize : 100;
	const float TexelSize = SampleGridSize / DungeonSize * TextureSize;

	const UPCGComponent* SourceComponent = Cast<UPCGComponent>(Context->ExecutionSource.Get());
	UPCGDungeonSGFTextureData* TextureData = NewObject<UPCGDungeonSGFTextureData>();
	if (ADungeon* Dungeon = Cast<ADungeon>(SourceComponent->GetOwner())) {
		if (Dungeon->PCGCache) {
			Dungeon->PCGCache->SDFPointData = TextureData;
		}
	}
	FTransform FinalTransform = Transform;
	FinalTransform.SetScale3D(Transform.GetScale3D() * 0.5f);

	// Initialize & set properties
	Context->bIsPaused = true;

	auto PostInitializeCallback = [Context, SourceComponent, TextureData]()
	{
		Context->bIsPaused = false;
		if (ADungeon* Dungeon = Cast<ADungeon>(SourceComponent->GetOwner())) {
			if (Dungeon->PCGCache) {
				Dungeon->PCGCache->SDFPointDataLoadState = EDungeonPCGDataLoadState::Loaded;
				Dungeon->PCGCache->SDFPointData = TextureData;
			}
		}
		
		if (!TextureData->IsValid())
		{
			PCGE_LOG_C(Error, GraphAndLog, Context, LOCTEXT("TextureDataInitFailed", "Texture data failed to initialize, check log for more information"));
		}
	};
	
	TextureData->HeatmapColorRamp = Settings->HeatmapColorRamp.LoadSynchronous();
	TextureData->ColorChannel = Settings->ColorChannel;
	TextureData->Filter = Settings->Filter;
	TextureData->TexelSize = TexelSize;
	TextureData->bUseAdvancedTiling = Settings->bUseAdvancedTiling;
	TextureData->Tiling = Settings->Tiling;
	TextureData->CenterOffset = Settings->CenterOffset;
	TextureData->Rotation = Settings->Rotation;
	TextureData->bUseTileBounds = Settings->bUseTileBounds;
	TextureData->TileBounds = FBox2D(Settings->TileBoundsMin, Settings->TileBoundsMax);

	TextureData->Initialize(TextureSDF, TextureHeight, FinalTransform, GroundHeightMin, GroundHeightMax, GroundHeightBias, PostInitializeCallback);
	
	return false;
}

FPCGContext* FPCGDungeonSDFTextureSamplerElement::CreateContext()
{
	return new FPCGDungeonSDFTextureSamplerContext();
}

void FPCGDungeonSDFTextureSamplerElement::GetDependenciesCrc(const FPCGGetDependenciesCrcParams& InParams, FPCGCrc& OutCrc) const
{
	FPCGCrc Crc;
	IPCGElement::GetDependenciesCrc(InParams, Crc);
	
	if (const UPCGComponent* SourceComponent = Cast<UPCGComponent>(InParams.ExecutionSource)) {
		if (const UPCGData* Data = SourceComponent->GetActorPCGData()) {
			// Add the actor data to the crc
			Crc.Combine(Data->GetOrComputeCrc(/*bFullDataCrc=*/false));

			// Add dungeon config data to Crc
			if (ADungeon* Dungeon = Cast<ADungeon>(SourceComponent->GetOwner())) {
				if (UDungeonConfig* DungeonConfig = Dungeon->GetConfig()) {
					FArchiveObjectCrc32 ObjectCrc;
					Crc.Combine(ObjectCrc.Crc32(DungeonConfig));
				}
			}
		}
	}

	OutCrc = Crc;
}



#undef LOCTEXT_NAMESPACE


