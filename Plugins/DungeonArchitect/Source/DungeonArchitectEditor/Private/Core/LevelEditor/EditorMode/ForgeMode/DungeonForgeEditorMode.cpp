//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorMode.h"

#include "Builders/Forge/DungeonForgeBuilder.h"
#include "Builders/Forge/DungeonForgeConfig.h"
#include "Core/Common/DungeonArchitectEditorLog.h"
#include "Core/Dungeon.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorModeCommands.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorModeToolkit.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/Tools/DungeonForgePaintBrushTool.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/Tools/DungeonForgeTransformTool.h"
#include "Core/Utils/Debug/DungeonDebugVisualizer.h"
#include "Frameworks/Forge/DungeonForgeAsset.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraph.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

#include "BaseGizmos/TransformGizmoUtil.h"
#include "EngineUtils.h"
#include "InteractiveToolManager.h"
#include "Snapping/ModelingSceneSnappingManager.h"
#include "Tools/EdModeInteractiveToolsContext.h"

#define LOCTEXT_NAMESPACE "DungeonForgeEditorMode"

const FEditorModeID UDungeonForgeEditorMode::EM_DungeonForgeEditorModeId = TEXT("EM_DungeonForgeEditorMode");

FString UDungeonForgeEditorMode::GridToolName = TEXT("DungeonForgeEdMode_GridTool");
FString UDungeonForgeEditorMode::TextureToolName = TEXT("DungeonForgeEdMode_TextureTool");
FString UDungeonForgeEditorMode:: TransformToolName = TEXT("DungeonForgeEdMode_TransformTool");
FString UDungeonForgeEditorMode:: TexturePaintToolName = TEXT("DungeonForgeEdMode_TexturePaintTool");

UDungeonForgeEditorMode::UDungeonForgeEditorMode()
{
	// appearance and icon in the editing mode ribbon can be customized here
	Info = FEditorModeInfo(UDungeonForgeEditorMode::EM_DungeonForgeEditorModeId,
		LOCTEXT("ModeName", "Dungeon Forge"),
		FSlateIcon(),
		true);
}


void UDungeonForgeEditorMode::Enter() {
	UEdMode::Enter();

	const FDungeonForgeEditorModeCommands& ToolCommands = FDungeonForgeEditorModeCommands::Get();
	
	TransformToolBuilder = NewObject<UDungeonForgeTransformToolBuilder>(this);
	TransformToolBuilder->SetEdMode(this);

	PaintBrushToolBuilder = NewObject<UDungeonForgePaintBrushToolBuilder>(this);
	PaintBrushToolBuilder->SetEdMode(this);
	
	RegisterTool(ToolCommands.TransformTool, TransformToolName, TransformToolBuilder);
	RegisterTool(ToolCommands.TexturePaintTool, TexturePaintToolName, PaintBrushToolBuilder);

	
	// register gizmo helper
	UE::TransformGizmoUtil::RegisterTransformGizmoContextObject(GetInteractiveToolsContext());
	// configure mode-level Gizmo options
	GetInteractiveToolsContext()->SetForceCombinedGizmoMode(true);
	GetInteractiveToolsContext()->SetAbsoluteWorldSnappingEnabled(true);

	// Now that we have the gizmo helper, bind the numerical UI.
	if (ensure(Toolkit.IsValid()))
	{
		static_cast<FDungeonForgeEditorModeToolkit*>(Toolkit.Get())->BindGizmoNumericalUI();
	}

	// register snapping manager
	UE::Geometry::RegisterSceneSnappingManager(GetInteractiveToolsContext());
	SceneSnappingManager = UE::Geometry::FindModelingSceneSnappingManager(GetToolManager());
	
	// active tool type is not relevant here, we just set to default
	//GetToolManager()->SelectActiveToolType(EToolSide::Left, GridToolName);

	FindAndCacheDungeonForgeActor();
}

void UDungeonForgeEditorMode::Exit() {
	UE::Geometry::DeregisterSceneSnappingManager(GetInteractiveToolsContext());
	SceneSnappingManager = nullptr;
	TransformToolBuilder = nullptr;
	
	Super::Exit();
}

void UDungeonForgeEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FDungeonForgeEditorModeToolkit);
}

