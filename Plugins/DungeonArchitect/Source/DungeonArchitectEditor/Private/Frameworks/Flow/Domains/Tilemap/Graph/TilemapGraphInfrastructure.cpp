//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/Domains/Tilemap/Graph/TilemapGraphInfrastructure.h"

#include "Frameworks/Flow/Common/Widgets/SFlowItemOverlay.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractItem.h"
#include "Frameworks/Flow/Domains/Tilemap/FlowTilemapRenderer.h"

#include "EdGraph/EdGraph.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Framework/Commands/GenericCommands.h"
#include "IDetailsView.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"

DEFINE_LOG_CATEGORY_STATIC(LogTilemapEdGraph, Log, All);

struct FGFTilemapNodeItemInfo {
    FGuid ItemId;
    TWeakObjectPtr<UFlowGraphItem> Item;
    TSharedPtr<class SFlowItemOverlay> Widget;
    FFlowTilemapCoord TileCoord;
    FVector2D RenderOffset;
};


namespace {
    const int32 DAGF_TileSize = 10; // TODO: Grab from the tilemap
    const int32 DAGF_TilePadding = 10;
}

/////////////////// Graph /////////////////// 
UFlowTilemapEdGraph::UFlowTilemapEdGraph(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    Schema = UFlowTilemapEdGraphSchema::StaticClass();
}

void UFlowTilemapEdGraph::Initialize() {
    FGraphNodeCreator<UFlowTilemapEdGraphNode> NodeCreator(*this);
    PreviewNode = NodeCreator.CreateNode(false);
    NodeCreator.Finalize();
}

void UFlowTilemapEdGraph::GeneratePreviewTexture(UFlowTilemap* InTilemap,
                                                     const FFlowTilemapRendererSettings& InRenderSettings,
                                                     const TArray<UFlowGraphItem*>& InItems) {
    if (!PreviewNode) return;

    // Release the old tilemap Render Texture
    if (UTextureRenderTarget2D* RTT = Cast<UTextureRenderTarget2D>(PreviewNode->PreviewTexture)) {
        UKismetRenderingLibrary::ReleaseRenderTarget2D(RTT);
        PreviewNode->PreviewTexture = nullptr;
    }

    if (InTilemap) {
        PreviewNode->PreviewTexture = FFlowTilemapRenderer::Create(InTilemap, InRenderSettings);

        // Update the items
        TMap<FGuid, FFlowTilemapCoord> TileItems;
        for (const FFlowTilemapCell& Cell : InTilemap->GetCells()) {
            if (Cell.bHasItem) {
                TileItems.Add(Cell.ItemId, FFlowTilemapCoord(Cell.TileCoord));
            }
        }
        for (const FFlowTilemapEdge& Edge : InTilemap->GetEdgesH()) {
            if (Edge.bHasItem) {
                TileItems.Add(Edge.ItemId, Edge.EdgeCoord);
            }
        }
        for (const FFlowTilemapEdge& Edge : InTilemap->GetEdgesV()) {
            if (Edge.bHasItem) {
                TileItems.Add(Edge.ItemId, Edge.EdgeCoord);
            }
        }
        TMap<FGuid, TWeakObjectPtr<UFlowGraphItem>> ItemList;
        for (UFlowGraphItem* Item : InItems) {
            TWeakObjectPtr<UFlowGraphItem>& ItemRef = ItemList.FindOrAdd(Item->ItemId);
            ItemRef = Item;
        }
        PreviewNode->SetItemInfo(TileItems, ItemList);
    }
    else {
        PreviewNode->PreviewTexture = nullptr;
        PreviewNode->ClearItemInfo();
    }

    // Update the preview node's location to align the center with the origin
    UTexture* Texture = PreviewNode->PreviewTexture;
    float Width = 0;
    float Height = 0;
    if (Texture) {
        Width = Texture->GetSurfaceWidth();
        Height = Texture->GetSurfaceHeight();
    }
    PreviewNode->NodePosX = -Width * 0.5f;
    PreviewNode->NodePosY = -Height * 0.5f;

    NotifyGraphChanged();
}

