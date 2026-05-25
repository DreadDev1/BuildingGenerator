//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/GraphEditor/ThemeGraphEditorImpl.h"

#include "Core/Dungeon.h"
#include "Core/DungeonBuilder.h"
#include "Core/Editors/ThemeEditor/GraphEditor/DungeonThemeGraphHandler.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeBase.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMeshList.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraph_DungeonTheme.h"
#include "Core/Editors/ThemeEditor/Widgets/SThemeEditorDropTarget.h"

#include "AssetRegistry/AssetData.h"
#include "AssetSelection.h"
#include "EdGraph/EdGraph.h"
#include "Engine/Blueprint.h"
#include "Engine/StaticMesh.h"
#include "GraphEditor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "ThemeGraphEditorImpl"

/////////////////////// FThemeGraphEditorImpl ///////////////////////
void FThemeGraphEditorImpl::Init(const FThemeGraphEditorImplSettings& InSettings) {
	Settings = InSettings;
	CreateGraphEditorWidget(Settings.ThemeGraph.Get());
	
	// Listen for graph changed event
	OnGraphChangedDelegateHandle = GraphEditor->GetCurrentGraph()->AddOnGraphChangedHandler(
			FOnGraphChanged::FDelegate::CreateSP(this, &FThemeGraphEditorImpl::NotifyGraphChange));
	bGraphStateChanged = true;
	bNeedsFullRebuild = true;

	UEdGraph_DungeonTheme* ThemeGraph = Cast<UEdGraph_DungeonTheme>(GraphEditor->GetCurrentGraph());
	if (ThemeGraph) {
		OnNodePropertyChangedDelegateHandle = ThemeGraph->AddOnNodePropertyChangedHandler(
				FOnThemeGraphPropertyChanged::FDelegate::CreateSP(this, &FThemeGraphEditorImpl::NodePropertyChanged));
	}

	InitThemeGraph(ThemeGraph);
}

FThemeGraphEditorImpl::~FThemeGraphEditorImpl() {
	if (GraphEditor->GetCurrentGraph()) {
		GraphEditor->GetCurrentGraph()->RemoveOnGraphChangedHandler(OnGraphChangedDelegateHandle);
		UEdGraph_DungeonTheme* ThemeGraph = Cast<UEdGraph_DungeonTheme>(GraphEditor->GetCurrentGraph());
		if (ThemeGraph) {
			ThemeGraph->RemoveOnNodePropertyChangedHandler(OnNodePropertyChangedDelegateHandle);
		}
	}
}

void FThemeGraphEditorImpl::Tick(float DeltaTime) {
	if (bGraphStateChanged) {
		FDungeonBuildSettings BuildSettings;
		BuildSettings.StartPhase = bNeedsFullRebuild
			? EDungeonBuildGraphPhase::Initial
			: EDungeonBuildGraphPhase::SpawnItems;

		bGraphStateChanged = false;
		bNeedsFullRebuild = false;
		
		NotifyBuildDungeon(BuildSettings);
	}
}

void FThemeGraphEditorImpl::SetEditorDefaultPropertiesGetter(const TFunction<UObject*()>& InFuncGetEditorProperties) {
	EditorDefaultPropertiesGetter = InFuncGetEditorProperties;
	ThemeGraphHandler->SetEditorPropertiesCallback(InFuncGetEditorProperties);
}

void FThemeGraphEditorImpl::SelectAndFocusOnNode(UEdGraphNode* InNode) const {
	if (GraphEditor.IsValid()) {
		FGraphPanelSelectionSet CurrentSelection = GraphEditor->GetSelectedNodes();
		if (CurrentSelection.Num() != 1 || !CurrentSelection.Contains(InNode)) {
			// The selection has changed
			GraphEditor->ClearSelectionSet();
			if (InNode) {
				GraphEditor->SetNodeSelection(InNode, true);
				GraphEditor->JumpToNode(InNode);
			}
		}
	}
}


