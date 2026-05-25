//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/Common/DungeonArchitectThemeEditorUtils.h"

#include "Builders/Grid/GridDungeonBuilder.h"
#include "Core/Common/Utils/AssetThumbnailCache.h"
#include "Core/Common/Utils/DungeonEditorUtils.h"
#include "Core/Editors/ThemeEditor/AppModes/ThemeGraph/ThemeGraphAppMode.h"
#include "Core/Editors/ThemeEditor/Compiler/DungeonThemeCompiler.h"
#include "Core/Editors/ThemeEditor/DungeonArchitectThemeEditor.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorTemplate.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMarker.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMesh.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeMeshList.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraph_DungeonTheme.h"
#include "Core/Editors/ThemeEditor/Widgets/SThemePreviewViewport.h"
#include "Core/Utils/DungeonUtils.h"
#include "Frameworks/MarkerGenerator/MarkerGenModel.h"
#include "Frameworks/ThemeEngine/Common/DungeonThemeEngineUtils.h"
#include "Frameworks/ThemeEngine/DungeonThemeEngine.h"

#include "ContentBrowserModule.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "EditorSupportDelegates.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GraphEditor.h"
#include "IContentBrowserSingleton.h"
#include "Toolkits/ToolkitManager.h"
#include "Widgets/Layout/SGridPanel.h"

#define LOCTEXT_NAMESPACE "DungeonArchitectThemeEditorUtils"

//////////////////////////////////////// FDungeonArchitectThemeEditorUtils ////////////////////////////////////////  
bool FDungeonArchitectThemeEditorUtils::GetBoundsForSelectedNodes(const UEdGraph* Graph, FSlateRect& Rect, float Padding) {
    const TSharedPtr<FDungeonArchitectThemeEditor> ThemeEditor = GetThemeEditorForAsset(Graph);
    if (ThemeEditor.IsValid()) {
        const TSharedPtr<FThemeGraphAppMode> AppMode = ThemeEditor->GetAppMode<FThemeGraphAppMode>(FDungeonArchitectThemeEditor::AppModeID_GraphEditor);
        if (AppMode.IsValid()) {
            const TSharedPtr<SGraphEditor> GraphEditor = AppMode->GetGraphEditor();
            if (GraphEditor.IsValid()) {
                return GraphEditor->GetBoundsForSelectedNodes(Rect, Padding);
            }
        }
    }
    return false;
}

TSharedPtr<SGridPanel> FDungeonArchitectThemeEditorUtils::CreateAssetThumbWidget(const TArray<UObject*>& ThumbnailObjects, const TSharedPtr<FDAAssetThumbnailCacheManager>& ThumbnailCacheManager) {
    TArray<TSharedPtr<SWidget>> ThumbnailWidgets;
    if (ThumbnailCacheManager.IsValid()) {
        ThumbnailCacheManager->PurgeStaleEntries(ThumbnailObjects);
        for (UObject* ThumbnailObject : ThumbnailObjects) {
            TSharedPtr<SWidget> Widget = SNullWidget::NullWidget;
            if (ThumbnailObject && ThumbnailCacheManager.IsValid()) {
                TSharedPtr<SWidget> EntryWidget = ThumbnailCacheManager->RequestThumbnailWidget(ThumbnailObject);
                if (EntryWidget.IsValid()) {
                    Widget = EntryWidget;
                }
            }
            ThumbnailWidgets.Add(Widget);
        }
    }
    FIntPoint ThumbnailSize = ThumbnailCacheManager.IsValid() ? ThumbnailCacheManager->GetThumbnailSize() : FIntPoint(128, 128);
    
    /*
    TArray<TSharedPtr<FAssetThumbnail>> AssetThumbnails;
    for (const UObject* ThumbnailObject : ThumbnailObjects) {
        TSharedPtr<FAssetThumbnailPool> AssetThumbnailPool = UThumbnailManager::Get().GetSharedThumbnailPool();
        TSharedPtr<FAssetThumbnail> AssetThumbnail = MakeShareable(new FAssetThumbnail(FAssetData(ThumbnailObject), InSize.X, InSize.Y, AssetThumbnailPool));
        
        AssetThumbnails.Add(AssetThumbnail);
    }
    */
    
    TSharedPtr<SGridPanel> ThumbnailWidget = SNew(SGridPanel);
    int NumRows{}, NumCols{};
    if (ThumbnailWidgets.Num() <= 1) {
        NumRows = 1;
        NumCols = 1;
    }
    else if (ThumbnailWidgets.Num() <= 4) {
        NumRows = 2;
        NumCols = 2;
    }
    else {
        NumRows = 3;
        NumCols = 3;
    }

    
    const int CellSizeX = ThumbnailSize.X / NumCols;
    const int CellSizeY = ThumbnailSize.Y / NumRows;
    
    for (int y = 0; y < NumRows; y++) {
        for (int x = 0; x < NumCols; x++) {
            const int Idx = y * NumRows + x;
            TSharedRef<SWidget> Widget = ThumbnailWidgets.IsValidIndex(Idx)
                ? ThumbnailWidgets[Idx].ToSharedRef()
                : SNullWidget::NullWidget;
            
            ThumbnailWidget->AddSlot(x, y)
            [
                SNew(SBox)
                .WidthOverride(CellSizeX)
                .HeightOverride(CellSizeY)
                [
                    Widget
                ]
            ];
        }
    }

    return ThumbnailWidget;
}

