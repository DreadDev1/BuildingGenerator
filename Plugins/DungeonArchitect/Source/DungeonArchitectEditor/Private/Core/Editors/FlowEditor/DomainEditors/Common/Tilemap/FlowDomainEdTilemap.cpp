//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Common/Tilemap/FlowDomainEdTilemap.h"

#include "Core/Editors/FlowEditor/FlowEditorUtils.h"
#include "Frameworks/Flow/Domains/Tilemap/FlowTilemapRenderer.h"
#include "Frameworks/Flow/Domains/Tilemap/Graph/TilemapGraphInfrastructure.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTask.h"

#include "Engine/TextureRenderTarget2D.h"
#include "IDetailsView.h"

#define LOCTEXT_NAMESPACE "FlowDomainEdAbstractGraph"
DEFINE_LOG_CATEGORY_STATIC(LogDomainEdTilemap, Log, All);

void FFlowDomainEdTilemap::InitializeImpl(const FDomainEdInitSettings& InSettings) {
    TilemapGraph = NewObject<UFlowTilemapEdGraph>();
    TilemapGraph->Initialize();
    TilemapGraph->OnCellClicked.BindRaw(this, &FFlowDomainEdTilemap::OnPreviewTilemapCellClicked);
    TilemapGraph->OnItemWidgetClicked.BindRaw(this, &FFlowDomainEdTilemap::OnItemWidgetClicked);

    // Create the appearance info
    FGraphAppearanceInfo AppearanceInfo;
    AppearanceInfo.CornerText = LOCTEXT("FlowTilemapGraphBranding", "Tilemap");
    TilemapGraphHandler = MakeShareable(new FFlowTilemapGraphHandler);
    TilemapGraphHandler->Bind();
    TilemapGraphHandler->SetPropertyEditor(InSettings.PropertyEditor);

    TilemapGraphEditor = SNew(SGraphEditor)
        .AdditionalCommands(TilemapGraphHandler->GraphEditorCommands)
        .Appearance(AppearanceInfo)
        .GraphToEdit(TilemapGraph)
        .IsEditable(true)
        .ShowGraphStateOverlay(false)
        .GraphEvents(TilemapGraphHandler->GraphEvents);

    TilemapGraphHandler->SetGraphEditor(TilemapGraphEditor);
}

void FFlowDomainEdTilemap::OnPreviewTilemapCellClicked(const FIntPoint& InTileCoords, bool bDoubleClicked) {
    bool bRequestSelection = false;
    FVector RequestedChunkCoord;

    TSharedPtr<FFlowExecNodeState> State = PreviewState.Pin();
    UFlowTilemap* Tilemap = State.IsValid() ? State->GetState<UFlowTilemap>(UFlowTilemap::StateTypeID) : nullptr;
    TSharedPtr<IMediator> MediatorPtr = Mediator.Pin();
    
    if (Tilemap) {
        const FFlowTilemapCell* CellPtr = Tilemap->GetSafe(InTileCoords.X, InTileCoords.Y);
        if (CellPtr) {
            const FFlowTilemapCell& Cell = *CellPtr;
            if (Cell.bLayoutCell) {
                RequestedChunkCoord = Cell.ChunkCoord;
                bRequestSelection = true;
            }
            else {
                bRequestSelection = false;
            }
        }

        bool bRequiresUpdate = true;

        if (!bChunkSelected && !bRequestSelection) {
            bRequiresUpdate = false;
        }

        if (bChunkSelected == bRequestSelection && SelectedChunkCoord.Equals(RequestedChunkCoord)) {
            bRequiresUpdate = false;
        }

        if (bRequiresUpdate) {
            if (MediatorPtr.IsValid()) {
                MediatorPtr->UpdateTilemapChunkSelection(RequestedChunkCoord);
            }
            
        }
    }

    SelectItem(FGuid());

    if (MediatorPtr.IsValid()) {
        MediatorPtr->OnTilemapCellClicked(InTileCoords, bDoubleClicked);
    }

    if (bDoubleClicked) {
        FocusOnTileCoord(InTileCoords);
    }
}

void FFlowDomainEdTilemap::SaveThumbnail(const struct FAssetData& InAsset, int32 ThumbSize) {
    
    UTexture* TilemapTexture = TilemapGraph ? TilemapGraph->GetPreviewTexture() : nullptr;
    if (!TilemapTexture) return;

    UTextureRenderTarget2D* RTT = Cast<UTextureRenderTarget2D>(TilemapTexture);
    FFlowEditorUtils::SaveTextureAsAssetThumbnail(InAsset, ThumbSize, RTT);
}