void FThemeGraphEditorImpl::HandleAssetsDropped(const FDragDropEvent& InEvent, TArrayView<FAssetData> InAssets) const {
	if (GraphEditor.IsValid()) {
		if (UEdGraph_DungeonTheme* ThemeGraph = Cast<UEdGraph_DungeonTheme>(GraphEditor->GetCurrentGraph())) {
			TArray<FAssetData> StaticMeshAssets;
			TArray<FAssetData> ActorTemplateAssets;
			TArray<FAssetData> OtherAssets;
			for (const FAssetData& Asset : InAssets) {
				if (UObject* AssetObject = Asset.GetAsset()) {
					if (const UClass* AssetClass = AssetObject->GetClass()) {
						if (AssetClass->IsChildOf<UStaticMesh>()) {
							StaticMeshAssets.Add(Asset);
						}
						else if (AssetClass->IsChildOf<UBlueprint>()) {
							if (const UBlueprint* Blueprint = Cast<UBlueprint>(AssetObject)) {
								if (Blueprint->ParentClass && Blueprint->ParentClass->IsChildOf<AActor>()) {
									ActorTemplateAssets.Add(Asset);
								}
							}
						}
						else {
							OtherAssets.Add(Asset);
						}
					}
				}
			}

			auto IncrementOffset = [](FVector2D& InOutOffset) {
				InOutOffset += FVector2D(20, 20);
			};

			auto CreateBasicAssetNodes = [this, ThemeGraph, &IncrementOffset](TArrayView<FAssetData> Assets, FVector2D& InOutOffset) {
				for (const FAssetData& Asset : Assets) {
					if (UObject* AssetObject = Asset.GetAsset()) {
						const FVector2D GridLocation = GetAssetDropGridLocation() + InOutOffset;
						ThemeGraph->CreateNewNode(AssetObject, GridLocation);
						IncrementOffset(InOutOffset);
					}
				}
			};

			FVector2D Offset = FVector2D::ZeroVector;
			CreateBasicAssetNodes(OtherAssets, Offset);

			const int32 MultiNodeItemCount = StaticMeshAssets.Num() + ActorTemplateAssets.Num();
			if (MultiNodeItemCount > 1) {
				const FVector2D NodeLocation = GetAssetDropGridLocation() + Offset;
				IncrementOffset(Offset);
				
				UEdGraphNode_DungeonThemeMeshList* MultiMeshNode = ThemeGraph->CreateNewNode<UEdGraphNode_DungeonThemeMeshList>(NodeLocation);
				for (const FAssetData& StaticMeshAsset : StaticMeshAssets) {
					if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(StaticMeshAsset.GetAsset())) {
						FDungeonMeshListItem& MeshItem = MultiMeshNode->Meshes.AddDefaulted_GetRef();
						MeshItem.StaticMesh = StaticMesh;
						MeshItem.ItemGuid = FGuid::NewGuid();
					}
				}
				
				for (const FAssetData& ActorTemplateAsset : ActorTemplateAssets) {
					if (UObject* AssetObject = ActorTemplateAsset.GetAsset()) {
						if (const UBlueprint* Blueprint = Cast<UBlueprint>(AssetObject)) {
							if (Blueprint->ParentClass && Blueprint->ParentClass->IsChildOf<AActor>()) {
								FDungeonActorTemplateListItem& TemplateInfo = MultiMeshNode->Blueprints.AddDefaulted_GetRef();
								TemplateInfo.ClassTemplate = Cast<UClass>(Blueprint->GeneratedClass);
								TemplateInfo.ItemGuid = FGuid::NewGuid();
							}
						}
					}
				}
			}
			else if (StaticMeshAssets.Num() == 1) {
				CreateBasicAssetNodes(StaticMeshAssets, Offset);
			}
			else if (ActorTemplateAssets.Num() == 1) {
				CreateBasicAssetNodes(ActorTemplateAssets, Offset);
			}
		}
	}
}

bool FThemeGraphEditorImpl::AreAssetsAcceptableForDrop(TArrayView<FAssetData> InAssets, FText& OutReason) const {
	bool bAllAssetsValidForDrop = false;
	if (GraphEditor.IsValid()) {
		if (const UEdGraph_DungeonTheme* ThemeGraph = Cast<UEdGraph_DungeonTheme>(GraphEditor->GetCurrentGraph())) {
			bAllAssetsValidForDrop = true;
			for (const FAssetData& AssetData : InAssets) {
				if (const UObject* AssetObject = AssetData.GetAsset()) {
					bool bCanDrop = ThemeGraph->IsAssetAcceptableForDrop(AssetObject);

					if (!bCanDrop) {
						// Check if a broker can convert this asset to an actor
						if (FActorFactoryAssetProxy::GetFactoryForAsset(AssetData) != nullptr) {
							bCanDrop = true;
						}
					}
					
					if (!bCanDrop) {
						bAllAssetsValidForDrop = false;
						break;
					}
				}
			}
		}
	}

	OutReason = bAllAssetsValidForDrop
		? LOCTEXT("DragDropAllowedLabel", "Allowed")
		: LOCTEXT("DragDropNotAllowedLabel", "Not supported");
	
	return bAllAssetsValidForDrop;
}

FVector2D FThemeGraphEditorImpl::GetAssetDropGridLocation() const {
	if (!AssetDropTarget.IsValid()) return FVector2D::ZeroVector;

	FVector2D PanelCoord = AssetDropTarget->GetPanelCoordDropPosition();
	FVector2f ViewLocation = FVector2f::ZeroVector;
	float ZoomAmount = 1.0f;
	GraphEditor->GetViewLocation(ViewLocation, ZoomAmount);
	FVector2D GridLocation = PanelCoord / ZoomAmount + FVector2D(ViewLocation);

	return GridLocation;
}

void FThemeGraphEditorImpl::NodeDoubleClicked(UEdGraphNode* InNode) const {
	if (OnNodeDoubleClicked.IsBound()) {
		OnNodeDoubleClicked.Execute(InNode);
	}
}

void FThemeGraphEditorImpl::SelectedNodesChanged(const TSet<UObject*>& SelectedNodes) const {
	if (OnSelectedNodesChanged.IsBound()) {
		OnSelectedNodesChanged.Execute(SelectedNodes);
	}
}

