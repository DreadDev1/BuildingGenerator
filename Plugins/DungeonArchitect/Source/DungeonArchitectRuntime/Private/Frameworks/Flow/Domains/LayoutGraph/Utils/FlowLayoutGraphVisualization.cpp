//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/Domains/LayoutGraph/Utils/FlowLayoutGraphVisualization.h"

#include "Core/Utils/EditorService/IDungeonEditorService.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraph.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractGraphQuery.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractItem.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Core/FlowAbstractNode.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

#define LOCTEXT_NAMESPACE "FlowDomainAbstractGraphVis"
DEFINE_LOG_CATEGORY_STATIC(LogFDAbstractGraphVis, Log, All);

//////////////////////////////// FFDAbstractNodePreview ////////////////////////////////

FDAbstractNodeVisualizerResources::FDAbstractNodeVisualizerResources()
    : DefaultMaterial(FSoftObjectPath(TEXT("/DungeonArchitect/Core/Runtime/Features/FlowGraph/AbstractGraph3D/Materials/M_AbstractNode.M_AbstractNode")))
    , SelectedMaterial(FSoftObjectPath(TEXT("/DungeonArchitect/Core/Runtime/Features/FlowGraph/AbstractGraph3D/Materials/M_AbstractNode_Selected.M_AbstractNode_Selected")))
    , TextMaterial(FSoftObjectPath(TEXT("/DungeonArchitect/Core/Runtime/Features/FlowGraph/AbstractGraph3D/Materials/M_AbstractGraph_Text.M_AbstractGraph_Text")))
    , DoorNodeMaterial(FSoftObjectPath(TEXT("/DungeonArchitect/Core/Runtime/Features/FlowGraph/AbstractGraph3D/Materials/M_AbstractDoorNode.M_AbstractDoorNode")))
    , BoundsMaterial(FSoftObjectPath(TEXT("/DungeonArchitect/Core/Runtime/Features/FlowGraph/AbstractGraph3D/Materials/M_AbstractGraphNodebounds_Inst.M_AbstractGraphNodebounds_Inst")))
    , PlaneMesh(FSoftObjectPath(TEXT("/DungeonArchitect/Core/Runtime/Features/FlowGraph/AbstractGraph3D/Meshes/NodePlane.NodePlane")))
    , BoundsMesh(FSoftObjectPath(TEXT("/DungeonArchitect/Core/Runtime/Features/FlowGraph/AbstractGraph3D/Meshes/NodeBounds.NodeBounds")))
{    
}

UFDAbstractNodePreview::UFDAbstractNodePreview(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    NodeMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("BackgroundPlaneComponent");
    NodeMeshComponent->bCastStaticShadow = false;
    NodeMeshComponent->bCastDynamicShadow = false;
    NodeMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    NodeMeshComponent->SetupAttachment(this);

    TextRendererComponent = CreateDefaultSubobject<UTextRenderComponent>("TextComponent");
    TextRendererComponent->bCastDynamicShadow = false;
    TextRendererComponent->bCastStaticShadow = false;
    TextRendererComponent->SetupAttachment(this);
    TextRendererComponent->HorizontalAlignment = EHTA_Center;
    TextRendererComponent->VerticalAlignment = EVRTA_TextCenter;
    TextRendererComponent->SetTextRenderColor(FColor(20, 20, 20));
    TextRendererComponent->SetWorldSize(50.0f);
    TextRendererComponent->SetRelativeLocation(FVector(1, 0, 0));
    TextRendererComponent->SetText(FText::FromString(""));

    // Create the bounds mesh to render a merged composite node
    BoundsMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("BoundsMeshComponent");
    BoundsMeshComponent->bCastDynamicShadow = false;
    BoundsMeshComponent->bCastStaticShadow = false;
    BoundsMeshComponent->SetVisibility(false);
    BoundsMeshComponent->SetupAttachment(this);

    InitResources();
}

