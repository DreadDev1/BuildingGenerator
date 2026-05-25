//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport2D/FlowDomainEdLayoutGraph2D.h"

#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport2D/FlowDomainEdViewport2D.h"
#include "Core/Editors/FlowEditor/DomainEditors/Common/Viewport2D/FlowDomainEdViewportClient2D.h"

#include "AssetEditorModeManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "PreviewScene.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FlowDomainEdAbstractGraph2D"
DEFINE_LOG_CATEGORY_STATIC(LogAbstractGraphEd2D, Log, All);



//////////////////////////////// FFlowDomainEdAbstractGraph3D ////////////////////////////////
void FFlowDomainEdLayoutGraph2D::InitializeImpl(const FDomainEdInitSettings& InSettings) {
    // Create the preview scene
    {
        FPreviewScene::ConstructionValues CVS;
        CVS.bCreatePhysicsScene = false;
        PreviewScene = MakeUnique<FPreviewScene>(CVS);
        const AStaticMeshActor* TestMeshActor = PreviewScene->GetWorld()->SpawnActor<AStaticMeshActor>();
        UStaticMesh* TestMesh = Cast<UStaticMesh>(FSoftObjectPath(TEXT("StaticMesh'/Engine/EditorMeshes/EditorSphere.EditorSphere'")).TryLoad());
        TestMeshActor->GetStaticMeshComponent()->SetStaticMesh(TestMesh);
    }
    
    TSharedPtr<FFlowDomainEdViewportClient2D> EditorViewportClient = CreateViewportClient(InSettings);
    check(InSettings.EditorModeManager.IsValid());
    StaticCastSharedPtr<FAssetEditorModeManager>(InSettings.EditorModeManager)->SetPreviewScene(EditorViewportClient->GetPreviewScene());
    
    Viewport = SNew(SFlowDomainEdViewport2D, EditorViewportClient);
    Viewport->GetViewportClient()->SetViewMode(VMI_Unlit);
    
    ContentWidget = SNew(SOverlay)
    // The graph panel
    +SOverlay::Slot()
    [
        Viewport.ToSharedRef()
    ]
    +SOverlay::Slot()
    .Padding(10)
    .VAlign(VAlign_Bottom)
    .HAlign(HAlign_Right)
    [
        SNew(STextBlock)
        .Visibility( EVisibility::HitTestInvisible )
        .TextStyle( FAppStyle::Get(), "Graph.CornerText" )
        .Text( LOCTEXT("CornerText", "Layout Graph") )
    ];
}

FFlowDomainEditorTabInfo FFlowDomainEdLayoutGraph2D::GetTabInfo() const {
    static const FFlowDomainEditorTabInfo TabInfo = {
        TEXT("LayoutGraph2DTab"),
        LOCTEXT("LayoutGraph2DTabLabel", "Layout 2D"),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details")
    };

    return TabInfo;
}

TSharedRef<SWidget> FFlowDomainEdLayoutGraph2D::GetContentWidget() {
    return ContentWidget.ToSharedRef();
}

void FFlowDomainEdLayoutGraph2D::Build(const FFlowExecNodeStatePtr& State) {
    if (!State.IsValid()) return;
    
    
}

void FFlowDomainEdLayoutGraph2D::RecenterView(const FFlowExecNodeStatePtr& State) {
    
}

void FFlowDomainEdLayoutGraph2D::Tick(float DeltaTime) {
    
}

void FFlowDomainEdLayoutGraph2D::AddReferencedObjects(FReferenceCollector& Collector) {
    
}

FString FFlowDomainEdLayoutGraph2D::GetReferencerName() const {
    static const FString NameString = TEXT("FFlowDomainEdAbstractGraph2D");
    return NameString;
}

#undef LOCTEXT_NAMESPACE