TSharedPtr<SGridPanel> FDungeonArchitectThemeEditorUtils::CreateNodeThumbWidget(const UEdGraphNode_DungeonThemeActorBase* InVisualNode, const TSharedPtr<FDAAssetThumbnailCacheManager>& ThumbnailCacheManager) {
    const TArray<UObject*> ThumbnailObjects = InVisualNode ? InVisualNode->GetThumbnailAssetObjects() : TArray<UObject*>();
    return CreateAssetThumbWidget(ThumbnailObjects, ThumbnailCacheManager);
}

void FDungeonArchitectThemeEditorUtils::InitializeNewThemeAsset(UDungeonThemeAsset* NewAsset) {
    NewAsset->Version = static_cast<int32>(EDungeonThemeAssetVersion::LatestVersion);
    InitializeThemeAsset(NewAsset);
}

void FDungeonArchitectThemeEditorUtils::InitializeThemeAsset(UDungeonThemeAsset* AssetBeingEdited) {
    if (!AssetBeingEdited->UpdateGraph) {
        UEdGraph_DungeonTheme* DungeonGraph = NewObject<UEdGraph_DungeonTheme>(
                AssetBeingEdited, UEdGraph_DungeonTheme::StaticClass(), NAME_None, RF_Transactional);
        DungeonGraph->RecreateDefaultMarkerNodes(UGridDungeonBuilder::StaticClass());
        
        AssetBeingEdited->UpdateGraph = DungeonGraph;
        AssetBeingEdited->Modify();
    }

    if (!AssetBeingEdited->MarkerGenerationModel) {
        AssetBeingEdited->MarkerGenerationModel = NewObject<UMarkerGenModel>(AssetBeingEdited);
        AssetBeingEdited->Modify();
    }
}

bool FDungeonArchitectThemeEditorUtils::CompileThemeGraph(UDungeonThemeAsset* InThemeAsset, TArray<FDungeonThemeGraphBuildError>& OutErrors) {
    OutErrors.Reset();
    
    TSharedPtr<IDungeonThemeCompiler> Compiler = IDungeonThemeCompiler::CreateLatestCompiler();
    if (Compiler.IsValid()) {
        if (Compiler->Compile(InThemeAsset, OutErrors)) {
            FEditorDelegates::RefreshEditor.Broadcast();
            FEditorSupportDelegates::RedrawAllViewports.Broadcast();
            return true;
        }
    }
    
    return false;
}

void FDungeonArchitectThemeEditorUtils::SaveThemeAsset(UDungeonThemeAsset* InThemeAsset, FViewport* InViewportForThumbnail) {
    if (InThemeAsset) {
        if (UPackage* Package = InThemeAsset->GetOutermost()) {
            TArray<UPackage*> PackagesToSave;
            PackagesToSave.Add(Package);

            // Update the asset thumbnail
            FDungeonLevelEditorUtils::UpdateAssetThumbnail(InThemeAsset, InViewportForThumbnail);

            // Save the asset
            FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false, false);
        }
    }
}