void UFDAbstractNodePreview::InitResources() {
    static FDAbstractNodeVisualizerResources RenderResources;
    
    if (UStaticMesh* PlaneMesh = RenderResources.PlaneMesh.LoadSynchronous()) {
        NodeMeshComponent->SetStaticMesh(PlaneMesh);
    }

    if (UMaterialInterface* MatDefaultTemplate = RenderResources.DefaultMaterial.LoadSynchronous()) {
        DefaultMaterialInstance = UMaterialInstanceDynamic::Create(MatDefaultTemplate, nullptr);
        DefaultMaterialInstance->SetFlags(RF_Transient);
        NodeMeshComponent->SetMaterial(0, DefaultMaterialInstance);
    }

    if (UMaterialInterface* MatSelectedTemplate = RenderResources.SelectedMaterial.LoadSynchronous()) {
        SelectedMaterialInstance = UMaterialInstanceDynamic::Create(MatSelectedTemplate, nullptr);
        SelectedMaterialInstance->SetFlags(RF_Transient);
    }

    if (UMaterialInterface* MatBoundsTemplate = RenderResources.BoundsMaterial.LoadSynchronous()) {
        BoundsMaterialInstance = UMaterialInstanceDynamic::Create(MatBoundsTemplate, nullptr);
        BoundsMaterialInstance->SetFlags(RF_Transient);
        BoundsMeshComponent->SetMaterial(0, BoundsMaterialInstance);
    }

    if (UMaterialInterface* TextMaterial = RenderResources.TextMaterial.LoadSynchronous()) {
        TextRendererComponent->SetTextMaterial(TextMaterial);
    }

    if (UStaticMesh* BoundsStaticMesh = RenderResources.BoundsMesh.LoadSynchronous()) {
        BoundsMeshComponent->SetStaticMesh(BoundsStaticMesh);
    }
}

void UFDAbstractNodePreview::AlignToCamera(const FVector& InCameraLocation) {
    if (bAlignToCamera) {
        FVector ActorToCam = InCameraLocation - GetComponentLocation();
        ActorToCam.Normalize();
        SetWorldRotation(ActorToCam.Rotation());
        BoundsMeshComponent->SetWorldRotation(FRotator::ZeroRotator);
    }
}

void UFDAbstractNodePreview::SetNodeState(const UFlowAbstractNode* InNode) {
    const AFlowLayoutGraphVisualizer* Visualizer = Cast<AFlowLayoutGraphVisualizer>(GetOwner());
    if (!Visualizer) return;
    
    if (InNode->bActive) {
        SetNodeColor(InNode->Color);
        SetOpacity(1.0f);
        NodeMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        if (InNode->IsA<UFlowAbstractDoorNode>()) {
            static FDAbstractNodeVisualizerResources RenderResources;
            if (UMaterialInterface* MatDefaultTemplate = RenderResources.DoorNodeMaterial.LoadSynchronous()) {
                UMaterialInstanceDynamic* DoorMID = UMaterialInstanceDynamic::Create(MatDefaultTemplate, nullptr);
                DoorMID->SetVectorParameterValue("NodeColor", InNode->Color);
                DoorMID->SetFlags(RF_Transient);
                NodeMeshComponent->SetMaterial(0, DoorMID);
            }
        }
        
        if (InNode->MergedCompositeNodes.Num() > 1) {
            BoundsMeshComponent->SetVisibility(true);
            FBox DesiredBounds(ForceInit);
            for (UFlowAbstractNode* SubNode : InNode->MergedCompositeNodes) {
                const FVector SubNodeLocation = Visualizer->GetNodeLocation(SubNode);
                DesiredBounds += SubNodeLocation;
            }
            const FDAAbstractGraphVisualizerSettings Settings = Visualizer->GetSettings();
            FVector BoundsSize = DesiredBounds.GetExtent() * 2.0f;
            BoundsSize += FVector(Settings.NodeRadius + Settings.LinkPadding) * 2.0f;
            const FVector DesiredScale = BoundsSize / 100.0f;    // The mesh size is 100 units
            BoundsMeshComponent->SetWorldScale3D(DesiredScale);
            if (BoundsMaterialInstance) {
                BoundsMaterialInstance->SetVectorParameterValue("Color", InNode->Color);
            }
        }
        
    }
    else {
        const FDAAbstractGraphVisualizerSettings Settings = Visualizer->GetSettings();
        SetOpacity(Settings.DisabledNodeOpacity);
        NodeMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        bActiveNode = false;
    }
}