UTexture* UFlowTilemapEdGraph::GetPreviewTexture() {
    return PreviewNode ? PreviewNode->PreviewTexture : nullptr;
}

/////////////////// Graph Schema /////////////////// 
FConnectionDrawingPolicy* UFlowTilemapEdGraphSchema::CreateConnectionDrawingPolicy(
    int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect,
    class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const {
    return new FFlowTilemapConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect,
                                                       InDrawElements, InGraphObj);
}

/////////////////// Link Drawing Policy /////////////////// 
FFlowTilemapConnectionDrawingPolicy::FFlowTilemapConnectionDrawingPolicy(
    int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect,
    FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
    : FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements)
      , GraphObj(InGraphObj) {
}

void FFlowTilemapConnectionDrawingPolicy::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries,
                                                   FArrangedChildren& InArrangedNodes) {
    if (InArrangedNodes.Num() == 0) {
        return;
    }

    FArrangedWidget& CurWidget = InArrangedNodes[0];
    TSharedRef<SFlowTilemapGraphNode> TilemapWidget = StaticCastSharedRef<SFlowTilemapGraphNode>(
        CurWidget.Widget);

    const TMap<FGuid, SFlowTilemapGraphNode::FNodeItemInfo>& ItemInfoList = TilemapWidget->GetItemInfo();

    FVector2D NodePadding(DAGF_TilePadding, DAGF_TilePadding);
    FVector2D Scale = FVector2D(1, 1);
    UFlowTilemapEdGraphNode* TilemapNode = TilemapWidget->GetTilemapNode();
    if (TilemapNode && TilemapNode->PreviewTexture) {
        float Width = TilemapNode->PreviewTexture->GetSurfaceWidth();
        float Height = TilemapNode->PreviewTexture->GetSurfaceHeight();
        FVector2D TextureSize(Width, Height);
        FVector2D DefaultSize = TextureSize + NodePadding * 2;
        FVector2D DrawSize = CurWidget.Geometry.GetDrawSize();
        Scale = DrawSize / DefaultSize;
    }

    for (auto& Entry : ItemInfoList) {
        const FGuid& ItemId = Entry.Key;
        const SFlowTilemapGraphNode::FNodeItemInfo& ItemInfo = Entry.Value;
        if (!ItemInfo.Item.IsValid()) continue;
        
        for (const FGuid& RefItemId : ItemInfo.Item->ReferencedItemIds) {
            const SFlowTilemapGraphNode::FNodeItemInfo* RefItem = ItemInfoList.Find(RefItemId);
            if (RefItem) {
                FVector2D Src = FVector2D(CurWidget.Geometry.AbsolutePosition) + ItemInfo.RenderOffset * Scale;
                FVector2D Dest = FVector2D(CurWidget.Geometry.AbsolutePosition) + RefItem->RenderOffset * Scale;
                FVector2D Direction = Dest - Src;
                Direction.Normalize();
                Dest -= Direction * (RefItem->Widget->GetWidgetRadius() - 1) * Scale;

                FConnectionParams Params;
                Params.WireColor = FLinearColor::Red;
                Params.WireThickness = 1.0f;
                DrawItemReferenceLink(Src, Dest, Params);
            }
        }
    }
}

void FFlowTilemapConnectionDrawingPolicy::DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom,
                                                                  const FConnectionParams& Params) {
    // Get a reasonable seed point (halfway between the boxes)
    FVector2D StartCenter = FGeometryHelper::CenterOf(StartGeom);
    FVector2D EndCenter = FGeometryHelper::CenterOf(EndGeom);

    float StartRadius = StartGeom.GetDrawSize().X * 0.5f;
    float EndRadius = EndGeom.GetDrawSize().X * 0.5f;

    const float FallbackRadius = 32;
    if (StartRadius == 0) {
        StartRadius = FallbackRadius;
        StartCenter += FVector2D(StartRadius, StartRadius);
    }
    if (EndRadius == 0) {
        EndRadius = FallbackRadius;
        EndCenter += FVector2D(EndRadius, EndRadius);
    }

    FVector2D Direction = (EndCenter - StartCenter);
    Direction.Normalize();

    const FVector2D StartAnchorPoint = StartCenter + Direction * StartRadius;
    const FVector2D EndAnchorPoint = EndCenter - Direction * EndRadius;

    DrawSplineWithArrow(StartAnchorPoint, EndAnchorPoint, Params);
}

