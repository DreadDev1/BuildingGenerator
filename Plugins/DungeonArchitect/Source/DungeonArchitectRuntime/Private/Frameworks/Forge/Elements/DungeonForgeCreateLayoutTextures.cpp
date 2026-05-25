//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Elements/DungeonForgeCreateLayoutTextures.h"

#include "Core/Dungeon.h"
#include "Core/Utils/DungeonLog.h"
#include "Frameworks/Canvas/DungeonCanvasRendering.h"
#include "Frameworks/Forge/Data/DungeonForgeLayoutData.h"
#include "Frameworks/Forge/Data/DungeonForgeLayoutTextureData.h"
#include "Frameworks/Forge/DungeonForgeResourceManager.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

#include "Engine/CanvasRenderTarget2D.h"

#define LOCTEXT_NAMESPACE "DungeonForgeCreateLayoutTextures"

//////////////////////////// UDungeonForgeCreateLayoutTexturesSettings ////////////////////////////
UDungeonForgeCreateLayoutTexturesSettings::UDungeonForgeCreateLayoutTexturesSettings() {
	DebugMaterial = FSoftObjectPath(TEXT("/DungeonArchitect/Core/Editors/ForgeEditor/Materials/M_VisualizeSDF_Inst.M_VisualizeSDF_Inst"));
}

#if WITH_EDITOR
FName UDungeonForgeCreateLayoutTexturesSettings::GetDefaultNodeName() const {
	return TEXT("CreateFloorTextures");
}

FText UDungeonForgeCreateLayoutTexturesSettings::GetDefaultNodeTitle() const {
	return LOCTEXT("NodeTitle", "Create Layout Textures");
}

FText UDungeonForgeCreateLayoutTexturesSettings::GetNodeTooltipText() const {
	return LOCTEXT("NodeTooltip", "Create the layout textures, like SDF, layout heights etc");
}

EDungeonForgeSettingsType UDungeonForgeCreateLayoutTexturesSettings::GetType() const {
	return EDungeonForgeSettingsType::Generic;
}
#endif // WITH_EDITOR

TArray<FDungeonForgePinProperties> UDungeonForgeCreateLayoutTexturesSettings::InputPinProperties() const {
	TArray<FDungeonForgePinProperties> PinProperties;
	PinProperties.Emplace(DungeonForgePinConstants::DungeonLayoutLabel, EDungeonForgeDataType::DungeonLayout);
	return PinProperties;
}

TArray<FDungeonForgePinProperties> UDungeonForgeCreateLayoutTexturesSettings::OutputPinProperties() const {
	TArray<FDungeonForgePinProperties> PinProperties;
	PinProperties.Emplace(DungeonForgePinConstants::DungeonLayoutTexturesLabel, EDungeonForgeDataType::DungeonLayoutTextures);
	return PinProperties;
}

FDungeonForgeElementPtr UDungeonForgeCreateLayoutTexturesSettings::CreateElement() const {
	return MakeShared<FDungeonForgeCreateLayoutTexturesElement>(); 
}

//////////////////////////// FDungeonForgeCreateLayoutTexturesElement ////////////////////////////
FDungeonForgeContextPtr FDungeonForgeCreateLayoutTexturesElement::CreateContext() {
	return MakeShared<FDungeonForgeLayoutTextureContext>();
}