void UFDAbstractNodePreview::SetItemState(const UFlowGraphItem* InItem) const {
    SetOpacity(1.0f);
    SetNodeColor(FFlowItemVisuals::GetBackgroundColor(InItem));
    
    TextRendererComponent->SetTextRenderColor(FFlowItemVisuals::GetTextColor(InItem).ToFColor(false));
    TextRendererComponent->SetText(FText::FromString(FFlowItemVisuals::GetText(InItem)));
    NodeMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetBorderSize(0.195f);
}

auto UFDAbstractNodePreview::SetNodeColor(const FLinearColor& InColor) const -> void {
    if (DefaultMaterialInstance) {
        DefaultMaterialInstance->SetVectorParameterValue("NodeColor", InColor);
    }
    if (SelectedMaterialInstance) {
        SelectedMaterialInstance->SetVectorParameterValue("NodeColor", InColor);
    }
}

void UFDAbstractNodePreview::SetOpacity(float InOpacity) const {
    if (DefaultMaterialInstance) {
        DefaultMaterialInstance->SetScalarParameterValue("MasterOpacity", InOpacity);
    }
    if (SelectedMaterialInstance) {
        SelectedMaterialInstance->SetScalarParameterValue("MasterOpacity", InOpacity);
    }
}

void UFDAbstractNodePreview::SetBorderSize(float InSize) const {
    if (DefaultMaterialInstance) {
        DefaultMaterialInstance->SetScalarParameterValue("BorderSize", InSize);
    }
    if (SelectedMaterialInstance) {
        SelectedMaterialInstance->SetScalarParameterValue("BorderSize", InSize);
    }
}

void UFDAbstractNodePreview::SetSelected(bool bInSelected) {
    bSelected = bInSelected;
    UMaterialInterface* Material = bSelected ? SelectedMaterialInstance : DefaultMaterialInstance;
    if (!Material) {
        Material = UMaterial::GetDefaultMaterial(MD_Surface);
    }
    NodeMeshComponent->SetMaterial(0, Material);
}

/////////////////////////////////////// UFDAbstractLink ///////////////////////////////////////

const float UFDAbstractLinkPreview::MeshSize = 100;

FDAbstractLinkVisualizerResources::FDAbstractLinkVisualizerResources()
    : LineMaterial(FSoftObjectPath(TEXT("/DungeonArchitect/Core/Runtime/Features/FlowGraph/AbstractGraph3D/Materials/M_AbstractGraphLink_Inst.M_AbstractGraphLink_Inst")))
    , HeadMaterial(FSoftObjectPath(TEXT("/DungeonArchitect/Core/Runtime/Features/FlowGraph/AbstractGraph3D/Materials/M_AbstractLink_ArrowHead_Inst.M_AbstractLink_ArrowHead_Inst")))
    , LinkMesh(FSoftObjectPath(TEXT("/DungeonArchitect/Core/Runtime/Features/FlowGraph/AbstractGraph3D/Meshes/NodeLink.NodeLink")))
{    
}

UFDAbstractLinkPreview::UFDAbstractLinkPreview(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    LineMesh = CreateDefaultSubobject<UStaticMeshComponent>("LineMesh");
    LineMesh->bCastDynamicShadow = false;
    LineMesh->bCastStaticShadow = false;
    LineMesh->SetCastShadow(false);
    LineMesh->SetupAttachment(this);
    
    InitResources();
}

