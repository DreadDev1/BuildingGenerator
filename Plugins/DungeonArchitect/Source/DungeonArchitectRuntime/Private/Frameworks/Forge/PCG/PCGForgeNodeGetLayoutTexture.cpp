//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/PCG/PCGForgeNodeGetLayoutTexture.h"

#include "Core/Dungeon.h"
#include "Core/DungeonConfig.h"
#include "Core/Utils/DungeonLog.h"
#include "Core/Utils/HeatmapColorRamp.h"
#include "Frameworks/Forge/Data/DungeonForgeLayoutTextureData.h"
#include "Frameworks/Forge/PCG/DungeonForgePCGConstants.h"
#include "Frameworks/PCG/Data/PCGSGFTextureData.h"
#include "Frameworks/PCG/DungeonPCGAttribute.h"

#include "Engine/CanvasRenderTarget2D.h"
#include "GameFramework/Actor.h"
#include "Helpers/PCGGraphParametersHelpers.h"
#include "Helpers/PCGHelpers.h"
#include "Helpers/PCGSettingsHelpers.h"
#include "PCGComponent.h"
#include "PCGContext.h"
#include "PCGCrc.h"
#include "PCGGraph.h"
#include "Serialization/ArchiveObjectCrc32.h"

#define LOCTEXT_NAMESPACE "PCGForgeFloorTextureSampler"

UPCGForgeLayoutTextureSamplerSettings::UPCGForgeLayoutTextureSamplerSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	HeatmapColorRamp = TSoftObjectPtr<UHeatmapColorRamp>(FSoftObjectPath(TEXT("/DungeonArchitect/Core/Features/PCG/SDFHeatmapColorRamp.SDFHeatmapColorRamp")));
	
#if WITH_EDITORONLY_DATA
	DebugSettings.MaterialOverride = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Script/Engine.MaterialInstanceConstant'/PCG/DebugObjects/PCG_DebugMaterial_Color.PCG_DebugMaterial_Color'")));
#endif // WITH_EDITORONLY_DATA
}

#if WITH_EDITOR
FText UPCGForgeLayoutTextureSamplerSettings::GetDefaultNodeTitle() const {
	return LOCTEXT("NodeTitle", "Get Dungeon Forge Layout Textures");
}

FText UPCGForgeLayoutTextureSamplerSettings::GetNodeTooltipText() const
{
	return LOCTEXT("NodeTooltip", "Sample the dungeon forge's layout texture (SDF and heights)");
}
#endif // WITH_EDITOR

TArray<FPCGPinProperties> UPCGForgeLayoutTextureSamplerSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> Properties;
	Properties.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Texture);

	return Properties;
}

FPCGElementPtr UPCGForgeLayoutTextureSamplerSettings::CreateElement() const
{
	return MakeShared<FPCGForgeLayoutTextureSamplerElement>();
}

bool FPCGForgeLayoutTextureSamplerElement::PrepareDataInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGForgeLayoutTextureSamplerElement::PrepareDataInternal);

	FPCGForgeLayoutTextureSamplerContext* Context = static_cast<FPCGForgeLayoutTextureSamplerContext*>(InContext);
	check(Context);

	return true;
}


bool FPCGForgeLayoutTextureSamplerElement::ExecuteInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGForgeLayoutTextureSamplerElement::ExecuteInternal);
	FPCGForgeLayoutTextureSamplerContext* Context = static_cast<FPCGForgeLayoutTextureSamplerContext*>(InContext);
	check(Context);

	if (Context->bIsPaused) {
		return false;
	}
	
	const UPCGForgeLayoutTextureSamplerSettings* Settings = Context->GetInputSettings<UPCGForgeLayoutTextureSamplerSettings>();
	check(Settings);

	if (Context->SDFPointDataLoadState == EDungeonPCGDataLoadState::Loaded) {
		TArray<FPCGTaggedData>& Outputs = InContext->OutputData.TaggedData;
		FPCGTaggedData& Output = Outputs.Emplace_GetRef();
		Output.Data = Context->SDFPointData;
		Context->SDFPointData->RemoveFromRoot();
		Context->SDFPointData = {};
		return true;
	}
	
	Context->SDFPointDataLoadState = EDungeonPCGDataLoadState::Loading;
	
	const UPCGComponent* SourceComponent = Cast<UPCGComponent>(Context->ExecutionSource.Get());
	UDungeonForgeLayoutTextureData* FloorTextureData{};
	if (SourceComponent) {
		if (UPCGGraphInstance* GraphInstance = SourceComponent->GetGraphInstance()) {
			FloorTextureData = Cast<UDungeonForgeLayoutTextureData>(UPCGGraphParametersHelpers::GetObjectParameter(GraphInstance, DungeonForgePCGConstants::ParameterLayoutTextures));
		}
	}

	if (!FloorTextureData) {
		UE_LOG(LogDungeonForge, Log, TEXT("Floor textures not passed in as a valid parameter to the PCG graph"));
		return true;
	}

	float BaseZ = FloorTextureData->WorldTransform.GetLocation().Z;
	const FDungeonCanvasLayoutFloorTextures& FloorTextures = FloorTextureData->FloorTextures;
	UCanvasRenderTarget2D* TextureSDF = FloorTextures.SDF;
	UCanvasRenderTarget2D* TextureHeight = FloorTextures.LayoutFill;
	const float GroundHeightMin = FloorTextures.GroundHeightMin + BaseZ;
	const float GroundHeightMax = FloorTextures.GroundHeightMax + BaseZ;
	const float GroundHeightBias = FloorTextures.GroundHeightBias;
	
	if (!TextureSDF || !TextureHeight) {
		UE_LOG(LogDungeonForge, Log, TEXT("Dungeon SDF Texture could not be loaded"));
		return true;
	}
	
	const FTransform Transform = FloorTextureData->GetTextureWorldTransform();
	FVector FullDungeonScale = Transform.GetScale3D();
	float DungeonSize = FMath::Max(FullDungeonScale.X, FullDungeonScale.Y);
	if (DungeonSize <= 0) {
		DungeonSize = 1;
	}

	const float TextureSize = FMath::Max(TextureSDF->SizeX, TextureSDF->SizeY);
	const float SampleGridSize = Settings->SampleGridSize > 0 ? Settings->SampleGridSize : 100;
	const float TexelSize = SampleGridSize / DungeonSize * TextureSize;

	UPCGDungeonSGFTextureData* TextureData = NewObject<UPCGDungeonSGFTextureData>();
	Context->SDFPointData = TextureData;
	Context->SDFPointData->AddToRoot();
	
	FTransform FinalTransform = Transform;
	FinalTransform.SetScale3D(Transform.GetScale3D() * 0.5f);

	// Initialize & set properties
	Context->bIsPaused = true;

	auto PostInitializeCallback = [Context, TextureData]()
	{
		Context->bIsPaused = false;
		Context->SDFPointDataLoadState = EDungeonPCGDataLoadState::Loaded;
		
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

FPCGContext* FPCGForgeLayoutTextureSamplerElement::CreateContext()
{
	return new FPCGForgeLayoutTextureSamplerContext();
}

FPCGForgeLayoutTextureSamplerContext::~FPCGForgeLayoutTextureSamplerContext() {
	if (SDFPointData) {
		SDFPointData->RemoveFromRoot();
		SDFPointData = nullptr;
	}	
}

void FPCGForgeLayoutTextureSamplerElement::GetDependenciesCrc(const FPCGGetDependenciesCrcParams& InParams, FPCGCrc& OutCrc) const {
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