void FThemeGraphEditorImpl::NotifyBuildDungeon(const FDungeonBuildSettings& InBuildSettings) {
	if (OnBuildDungeon.IsBound()) {
		OnBuildDungeon.Execute(InBuildSettings);
	}
}

bool FThemeGraphEditorImpl::DisplayAsReadOnly() const {
	return !bGraphIsEditable;
}

bool FThemeGraphEditorImpl::IsEditable() const {
	return bGraphIsEditable;
}

void FThemeGraphEditorImpl::SetIsEditable(bool bInEnabled) {
	bGraphIsEditable = bInEnabled;
}

void FThemeGraphEditorImpl::CreateGraphEditorWidget(UEdGraph* InGraph) {
	ThemeGraphHandler = MakeShareable(new FDungeonArchitectThemeGraphHandler);
	ThemeGraphHandler->Bind();
	ThemeGraphHandler->GetGraphEvents().OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FThemeGraphEditorImpl::OnNodeTitleCommitted);
	ThemeGraphHandler->NodeDoubleClicked = FDungeonArchitectThemeGraphHandler::FSingleNodeDelegate::CreateSP(this, &FThemeGraphEditorImpl::NodeDoubleClicked);
	ThemeGraphHandler->SelectedNodesChanged = FDungeonArchitectThemeGraphHandler::FMultiNodeDelegate::CreateSP(this, &FThemeGraphEditorImpl::SelectedNodesChanged);
	
	// Create the appearance info
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText", "Dungeon Architect");

	GraphEditor = SNew(SGraphEditor)
		.AdditionalCommands(ThemeGraphHandler->GetCommands())
		.IsEditable(this, &FThemeGraphEditorImpl::IsEditable)
		.DisplayAsReadOnly(this, &FThemeGraphEditorImpl::DisplayAsReadOnly)
		.Appearance(AppearanceInfo)
		.GraphToEdit(InGraph)
		.GraphEvents(ThemeGraphHandler->GraphEvents);
	
	ThemeGraphHandler->Initialize(GraphEditor, Settings.PropertyEditor, InGraph);
	AssetDropTarget = SNew(SThemeEditorDropTarget)
		.OnAreAssetsAcceptableForDropWithReason(this, &FThemeGraphEditorImpl::AreAssetsAcceptableForDrop)
		.OnAssetsDropped(this, &FThemeGraphEditorImpl::HandleAssetsDropped);

	GraphEditorHost = SNew(SOverlay)
		+ SOverlay::Slot()
		[
			GraphEditor.ToSharedRef()
		]
		+ SOverlay::Slot()
		[
			AssetDropTarget.ToSharedRef()
		];
}

void FThemeGraphEditorImpl::NotifyGraphChange(const FEdGraphEditAction& Action) {
	bGraphStateChanged = true;
	if (OnGraphChanged.IsBound()) {
		OnGraphChanged.Execute(Action);
	}
}

void FThemeGraphEditorImpl::NodePropertyChanged(const FEdGraphEditAction& Action, const FPropertyChangedEvent& InChangeEvent) {
	bGraphStateChanged = true;
	if (OnNodePropertyChanged.IsBound()) {
		OnNodePropertyChanged.Execute(Action, InChangeEvent);
	}
}

void FThemeGraphEditorImpl::InitThemeGraph(UEdGraph_DungeonTheme* ThemeGraph) const {
	if (!ThemeGraph) return;

	for (UEdGraphNode* Node : ThemeGraph->Nodes) {
		UEdGraphNode_DungeonThemeBase* DungeonNode = Cast<UEdGraphNode_DungeonThemeBase>(Node);
		if (DungeonNode) {
			// Make sure the actor node is initialized
			DungeonNode->OnThemeEditorLoaded();
		}
	}
}

void FThemeGraphEditorImpl::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged) {
	if (NodeBeingChanged)
	{
		const FScopedTransaction Transaction( LOCTEXT( "RenameNode", "Rename Node" ) );
		NodeBeingChanged->Modify();
		NodeBeingChanged->OnRenameNode(NewText.ToString());
	}
}

void FThemeGraphEditorImpl::ShowObjectDetails(UObject* ObjectProperties, bool bForceRefresh) const {
	TSharedPtr<IDetailsView> PropertyEditor = Settings.PropertyEditor.Pin();
	if (PropertyEditor.IsValid()) {
		if (!ObjectProperties) {
			ObjectProperties = EditorDefaultPropertiesGetter();
		}
		
		PropertyEditor->SetObject(ObjectProperties, bForceRefresh);
	}
}

void FThemeGraphEditorImpl::SetBuilderClass(TSubclassOf<UDungeonBuilder> InBuilderClass) {
	if (Settings.ThemeGraph.IsValid() && InBuilderClass) {
		if (UEdGraph_DungeonTheme* DungeonGraph = Cast<UEdGraph_DungeonTheme>(Settings.ThemeGraph.Get())) {
			DungeonGraph->RecreateDefaultMarkerNodes(InBuilderClass);
		}
	}
}


#undef LOCTEXT_NAMESPACE