void UFDAbstractLinkPreview::InitResources() {
    static FDAbstractLinkVisualizerResources RenderResources;
    
    if (UMaterialInterface* LineMat = RenderResources.LineMaterial.LoadSynchronous()) {
        LineMaterialInstance = UMaterialInstanceDynamic::Create(LineMat, nullptr);
        LineMaterialInstance->SetFlags(RF_Transient);
    }
    
    if (UMaterialInterface* HeadMat = RenderResources.HeadMaterial.LoadSynchronous()) {
        HeadMaterialInstance = UMaterialInstanceDynamic::Create(HeadMat, nullptr);
        HeadMaterialInstance->SetFlags(RF_Transient);
    }
    
    if (UStaticMesh* LinkMesh = RenderResources.LinkMesh.LoadSynchronous()) {
        LineMesh->SetStaticMesh(LinkMesh);
        if (LineMaterialInstance) {
            LineMesh->SetMaterial(0, LineMaterialInstance);
        }
    }
}

void UFDAbstractLinkPreview::SetState(const FVector& InStart, const FVector& InEnd, float InThickness, const FLinearColor& InColor, int32 InNumHeads) {
    StartLocation = InStart;
    EndLocation = InEnd;
    Thickness = InThickness;
    NumHeads = InNumHeads;
    Color = InColor;
    
    // Set the location
    SetRelativeLocation(InStart);
    

    // Set the scale
    const FVector StartToEnd = EndLocation - StartLocation;
    float Distance = StartToEnd.Size();
    const FVector Direction = StartToEnd / Distance;
    const FRotator Rotation = Direction.Rotation();

    if (InNumHeads > 0) {
        const float HeadSizeMultiplier = 4;
        const float HeadSize = Thickness * HeadSizeMultiplier;
        Distance -= HeadSize * InNumHeads;
    
        const FVector HeadStart = StartLocation + Direction * Distance;
        const FVector HeadEnd = EndLocation;
        const float HeadThickness = HeadSize;

        // Draw the head
        if (!HeadComponent.IsValid()) {
            HeadComponent = NewObject<UFDAbstractLinkPreview>(GetOwner());
            HeadComponent->SetupAttachment(GetOwner()->GetRootComponent());
            HeadComponent->RegisterComponent();
        }
        HeadComponent->SetState(HeadStart, HeadEnd, HeadThickness, Color);
        HeadComponent->SetWorldRotation(Rotation);
        HeadComponent->SetLinkColor(InColor);
        HeadComponent->UseHeadMaterial(InNumHeads);
    }

    // Setup the actor transform
    {
        FVector Scale = GetComponentScale();
        Scale.X = Distance / MeshSize;
        Scale.Y = Thickness / MeshSize;
        SetWorldScale3D(Scale);
        SetWorldRotation(Rotation);
    }
    SetLinkColor(InColor);
}

void UFDAbstractLinkPreview::SetLinkVisibility(bool bInVisible) {
    SetVisibility(bInVisible, true);
    LineMesh->SetVisibility(bInVisible, true);
}

void UFDAbstractLinkPreview::UseHeadMaterial(int32 InNumHeads) const {
    LineMesh->SetMaterial(0, HeadMaterialInstance);
    if (HeadMaterialInstance) {
        HeadMaterialInstance->SetScalarParameterValue("NumHeads", InNumHeads);
    }
}

void UFDAbstractLinkPreview::SetDynamicAlignment(USceneComponent* Start, USceneComponent* End) {
    bDynamicUpdate = true;
    DynamicCompStart = Start;
    DynamicCompEnd = End;
}

void UFDAbstractLinkPreview::SetLinkColor(const FLinearColor& InColor) const {
    if (LineMaterialInstance) {
        LineMaterialInstance->SetVectorParameterValue("Color", InColor);
    }
    if (HeadMaterialInstance) {
        HeadMaterialInstance->SetVectorParameterValue("Color", InColor);
    }
}