void FFlowTilemapConnectionDrawingPolicy::DrawSplineWithArrow(const FVector2D& StartPoint,
                                                                  const FVector2D& EndPoint,
                                                                  const FConnectionParams& Params) {
    DrawItemReferenceLink(StartPoint, EndPoint, Params);
}

void FFlowTilemapConnectionDrawingPolicy::DrawConnection(int32 LayerId, const FVector2D& Start,
                                                             const FVector2D& End, const FConnectionParams& Params) {
    const FVector2D& P0 = Start;
    const FVector2D& P1 = End;

    const FVector2D Delta = End - Start;
    const FVector2D NormDelta = Delta.GetSafeNormal();

    const FVector2D P0Tangent = NormDelta;
    const FVector2D P1Tangent = NormDelta;

    // Draw the spline itself
    FSlateDrawElement::MakeDrawSpaceSpline(
        DrawElementsList,
        LayerId,
        P0, P0Tangent,
        P1, P1Tangent,
        //ClippingRect,
        Params.WireThickness,
        ESlateDrawEffect::None,
        Params.WireColor
    );
}

void FFlowTilemapConnectionDrawingPolicy::DrawItemReferenceLink(const FVector2D& Src, const FVector2D& Dest,
                                                                    const FConnectionParams& Params) {
    const FVector2D ArrowHeadRadius = ArrowRadius;
    const FSlateBrush* ArrowHeadImage = ArrowImage;

    const FVector2D StartAnchorPoint = Src;
    const FVector2D EndAnchorPoint = Dest;
    const FVector2D DeltaPos = EndAnchorPoint - StartAnchorPoint;
    const FVector2D UnitDelta = DeltaPos.GetSafeNormal();
    const FVector2D Normal = FVector2D(DeltaPos.Y, -DeltaPos.X).GetSafeNormal();

    // Come up with the final start/end points
    const FVector2D LengthBias = ArrowHeadRadius.X * UnitDelta;

    const FVector2D StartPoint = StartAnchorPoint;
    FVector2D EndPoint = EndAnchorPoint;

    EndPoint -= LengthBias;

    DrawConnection(ArrowLayerID, StartPoint, EndPoint, Params);

    FVector2D ArrowDrawPos = (EndAnchorPoint - LengthBias) - ArrowHeadRadius;
    const float AngleInRadians = FMath::Atan2(DeltaPos.Y, DeltaPos.X);

    // Draw the arrow
    FSlateDrawElement::MakeRotatedBox(
        DrawElementsList,
        ArrowLayerID,
        FPaintGeometry(ArrowDrawPos, ArrowHeadImage->ImageSize * ZoomFactor, ZoomFactor),
        ArrowHeadImage,
        //ClippingRect,
        ESlateDrawEffect::None,
        AngleInRadians,
        TOptional<FVector2D>(),
        FSlateDrawElement::RelativeToElement,
        Params.WireColor
    );
}

/////////////////// Graph Handler ///////////////////