void UDungeonForgeEditorMode::ActorSelectionChangeNotify()
{
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> UDungeonForgeEditorMode::GetModeCommands() const
{
	return FDungeonForgeEditorModeCommands::Get().GetCommands();
}

ADungeon* UDungeonForgeEditorMode::GetDungeonActor() {
	if (!Dungeon.IsValid()) {
		FindAndCacheDungeonForgeActor();
	}
	return Dungeon.Get();
}

void UDungeonForgeEditorMode::FindAndCacheDungeonForgeActor() {
	Dungeon = nullptr;
	if (UWorld* World = GEditor->GetEditorWorldContext().World()) {
		for (TActorIterator<ADungeon> It(World); It; ++It) {
			ADungeon* DungeonCandidate = *It;
			if (UDungeonForgeBuilder* DungeonForgeBuilder = Cast<UDungeonForgeBuilder>(DungeonCandidate->GetBuilder())) {
				Dungeon = DungeonCandidate;
				break;
			}
		}
	}

	if (Dungeon.IsValid()) {
		Dungeon->OnDungeonBuildComplete.RemoveAll(this);
		Dungeon->OnDungeonBuildComplete.AddDynamic(this, &UDungeonForgeEditorMode::HandleDungeonBuildComplete);
	}
}

TWeakObjectPtr<UDungeonForgeGraph> UDungeonForgeEditorMode::GetForgeGraph() const {
	if (Dungeon.IsValid()) {
		if (UDungeonForgeConfig* ForgeConfig = Cast<UDungeonForgeConfig>(Dungeon->GetConfig())) {
			if (TObjectPtr<UDungeonForgeAsset> ForgeAsset = ForgeConfig->DungeonForgeAsset.LoadSynchronous()) {
				if (IsValid(ForgeAsset->ForgeGraph)) {
					return ForgeAsset->ForgeGraph;
				}
			}
		}
	}
	return nullptr;
}

void UDungeonForgeEditorMode::HandleDungeonBuildComplete(ADungeon* InDungeon, bool bInSuccess) {
}

void UDungeonForgeEditorMode::SpawnDungeonForgeActor(const FAssetData& InForgeAssetPath) {
	if (UWorld* World = GEditor->GetEditorWorldContext().World()) {
		if (UDungeonForgeAsset* ForgeAsset = Cast<UDungeonForgeAsset>(InForgeAssetPath.GetAsset())) {
			FActorSpawnParameters SpawnParams;
			Dungeon = World->SpawnActor<ADungeon>(ADungeon::StaticClass(), SpawnParams);
			Dungeon->SetBuilderClass(UDungeonForgeBuilder::StaticClass());
			if (UDungeonForgeConfig* ForgeConfig = Cast<UDungeonForgeConfig>(Dungeon->GetConfig())) {
				ForgeConfig->DungeonForgeAsset = ForgeAsset;
			}
			World->Modify();
		}
	}
}

void UDungeonForgeEditorMode::ActiveTransformTool(UDungeonForgeNodeSettings* InNodeSettings) const {
	UE_LOG(LogDungeonForgeEdMode, Log, TEXT("Location Tool activated for node %s"), *(InNodeSettings ? InNodeSettings->GetDefaultNodeName() : TEXT("None")).ToString());

	constexpr EToolSide ToolSide = EToolSide::Mouse;
	if (UInteractiveToolManager* ToolManager = GetToolManager()) {
		if (TransformToolBuilder) {
			TransformToolBuilder->SetNodeSettings(InNodeSettings);
		}
		
		ToolManager->SelectActiveToolType(ToolSide, TransformToolName);
		ToolManager->ActivateTool(ToolSide);
	}
}

void UDungeonForgeEditorMode::ActivePaintBrushTool(UDungeonForgeNodeSettings* InNodeSettings) const {
	UE_LOG(LogDungeonForgeEdMode, Log, TEXT("Paint Brush Tool activated for node %s"), *(InNodeSettings ? InNodeSettings->GetDefaultNodeName() : TEXT("None")).ToString());

	if (InNodeSettings && Dungeon.IsValid()) {
		constexpr EToolSide ToolSide = EToolSide::Mouse;
		if (UInteractiveToolManager* ToolManager = GetToolManager()) {
			if (PaintBrushToolBuilder) {
				PaintBrushToolBuilder->SetState(InNodeSettings, Dungeon.Get());
			}
		
			ToolManager->SelectActiveToolType(ToolSide, TexturePaintToolName);
			ToolManager->ActivateTool(ToolSide);
		}
	}
}

void UDungeonForgeEditorMode::BuildDungeon() const {
	UE_LOG(LogDungeonForgeEdMode, Log, TEXT("Built Forge Dungeon"));

	if (Dungeon.IsValid()) {
		Dungeon->BuildDungeon();
	}
}

void UDungeonForgeEditorMode::SetSelectedObject(UObject* InObject) const {
	if (ensure(Toolkit.IsValid())) {
		static_cast<FDungeonForgeEditorModeToolkit*>(Toolkit.Get())->SetSelectedObject(InObject);
	}
}

void UDungeonForgeEditorMode::SetSelectedObjects(TArray<UObject*> InObjects) const {
	if (ensure(Toolkit.IsValid())) {
		static_cast<FDungeonForgeEditorModeToolkit*>(Toolkit.Get())->SetSelectedObjects(InObjects);
	}
}

#undef LOCTEXT_NAMESPACE