void FDungeonArchitectThemeEditorUtils::GetNodeTags(const UEdGraphNode_DungeonThemeActorBase* InNode, TArray<FName>& OutTags) {
    OutTags = {};
    if (!InNode) {
        return;
    }
    
    if (const UEdGraphNode_DungeonThemeMeshList* MultiActorNode = Cast<UEdGraphNode_DungeonThemeMeshList>(InNode)) {
        for (const FDungeonMeshListItem& Item : MultiActorNode->Meshes) {
            const FName ItemTag = FDungeonThemeEngineUtils::CreateNodeTagFromId(Item.GetNodeItemGuid(InNode->NodeGuid));
            OutTags.Add(ItemTag);
        }

        for (const FDungeonActorTemplateListItem& Item : MultiActorNode->Blueprints) {
            const FName ItemTag = FDungeonThemeEngineUtils::CreateNodeTagFromId(Item.GetNodeItemGuid(InNode->NodeGuid));
            OutTags.Add(ItemTag);
        }
    }
    else {
        OutTags.Add(FDungeonThemeEngineUtils::CreateNodeTagFromId(InNode->NodeGuid));
    }
}

UEdGraphNode_DungeonThemeActorBase* FDungeonArchitectThemeEditorUtils::FindThemeNodeFromSpawnedActor(AActor* InSpawnedActor, UEdGraph* InThemeGraph) {
    if (InSpawnedActor && InThemeGraph) {
        TSet<FName> ActorTags(InSpawnedActor->Tags);
        
        for (UEdGraphNode* GraphNode : InThemeGraph->Nodes) {
            if (UEdGraphNode_DungeonThemeActorBase* ActorNode = Cast<UEdGraphNode_DungeonThemeActorBase>(GraphNode)) {
                TArray<FName> NodeTags;
                GetNodeTags(ActorNode, NodeTags);
                for (const FName& NodeTag : NodeTags) {
                    if (ActorTags.Contains(NodeTag)) {
                        return ActorNode;
                    }
                }
            }
        }
    }
    return nullptr;
}

UEdGraphNode_DungeonThemeMarker* FDungeonArchitectThemeEditorUtils::GetAttachedMarkerNode(const UEdGraphNode_DungeonThemeActorBase* InVisualNode, bool bRecursive) {
    if (InVisualNode) {
        if (UEdGraphPin* InputPin = InVisualNode->GetInputPin()) {
            for (UEdGraphPin* LinkedPin : InputPin->LinkedTo) {
                if (LinkedPin) {
                    return Cast<UEdGraphNode_DungeonThemeMarker>(LinkedPin->GetOwningNode());
                }
            }
        }
    }
    return nullptr;
}

UEdGraphNode_DungeonThemeBase* FDungeonArchitectThemeEditorUtils::GetParentNode(const UEdGraphNode_DungeonThemeBase* InNode) {
    if (InNode && InNode->GetInputPin() && InNode->GetInputPin()->LinkedTo.Num() > 0) {
        if (UEdGraphPin* ParentNodePin = InNode->GetInputPin()->LinkedTo[0]) {
            return Cast<UEdGraphNode_DungeonThemeBase>(ParentNodePin->GetOwningNode());
        }
    }
    return nullptr;
}