void FFlowTilemapGraphHandler::Bind() {
    GraphEditorCommands = MakeShareable(new FUICommandList);
    GraphEditorCommands->MapAction(FGenericCommands::Get().SelectAll,
                                   FExecuteAction::CreateSP(this, &FFlowTilemapGraphHandler::SelectAllNodes),
                                   FCanExecuteAction::CreateSP(this, &FFlowTilemapGraphHandler::CanSelectAllNodes)
    );

    GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
                                   FExecuteAction::CreateSP(this, &FFlowTilemapGraphHandler::DeleteSelectedNodes),
                                   FCanExecuteAction::CreateSP(this, &FFlowTilemapGraphHandler::CanDeleteNodes)
    );

    GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
                                   FExecuteAction::CreateSP(this, &FFlowTilemapGraphHandler::CopySelectedNodes),
                                   FCanExecuteAction::CreateSP(this, &FFlowTilemapGraphHandler::CanCopyNodes)
    );

    GraphEditorCommands->MapAction(FGenericCommands::Get().Paste,
                                   FExecuteAction::CreateSP(this, &FFlowTilemapGraphHandler::PasteNodes),
                                   FCanExecuteAction::CreateSP(this, &FFlowTilemapGraphHandler::CanPasteNodes)
    );

    GraphEditorCommands->MapAction(FGenericCommands::Get().Cut,
                                   FExecuteAction::CreateSP(this, &FFlowTilemapGraphHandler::CutSelectedNodes),
                                   FCanExecuteAction::CreateSP(this, &FFlowTilemapGraphHandler::CanCutNodes)
    );

    GraphEditorCommands->MapAction(FGenericCommands::Get().Duplicate,
                                   FExecuteAction::CreateSP(this, &FFlowTilemapGraphHandler::DuplicateNodes),
                                   FCanExecuteAction::CreateSP(this, &FFlowTilemapGraphHandler::CanDuplicateNodes)
    );

    GraphEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(
        this, &FFlowTilemapGraphHandler::HandleSelectedNodesChanged);
    GraphEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(
        this, &FFlowTilemapGraphHandler::HandleNodeDoubleClicked);
}

void FFlowTilemapGraphHandler::SetGraphEditor(TSharedPtr<SGraphEditor> InGraphEditor) {
    this->GraphEditor = InGraphEditor;
}

void FFlowTilemapGraphHandler::SetPropertyEditor(TWeakPtr<IDetailsView> InPropertyEditor) {
    PropertyEditor = InPropertyEditor;
}

void FFlowTilemapGraphHandler::SelectAllNodes() {
    if (!GraphEditor.IsValid()) return;
    GraphEditor->SelectAllNodes();
}

bool FFlowTilemapGraphHandler::CanSelectAllNodes() const {
    return GraphEditor.IsValid();
}

void FFlowTilemapGraphHandler::DeleteSelectedNodes() {
}

bool FFlowTilemapGraphHandler::CanDeleteNode(class UEdGraphNode* Node) {
    return false;
}

void FFlowTilemapGraphHandler::DeleteNodes(const TArray<class UEdGraphNode*>& NodesToDelete) {
}

void FFlowTilemapGraphHandler::DeleteSelectedDuplicatableNodes() {
}

bool FFlowTilemapGraphHandler::CanDeleteNodes() const {
    return false;
}

void FFlowTilemapGraphHandler::CopySelectedNodes() {
}

bool FFlowTilemapGraphHandler::CanCopyNodes() const {
    return false;
}

void FFlowTilemapGraphHandler::PasteNodes() {

}

bool FFlowTilemapGraphHandler::CanPasteNodes() const {
    return false;
}

void FFlowTilemapGraphHandler::PasteNodesHere(const FVector2D& Location) {
}

void FFlowTilemapGraphHandler::CutSelectedNodes() {
}

bool FFlowTilemapGraphHandler::CanCutNodes() const {
    return false;
}

void FFlowTilemapGraphHandler::DuplicateNodes() {
}

bool FFlowTilemapGraphHandler::CanDuplicateNodes() const {
    return false;
}

void FFlowTilemapGraphHandler::HandleSelectedNodesChanged(const TSet<class UObject*>& NewSelection) {
    if (PropertyEditor.IsValid()) {
        TSharedPtr<IDetailsView> DetailsView = PropertyEditor.Pin();
        DetailsView->SetObjects(NewSelection.Array());
    }

    OnNodeSelectionChanged.ExecuteIfBound(NewSelection);
}

void FFlowTilemapGraphHandler::HandleNodeDoubleClicked(class UEdGraphNode* Node) {
    OnNodeDoubleClicked.ExecuteIfBound(Node);
}

/////////////////// Node Widget /////////////////// 

void SFlowTilemapGraphNode::Construct(const FArguments& InArgs, UFlowTilemapEdGraphNode* InNode) {
    GraphNode = InNode;
    TilemapNode = InNode;
    UpdateGraphNode();
}