void UFDAbstractLinkPreview::AlignToCamera(const FVector& InCameraLocation, const FDAAbstractGraphVisualizerSettings& InSettings) {
    const FTransform ActorTransform = GetOwner() ? GetOwner()->GetTransform() : FTransform::Identity;
    
    if (bDynamicUpdate && DynamicCompStart.IsValid() && DynamicCompEnd.IsValid()) {
        const FTransform InvTransform = ActorTransform.Inverse();
        const FVector Start = InvTransform.TransformPosition(DynamicCompStart->GetComponentLocation());
        const FVector End = InvTransform.TransformPosition(DynamicCompEnd->GetComponentLocation());
        FVector Direction = End - Start;
        Direction.Normalize();

        const float ItemRadius = InSettings.NodeRadius * InSettings.ItemScale; 
        const FVector LineStart = Start + Direction * ItemRadius;
        const FVector LineEnd = End - Direction * ItemRadius;
        SetState(LineStart, LineEnd, Thickness, Color, NumHeads);
    }

    const FVector WorldStart = ActorTransform.TransformPosition(StartLocation);
    const FVector WorldEnd = ActorTransform.TransformPosition(EndLocation);
    FVector AxisX = WorldEnd - WorldStart;
    AxisX.Normalize();

    FVector AxisZ = InCameraLocation - WorldStart;
    AxisZ.Normalize();

    const FVector AxisY = FVector::CrossProduct(AxisZ, AxisX);
    AxisZ = FVector::CrossProduct(AxisX, AxisY);

    const FMatrix TransformMat(AxisX, AxisY, AxisZ, FVector::ZeroVector);
    SetWorldRotation(TransformMat.Rotator());

    if (HeadComponent.IsValid()) {
        HeadComponent->AlignToCamera(InCameraLocation, InSettings);
    }
}

///////////////////////////////////// AFlowLayoutGraphVisualizer /////////////////////////////////////

AFlowLayoutGraphVisualizer::AFlowLayoutGraphVisualizer(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>("Root");
    RootComponent = SceneRoot;
}


void AFlowLayoutGraphVisualizer::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);

    if (bAutoAlignToLevelViewport) {
        TSharedPtr<IDungeonEditorService> EditorService = IDungeonEditorService::Get();
        if (EditorService.IsValid()) {
            FVector CameraLocation;
            FRotator CameraRotation;
            if (EditorService->GetLevelViewportCameraInfo(GetWorld(), CameraLocation, CameraRotation)) {
                AlignToCamera(CameraLocation);
            }
        }
    }
}

void AFlowLayoutGraphVisualizer::AlignToCamera(const FVector& InCameraLocation) const {
    for (UActorComponent* Component : GetComponents()) {
        if (UFDAbstractNodePreview* NodePreview = Cast<UFDAbstractNodePreview>(Component)) {
            NodePreview->AlignToCamera(InCameraLocation);
        }
        if (UFDAbstractLinkPreview* LinkPreview = Cast<UFDAbstractLinkPreview>(Component)) {
            LinkPreview->AlignToCamera(InCameraLocation, Settings);
        }
    }
}

FVector AFlowLayoutGraphVisualizer::GetNodeLocation(const UFlowAbstractNode* InNode) const {
    return InNode->Coord * Settings.NodeSeparationDistance;
}