void FFlowDomainEdTilemap::FocusOnTileCoord(const FIntPoint& InTileCoords) {
    TSharedPtr<FFlowExecNodeState> State = PreviewState.Pin();
    const UFlowTilemap* Tilemap = State.IsValid() ? State->GetState<UFlowTilemap>(UFlowTilemap::StateTypeID) : nullptr;
    
    if (Tilemap) {
        float HeightCoord = 0;
        const FFlowTilemapCell* CellPtr = Tilemap->GetSafe(InTileCoords.X, InTileCoords.Y);
        if (CellPtr) {
            HeightCoord = CellPtr->Height;
            const FVector Coord(InTileCoords.X, InTileCoords.Y, HeightCoord);
            TSharedPtr<IMediator> MediatorPtr = Mediator.Pin();
            if (MediatorPtr.IsValid()) {
                MediatorPtr->FocusViewportOnTileCoord(Coord);
            }
        }
    }
}

void FFlowDomainEdTilemap::Build(const FFlowExecNodeStatePtr& State) {
    PreviewState = State;
    UFlowTilemap* Tilemap = State->GetState<UFlowTilemap>(UFlowTilemap::StateTypeID);
    if (TilemapGraph) {
        TArray<UFlowGraphItem*> Items;
        TSharedPtr<IMediator> MediatorPtr = Mediator.Pin();
        if (MediatorPtr.IsValid()) {
            MediatorPtr->GetAllTilemapItems(State, Items);
        }

        FFlowTilemapRendererSettings RenderSettings;
        RenderSettings.bUseTextureTileSize = false;
        RenderSettings.TileSize = 10;
        RenderSettings.BackgroundColor = FLinearColor::Black;
        RenderSettings.FuncCellSelected =
            [this](const FVector& InChunkCoord) -> bool {
                return IsTileCellSelected(InChunkCoord);
        };

        TilemapGraph->GeneratePreviewTexture(Tilemap, RenderSettings, Items);
    }
}

bool FFlowDomainEdTilemap::IsTileCellSelected(const FVector& InChunkCoord) const {
    return bChunkSelected ? InChunkCoord.Equals(SelectedChunkCoord) : false;
}

void FFlowDomainEdTilemap::SelectItem(const FGuid& InItemId) const {
    TilemapGraph->SelectedItemId = InItemId;
}

void FFlowDomainEdTilemap::SelectChunk(const FVector& InChunkCoord, bool bInSelected) {
    SelectedChunkCoord = InChunkCoord;
    bChunkSelected = bInSelected;
}

void FFlowDomainEdTilemap::RefreshPreviewTilemap() {
    const FFlowExecNodeStatePtr State = PreviewState.Pin();
    if (State.IsValid()) {
        Build(State);
    }
}

void FFlowDomainEdTilemap::OnItemWidgetClicked(const FGuid& InItemId, bool bDoubleClicked) {
    SelectItem(InItemId);

    TSharedPtr<IMediator> MediatorPtr = Mediator.Pin();
    if (MediatorPtr.IsValid()) {
        MediatorPtr->OnTilemapItemWidgetClicked(InItemId, bDoubleClicked);
    }
}

FFlowDomainEditorTabInfo FFlowDomainEdTilemap::GetTabInfo() const {
    static const FFlowDomainEditorTabInfo TabInfo = {
        TEXT("TilemapTab"),
        LOCTEXT("TilemapTabLabel", "Tilemap"),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details")
    };

    return TabInfo;
}

TSharedRef<SWidget> FFlowDomainEdTilemap::GetContentWidget() {
    return TilemapGraphEditor.ToSharedRef();
}

void FFlowDomainEdTilemap::AddReferencedObjects(FReferenceCollector& Collector) {
    if (TilemapGraph) {
        Collector.AddReferencedObject(TilemapGraph);
    }
}

FString FFlowDomainEdTilemap::GetReferencerName() const {
    static const FString NameString = TEXT("FFlowDomainEdTilemap");
    return NameString;
}

void FFlowDomainEdTilemap::Tick(float DeltaTime) {
}

#undef LOCTEXT_NAMESPACE