void SFlowTilemapGraphNode::UpdateGraphNode() {
    InputPins.Empty();
    OutputPins.Empty();

    // Reset variables that are going to be exposed, in case we are refreshing an already setup node.
    RightNodeBox.Reset();
    LeftNodeBox.Reset();

    static const FVector2D NodePadding(DAGF_TilePadding, DAGF_TilePadding);

    // Create the item widgets
    {
        ItemInfoList.Reset();

        for (auto& Entry : TilemapNode->ItemList) {
            TWeakObjectPtr<UFlowGraphItem> ItemPtr = Entry.Value;
            if (!ItemPtr.IsValid()) continue;
            UFlowGraphItem* Item = ItemPtr.Get();
            TSharedPtr<SFlowItemOverlay> ItemWidget = SNew(SFlowItemOverlay, Item)
                .Selected(this, &SFlowTilemapGraphNode::IsItemSelected, Item->ItemId);
            ItemWidget->GetOnMousePressed().BindRaw(this, &SFlowTilemapGraphNode::OnItemWidgetClicked);

            FFlowTilemapCoord* ItemCoordPtr = TilemapNode->TileItems.Find(Item->ItemId);
            if (ItemCoordPtr) {
                FFlowTilemapCoord Coord = *ItemCoordPtr;
                FVector2D RenderOffset = NodePadding + FVector2D(Coord.Coord.X, Coord.Coord.Y) * DAGF_TileSize;
                if (!Coord.bIsEdgeCoord) {
                    RenderOffset += FVector2D(0.5f, 0.5f) * DAGF_TileSize;
                }
                else {
                    if (Coord.bHorizontalEdge) {
                        RenderOffset += FVector2D(0.5f, 0) * DAGF_TileSize;
                    }
                    else {
                        RenderOffset += FVector2D(0, 0.5f) * DAGF_TileSize;
                    }
                }

                FNodeItemInfo ItemInfo;
                ItemInfo.ItemId = Item->ItemId;
                ItemInfo.Item = Item;
                ItemInfo.TileCoord = Coord;
                ItemInfo.RenderOffset = RenderOffset;
                ItemInfo.Widget = ItemWidget;
                ItemInfoList.Add(ItemInfo.ItemId, ItemInfo);
            }
        }
    }

    FLinearColor TitleShadowColor(0.6f, 0.6f, 0.6f);
    TextureSize = FVector2D::ZeroVector;
    if (TilemapNode && TilemapNode->PreviewTexture) {
        TextureSize.X = TilemapNode->PreviewTexture->GetSurfaceWidth();
        TextureSize.Y = TilemapNode->PreviewTexture->GetSurfaceHeight();
        TextureBrush.SetResourceObject(TilemapNode->PreviewTexture);
        TextureBrush.ImageSize = TextureSize;
    }
    else {
        TextureBrush.SetResourceObject(nullptr);
    }

    this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
    this->GetOrAddSlot(ENodeZone::Center)
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
    [
        SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Graph.StateNode.Body"))
			.Padding(0)
			.BorderBackgroundColor(this, &SFlowTilemapGraphNode::GetBorderBackgroundColor)
        [
            SNew(SOverlay)
            // THUMBNAIL AREA
            + SOverlay::Slot()
              .HAlign(HAlign_Center)
              .VAlign(VAlign_Center)
              .Padding(10.0f)
            [
                SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("Graph.StateNode.ColorSpill"))
					.BorderBackgroundColor(TitleShadowColor)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
                [
                    SNew(SBox)
						.WidthOverride(TextureSize.X)
						.HeightOverride(TextureSize.Y)
                    [
                        SNew(SImage)
                        .Image(&TextureBrush)
                    ]
                ]
            ]
        ]
    ];
}

TArray<FOverlayWidgetInfo> SFlowTilemapGraphNode::GetOverlayWidgets(
    bool bSelected, const FVector2f& WidgetSize) const {
    TArray<FOverlayWidgetInfo> Overlays;

    for (auto& Entry : ItemInfoList) {
        const FNodeItemInfo& ItemInfo = Entry.Value;
        TSharedPtr<SFlowItemOverlay> Widget = ItemInfo.Widget;
        if (Widget.IsValid()) {
            FVector2D Offset = ItemInfo.RenderOffset - Widget->GetDesiredSize() * 0.5f + FVector2D(2, 3);
            FOverlayWidgetInfo OverlayInfo;
            OverlayInfo.Widget = Widget;
            OverlayInfo.OverlayOffset = Offset;
            Overlays.Add(OverlayInfo);
        }
    }

    return Overlays;
}