UEdGraphNode_DungeonThemeActorBase* FDungeonArchitectThemeEditorUtils::CreateVisualNodeUnderNode(UObject* InAssetObject,
                                                                                              const UEdGraphNode_DungeonThemeBase* InParentMarkerNode, int32 InInsertIndex)
{
    if (!InAssetObject || !InParentMarkerNode) {
        return nullptr;
    }

    TArray<UEdGraphNode_DungeonThemeActorBase*> ExistingVisualNodes;
    if (UEdGraphPin* MarkerOutPin = InParentMarkerNode->GetOutputPin()) {
        for (UEdGraphPin* LinkedPin : MarkerOutPin->LinkedTo) {
            if (UEdGraphNode_DungeonThemeActorBase* ActorNode = Cast<UEdGraphNode_DungeonThemeActorBase>(LinkedPin->GetOwningNode())) {
                ExistingVisualNodes.Add(ActorNode);
            }
        } 
    }
    
    UEdGraphNode_DungeonThemeActorBase* SpawnedNode = nullptr;
    if (UEdGraph_DungeonTheme* ThemeGraph = Cast<UEdGraph_DungeonTheme>(InParentMarkerNode->GetGraph())) {
        const FVector2D NodePosition = FVector2D(InParentMarkerNode->NodePosX - 40, InParentMarkerNode->NodePosY + 80);

        if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(InAssetObject)) {
            if (UEdGraphNode_DungeonThemeMesh* MeshNode = ThemeGraph->CreateNewNode<UEdGraphNode_DungeonThemeMesh>(NodePosition)) {
                MeshNode->Mesh = StaticMesh;
                MeshNode->Offset = FTransform::Identity;
                SpawnedNode = MeshNode;
            }
        }
        else if (UDungeonMeshList* MeshList = Cast<UDungeonMeshList>(InAssetObject)) {
            if (UEdGraphNode_DungeonThemeMeshList* MeshListNode = ThemeGraph->CreateNewNode<UEdGraphNode_DungeonThemeMeshList>(NodePosition)) {
                MeshListNode->Meshes = MeshList->StaticMeshes;
                MeshListNode->Blueprints = MeshList->ActorTemplates;
                SpawnedNode = MeshListNode;
            }
        }
        else if (UClass* BlueprintClass = Cast<UClass>(InAssetObject)) {
            if (UEdGraphNode_DungeonThemeActorTemplate* ActorNode = ThemeGraph->CreateNewNode<UEdGraphNode_DungeonThemeActorTemplate>(NodePosition)) {
                ActorNode->ClassTemplate = BlueprintClass;
                ActorNode->Offset = FTransform::Identity;
                SpawnedNode = ActorNode;
            }
        }
        
        if (SpawnedNode) {
            ExistingVisualNodes.Sort([](UEdGraphNode_DungeonThemeActorBase& A, UEdGraphNode_DungeonThemeActorBase& B) {
                return A.ExecutionOrder < B.ExecutionOrder;
            });

            float NodeX = InParentMarkerNode->NodePosX;
            for (UEdGraphNode_DungeonThemeActorBase* ExistingVisualNode : ExistingVisualNodes) {
                if (ExistingVisualNode) {
                    NodeX = FMath::Min(NodeX, ExistingVisualNode->NodePosX - 160);
                }
            }
            SpawnedNode->NodePosX = NodeX;
            
            if (ExistingVisualNodes.IsValidIndex(InInsertIndex)) {
                ExistingVisualNodes.Insert(SpawnedNode, InInsertIndex);
            }
            else {
                ExistingVisualNodes.Add(SpawnedNode);
            }
            
            // Connect the newly spawned node.
            const UEdGraphSchema* Schema = ThemeGraph->GetSchema();
            Schema->TryCreateConnection(InParentMarkerNode->GetOutputPin(), SpawnedNode->GetInputPin());
            
            for (int i = 0; i < ExistingVisualNodes.Num(); i++) {
                ExistingVisualNodes[i]->ExecutionOrder = i;
            }
        }
    }
	
    return SpawnedNode;
}

AActor* FDungeonArchitectThemeEditorUtils::FindActorSpawnedByMarker(int32 InMarkerId, const ADungeon* InDungeon, const UEdGraphNode_DungeonThemeActorBase* InVisualNode) {
    if (!InVisualNode || !InDungeon) {
        return nullptr;
    }
    
    const FName NewNodeTag = FDungeonThemeEngineUtils::CreateNodeTagFromId(InVisualNode->NodeGuid);
    return FindActorSpawnedByMarker(InMarkerId, InDungeon, NewNodeTag);
}

AActor* FDungeonArchitectThemeEditorUtils::FindActorSpawnedByMarker(int32 InMarkerId, const ADungeon* InDungeon, const FName& InNodeTag) {
    if (!InDungeon) {
        return nullptr;
    }

    const FName DungeonTag = FDungeonUtils::GetDungeonIdTag(InDungeon);
    if (UDungeonModel* DungeonModel = InDungeon->GetModel()) {
        const TArray<FDungeonMarkerInstance>& WorldMarkers = DungeonModel->WorldMarkers;
        for (TActorIterator<AActor> It(InDungeon->GetWorld()); It; ++It) {
            if (AActor* Actor = *It) {
                // Check if this mesh was spawned my the given theme node and dungeon
                if (Actor->Tags.Contains(InNodeTag) && Actor->Tags.Contains(DungeonTag)) {
                    // Check if this was spawned by our marker
                    if (UDungeonSpawnDataComponent* SpawnData = Actor->FindComponentByClass<UDungeonSpawnDataComponent>()) {
                        if (WorldMarkers.IsValidIndex(SpawnData->MarkerIndex) && WorldMarkers[SpawnData->MarkerIndex].Id == InMarkerId) {
                            return Actor;
                        }
                    }
                }
            }
        }
    }
    return nullptr;
}