void AFlowLayoutGraphVisualizer::Generate(UFlowAbstractGraphBase* InGraph, const FDAAbstractGraphVisualizerSettings& InSettings) {
    Settings = InSettings;
    
    // Clear out the existing scene
    {
        TArray<UActorComponent*> ActorComponents;
        GetComponents(ActorComponents);
        for (UActorComponent* Component : ActorComponents) {
            if (Component == SceneRoot) continue;
            Component->DestroyComponent();
        }
    }
    
    if (!InGraph) {
        return;
    }

    float NodeBaseScale = (Settings.NodeRadius + Settings.LinkPadding) / 50.0f;
    TMap<FGuid, UFDAbstractNodePreview*> ItemPreviewMap;
    // Build the graph nodes
    for (const UFlowAbstractNode* Node : InGraph->GraphNodes) {
        UFDAbstractNodePreview* NodePreview = NewObject<UFDAbstractNodePreview>(this);
        NodePreview->SetRelativeLocation(GetNodeLocation(Node));
        {
            float NodeScale = NodeBaseScale;
            if (!Node->bActive) {
                NodeScale *= Settings.DisabledNodeScale;
            }
            NodePreview->SetRelativeScale3D(FVector(NodeScale));
        }
        NodePreview->SetNodeState(Node);
        NodePreview->SetupAttachment(RootComponent);

        // Create the items
        {
            const float ItemRadius = Settings.NodeRadius * Settings.ItemScale;
            int32 NumItems = Node->NodeItems.Num();
            for (int i = 0; i < NumItems; i++) {
                float Angle = (2 * PI * i) / NumItems + PI * 0.25f;
                float Offset = (Settings.NodeRadius - ItemRadius) / NodeBaseScale;
                float RY = 0, RZ = 0;
                FMath::SinCos(&RZ, &RY, Angle);
                FVector Location = FVector(5, RY * Offset, RZ * Offset);
                const UFlowGraphItem* Item = Node->NodeItems[i];
                UFDAbstractNodePreview* ItemPreview = NewObject<UFDAbstractNodePreview>(this);
                ItemPreview->SetItemState(Item);
                ItemPreview->SetRelativeLocation(Location);
                ItemPreview->SetRelativeScale3D(FVector(Settings.ItemScale));
                ItemPreview->SetAlignToCameraEnabled(false);
                ItemPreview->SetupAttachment(NodePreview);

                UFDAbstractNodePreview*& ItemActorRef = ItemPreviewMap.FindOrAdd(Item->ItemId);
                ItemActorRef = ItemPreview;
            }
        }

        if (Node->MergedCompositeNodes.Num() > 1) {
            for (const UFlowAbstractNode* CompositeNode : Node->MergedCompositeNodes) {
                UFDAbstractNodePreview* CompositeNodePreview = NewObject<UFDAbstractNodePreview>(this);
                CompositeNodePreview->SetRelativeLocation(GetNodeLocation(CompositeNode));
                CompositeNodePreview->SetRelativeScale3D(FVector(NodeBaseScale * 0.25f));
                CompositeNodePreview->SetupAttachment(RootComponent);
                
                //CompositeNodePreview->SetNodeColor(BaseCompositeNode->Color);
                CompositeNodePreview->SetNodeColor(FLinearColor::Black);
                CompositeNodePreview->SetOpacity(0.75f);
                CompositeNodePreview->NodeMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                CompositeNodePreview->TextRendererComponent->SetText(FText::FromString(""));
            }
        }
    }
    
    // Build the graph links
    FFlowAbstractGraphQuery GraphQuery(InGraph);
    for (const UFlowAbstractLink* Link : InGraph->GraphLinks) {
        if (!Link) continue;
        
        UFlowAbstractNode* Source = GraphQuery.GetNode(Link->Source);
        UFlowAbstractNode* Dest = GraphQuery.GetNode(Link->Destination);

        if (Link->Type != EFlowAbstractLinkType::Unconnected) {
            if (Link->SourceSubNode.IsValid()) {
                for (UFlowAbstractNode* SubNode : Source->MergedCompositeNodes) {
                    if (SubNode->NodeId == Link->SourceSubNode) {
                        Source = SubNode;
                        break;
                    }
                }
            }
            if (Link->DestinationSubNode.IsValid()) {
                for (UFlowAbstractNode* SubNode : Dest->MergedCompositeNodes) {
                    if (SubNode->NodeId == Link->DestinationSubNode) {
                        Dest = SubNode;
                        break;
                    }
                }
            }
        }
        
        if (Source && Dest) {
            UFDAbstractLinkPreview* PreviewLink = NewObject<UFDAbstractLinkPreview>(this);
            FVector NodeStart = GetNodeLocation(Source);
            FVector NodeEnd = GetNodeLocation(Dest);
            PreviewLink->SetupAttachment(RootComponent);
            
            const float NodeRadius = Settings.NodeRadius;
            FVector Direction = NodeEnd - NodeStart;
            const float Distance = Direction.Size();
            Direction /= Distance;

            const FVector Start = NodeStart + (NodeRadius + Settings.LinkPadding) * Direction;
            const FVector End = NodeStart + (Distance - NodeRadius - Settings.LinkPadding) * Direction;

            if (Link->Type != EFlowAbstractLinkType::Unconnected) {
                int32 NumHeads = (Link->Type == EFlowAbstractLinkType::OneWay) ? 2 : 1;
                FLinearColor Color = FLinearColor::Black;
                if (Link->Type == EFlowAbstractLinkType::OneWay) {
                    Color = Settings.OneWayLinkColor;
                }
                PreviewLink->SetState(Start, End, Settings.LinkThickness, Color, NumHeads);
                
                // Create the items
                {
                    const float ItemRadius = Settings.NodeRadius * Settings.ItemScale;
                    int32 NumItems = Link->LinkItems.Num();
                    for (int i = 0; i < NumItems; i++) {
                        FVector Offset = FVector::ZeroVector;
                        {
                            float Angle = (2 * PI * i) / NumItems + PI * 0.5f;
                            const float ItemDistance = Settings.NodeRadius - ItemRadius;
                            FMath::SinCos(&Offset.Z, &Offset.Y, Angle);
                            Offset *= ItemDistance;
                            FRotator LineRotation = (End - Start).Rotation();
                            Offset = LineRotation.RotateVector(Offset);
                        }
                        const FVector BaseLocation = Start + (End - Start) * 0.5f;
                        const FVector Location = BaseLocation + Offset;
                        const UFlowGraphItem* Item = Link->LinkItems[i];
                        UFDAbstractNodePreview* LinkItemPreview = NewObject<UFDAbstractNodePreview>(this);
                        LinkItemPreview->SetItemState(Item);
                        LinkItemPreview->SetRelativeLocation(Location);
                        LinkItemPreview->SetWorldScale3D(FVector(Settings.ItemScale));
                        LinkItemPreview->SetupAttachment(RootComponent);

                        UFDAbstractNodePreview*& ItemActorRef = ItemPreviewMap.FindOrAdd(Item->ItemId);
                        ItemActorRef = LinkItemPreview;
                     }
                }
            }
            else {
                PreviewLink->SetLinkVisibility(false);
            }
        }
    }

    // Link up the dependent items
    FTransform ActorInverseTransform = GetActorTransform().Inverse();
    TArray<UFlowGraphItem*> Items;
    InGraph->GetAllItems(Items);
    for (const UFlowGraphItem* Item : Items) {
        if (!Item) continue;
        UFDAbstractNodePreview** SearchResult = ItemPreviewMap.Find(Item->ItemId);
        if (!SearchResult) continue;
        UFDAbstractNodePreview* ItemPreview = *SearchResult;
        for (const FGuid& ReferencedItemId : Item->ReferencedItemIds) {
            UFDAbstractNodePreview** RefSearchResult = ItemPreviewMap.Find(ReferencedItemId);
            if (!RefSearchResult) continue;
            UFDAbstractNodePreview* RefItemPreview = *RefSearchResult;
            if (ItemPreview && RefItemPreview) {
                // Draw a link between the two items
                FVector Start = ActorInverseTransform.TransformPosition(ItemPreview->GetComponentLocation());
                FVector End = ActorInverseTransform.TransformPosition(RefItemPreview->GetComponentLocation());
                FVector Direction = End - Start;
                Direction.Normalize();

                float ItemRadius = Settings.NodeRadius * Settings.ItemScale; 
                FVector LineStart = Start + Direction * ItemRadius;
                FVector LineEnd = End - Direction * ItemRadius;
                UFDAbstractLinkPreview* RefLink = NewObject<UFDAbstractLinkPreview>(this);
                if (!LineStart.Equals(LineEnd)) {
                    RefLink->SetState(LineStart, LineEnd, Settings.LinkRefThickness, FLinearColor::Red, 1);
                    RefLink->SetDynamicAlignment(ItemPreview, RefItemPreview);
                    RefLink->SetupAttachment(RootComponent);
                }
            }
        }
    }

    ReregisterAllComponents();
}


#undef LOCTEXT_NAMESPACE