FReply SFlowTilemapGraphNode::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
    HandleMouseClick(MyGeometry, MouseEvent, false);
    return SGraphNode::OnMouseButtonDoubleClick(MyGeometry, MouseEvent);
}

FReply SFlowTilemapGraphNode::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry,
                                                           const FPointerEvent& InMouseEvent) {
    HandleMouseClick(InMyGeometry, InMouseEvent, true);
    return SGraphNode::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
}

void SFlowTilemapGraphNode::HandleMouseClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent,
                                                 bool bDoubleClick) {
    UFlowTilemapEdGraph* Graph = Cast<UFlowTilemapEdGraph>(GetNodeObj()->GetGraph());

    if (Graph && Graph->OnCellClicked.IsBound()) {
        FVector2D MousePosition = MouseEvent.GetScreenSpacePosition();
        FVector2D WidgetPosition = MyGeometry.GetAbsolutePosition();
        FVector2D Delta = MousePosition - WidgetPosition;
        Delta /= MyGeometry.Scale;

        Delta -= FVector2D(DAGF_TilePadding, DAGF_TilePadding);
        int32 TileX = FMath::FloorToInt(Delta.X / DAGF_TileSize);
        int32 TileY = FMath::FloorToInt(Delta.Y / DAGF_TileSize);

        Graph->OnCellClicked.Execute(FIntPoint(TileX, TileY), bDoubleClick);
    }
}


void SFlowTilemapGraphNode::OnItemWidgetClicked(const FGuid& InItemId, bool bDoubleClicked) {
    UFlowTilemapEdGraph* Graph = Cast<UFlowTilemapEdGraph>(GetNodeObj()->GetGraph());
    if (Graph) {
        //  Invoke a tile click event, for the tile under this item
        {
            UFlowTilemapEdGraphNode* EdNode = Cast<UFlowTilemapEdGraphNode>(GetNodeObj());
            FFlowTilemapCoord* SearchResult = EdNode->TileItems.Find(InItemId);
            if (SearchResult) {
                FFlowTilemapCoord TileCoord = *SearchResult;

                Graph->OnCellClicked.Execute(TileCoord.Coord, bDoubleClicked);
            }
        }

        Graph->OnItemWidgetClicked.ExecuteIfBound(InItemId, bDoubleClicked);
    }
}

bool SFlowTilemapGraphNode::IsItemSelected(FGuid InItemId) const {
    if (UFlowTilemapEdGraph* Graph = Cast<UFlowTilemapEdGraph>(GetNodeObj()->GetGraph())) {
        return Graph->SelectedItemId == InItemId;
    }
    return false;
}

FSlateColor SFlowTilemapGraphNode::GetBorderBackgroundColor() const {
    return FLinearColor(0.08f, 0.08f, 0.08f);
}

/////////////////// Node Widget Factory /////////////////// 

TSharedPtr<SGraphNode> FFlowTilemapGraphPanelNodeFactory::CreateNode(UEdGraphNode* Node) const {
    if (UFlowTilemapEdGraphNode* TilemapNode = Cast<UFlowTilemapEdGraphNode>(Node)) {
        TSharedPtr<SFlowTilemapGraphNode> SNode = SNew(SFlowTilemapGraphNode, TilemapNode);
        return SNode;
    }

    return nullptr;
}

void UFlowTilemapEdGraphNode::SetItemInfo(const TMap<FGuid, FFlowTilemapCoord>& InTileItems,
                                              const TMap<FGuid, TWeakObjectPtr<UFlowGraphItem>>& InItemList) {
    TileItems = InTileItems;
    ItemList = InItemList;
}

void UFlowTilemapEdGraphNode::ClearItemInfo() {
    TileItems.Reset();
    ItemList.Reset();
}