bool FDungeonForgeCreateLayoutTexturesElement::ExecuteImpl(const FDungeonForgeContextPtr& InContext) const {
	FDungeonForgeLayoutTextureContextPtr Context = StaticCastSharedPtr<FDungeonForgeLayoutTextureContext>(InContext);
	if (!Context->Node.IsValid()) {
		UE_LOG(LogDungeonForge, Error, TEXT("FDungeonForgeCreateFloorTexturesElement: Stale node state"));
		return true;
	}

	UDungeonForgeCreateLayoutTexturesSettings* NodeSettings = Cast<UDungeonForgeCreateLayoutTexturesSettings>(Context->Node->GetSettings());
	if (!NodeSettings) {
		UE_LOG(LogDungeonForge, Error, TEXT("FDungeonForgeCreateFloorTexturesElement: Invalid settings state"));
		return true;
	}

	TSharedPtr<const FDungeonForgeGraphExecutionState> ExecState = Context->ExecutionState.Pin();
	if (!ExecState.IsValid()) {
		UE_LOG(LogDungeonForge, Error, TEXT("FDungeonForgeCreateFloorTexturesElement: Invalid execution state."));
		return true;
	}
	
	ADungeon* ForgeDungeon = ExecState->Dungeon.Get();
	UWorld* World = ForgeDungeon ? ForgeDungeon->GetWorld() : nullptr;
	if (!World) {
		UE_LOG(LogDungeonForge, Error, TEXT("FDungeonForgeCreateFloorTexturesElement: Invalid World state"));
		return true;
	}

	// Generate the floor textures
	UDungeonForgeResourceManager* ResourceManager = ExecState->ResourceManager.Get();
	if (!ResourceManager) {
		UE_LOG(LogDungeonForge, Error, TEXT("FDungeonForgeCreateFloorTexturesElement: Invalid Resource Manager state"));
		return true;
	}

	const UDungeonForgeLayoutData* LayoutData = Context->InputData.GetInputByPinOfType<UDungeonForgeLayoutData>(DungeonForgePinConstants::DungeonLayoutLabel);
	if (!LayoutData) {
		UE_LOG(LogDungeonForge, Error, TEXT("FDungeonForgeCreateFloorTexturesElement: Invalid dungeon layout input"));
		return true;
	}

	FBox LocalBounds = LayoutData->Layout.Bounds.ExpandBy(FVector(NodeSettings->PaddingWorldUnits, NodeSettings->PaddingWorldUnits, 0));
	UDungeonForgeLayoutTextureData* LayoutTextureData = NewObject<UDungeonForgeLayoutTextureData>(ResourceManager);
	LayoutTextureData->LocalBounds = LocalBounds;
	LayoutTextureData->WorldTransform = LayoutData->Layout.DungeonTransform;
	LayoutTextureData->DebugMaterial = NodeSettings->DebugMaterial;

	FDungeonForgeTaggedData LayoutTexPinData;
	LayoutTexPinData.Pin = DungeonForgePinConstants::DungeonLayoutTexturesLabel;
	LayoutTexPinData.Data = LayoutTextureData;
	Context->OutputData.AddData(LayoutTexPinData);

	// Do not generate the dynamic occlusion and fog of war as they are not needed for layout generation
	FDungeonCanvasRenderingLibrary::FFloorTexturesGenerationOptions TexGenOptions{};
	TexGenOptions.bGenerateDynamicOcclusion = false;
	TexGenOptions.bGenerateFogOfWarExplored = false;
	TexGenOptions.bGenerateFogOfWarVisibility = false;
	
	FDungeonCanvasRenderingLibrary::GenerateFloorTextures(World, ResourceManager, LayoutData->Layout, NodeSettings->LayoutTextureSize,
			NodeSettings->PaddingWorldUnits, NodeSettings->bDrawAllFloors, NodeSettings->FloorIndex, LayoutTextureData->FloorTextures, TexGenOptions);

	ResourceManager->RegisterManagedTexture(LayoutTextureData->FloorTextures.SDF);
	ResourceManager->RegisterManagedTexture(LayoutTextureData->FloorTextures.LayoutFill);
	ResourceManager->RegisterManagedTexture(LayoutTextureData->FloorTextures.LayoutBorder);
	ResourceManager->RegisterManagedTexture(LayoutTextureData->FloorTextures.DynamicOcclusion);
	ResourceManager->RegisterManagedTexture(LayoutTextureData->FloorTextures.FogOfWarExplored);
	ResourceManager->RegisterManagedTexture(LayoutTextureData->FloorTextures.FogOfWarVisibility);
	
	Context->TextureRenderFence.BeginFence();

	return Context->TextureRenderFence.IsFenceComplete();
}

bool FDungeonForgeCreateLayoutTexturesElement::TickImpl(const FDungeonForgeContextPtr& InContext, float DeltaTime) const {
	FDungeonForgeLayoutTextureContextPtr Context = StaticCastSharedPtr<FDungeonForgeLayoutTextureContext>(InContext);
	return Context->TextureRenderFence.IsFenceComplete();
}

#undef LOCTEXT_NAMESPACE