TSharedPtr<FDungeonArchitectThemeEditor> FDungeonArchitectThemeEditorUtils::GetThemeEditorForAsset(const UEdGraph* Graph) {
    if (!Graph) {
        return nullptr;
    }
    
    UDungeonThemeAsset* ThemeAsset = Cast<UDungeonThemeAsset>(Graph->GetOuter());
    TSharedPtr<FDungeonArchitectThemeEditor> ThemeEditor;
    if (ThemeAsset) {
        TSharedPtr< IToolkit > FoundAssetEditor = FToolkitManager::Get().FindEditorForAsset(ThemeAsset);
        if (FoundAssetEditor.IsValid()) {
            ThemeEditor = StaticCastSharedPtr<FDungeonArchitectThemeEditor>(FoundAssetEditor);
        }
    }
    return ThemeEditor;
}

void FDungeonArchitectThemeEditorUtils::FindParentMarkersRecursive(UEdGraphNode* InNode, TSet<UEdGraphNode*>& Visited, TArray<UEdGraphNode_DungeonThemeMarker*>& OutMarkerNodes) {
    if (!InNode || Visited.Contains(InNode)) {
        return;
    }
    Visited.Add(InNode);

    if (UEdGraphNode_DungeonThemeMarker* MarkerNode = Cast<UEdGraphNode_DungeonThemeMarker>(InNode)) {
        OutMarkerNodes.Add(MarkerNode);
        return;
    }
    if (UEdGraphNode_DungeonThemeBase* ThemeNode = Cast<UEdGraphNode_DungeonThemeBase>(InNode)) {
        if (UEdGraphPin* InputPin = ThemeNode->GetInputPin()) {
            for (UEdGraphPin* IncomingNodePin : InputPin->LinkedTo) {
                if (UEdGraphNode* IncomingNode = IncomingNodePin ? IncomingNodePin->GetOwningNode() : nullptr) {
                    FindParentMarkersRecursive(IncomingNode, Visited, OutMarkerNodes);
                }
            }
        }   
    }
}
    
TArray<UEdGraphNode_DungeonThemeMarker*> FDungeonArchitectThemeEditorUtils::FindParentMarkers(UEdGraphNode* InNode) {
    TArray<UEdGraphNode_DungeonThemeMarker*> MarkerNodes;
    TSet<UEdGraphNode*> Visited;
    FindParentMarkersRecursive(InNode, Visited, MarkerNodes);
    return MarkerNodes;
}

UEdGraphNode_DungeonThemeBase* FDungeonArchitectThemeEditorUtils::GetParentNode(UEdGraphNode* GraphNode) {
    UEdGraphNode_DungeonThemeBase* StateNode = CastChecked<UEdGraphNode_DungeonThemeBase>(GraphNode);
    UEdGraphPin* MyInputPin = StateNode->GetInputPin();
    UEdGraphPin* MyParentOutputPin = nullptr;
    if (MyInputPin != nullptr && MyInputPin->LinkedTo.Num() > 0) {
        MyParentOutputPin = MyInputPin->LinkedTo[0];
        if (MyParentOutputPin != nullptr) {
            if (MyParentOutputPin->GetOwningNode() != nullptr) {
                return CastChecked<UEdGraphNode_DungeonThemeBase>(MyParentOutputPin->GetOwningNode());
            }
        }
    }

    return nullptr;
}

UEdGraphNode_DungeonThemeMarker* FDungeonArchitectThemeEditorUtils::FindParentMarker(UEdGraphNode* InNode) {
    TArray<UEdGraphNode_DungeonThemeMarker*> Markers = FindParentMarkers(InNode);
    return Markers.Num() > 0 ? Markers[0] : nullptr;
}

#undef LOCTEXT_NAMESPACE

