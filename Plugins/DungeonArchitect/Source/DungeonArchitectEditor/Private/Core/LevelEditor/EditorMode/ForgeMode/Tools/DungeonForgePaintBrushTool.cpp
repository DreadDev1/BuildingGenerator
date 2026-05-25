//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ForgeMode/Tools/DungeonForgePaintBrushTool.h"

#include "Builders/Forge/DungeonForgeBuilder.h"
#include "Core/Common/DungeonArchitectEditorLog.h"
#include "Core/Dungeon.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorMode.h"
#include "Frameworks/Forge/Elements/DungeonForgePaintableTextureElement.h"
#include "Frameworks/Forge/Elements/Interfaces/DungeonForgePaintBrushToolHandler.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

#include "BaseBehaviors/ClickDragBehavior.h"
#include "BaseBehaviors/MouseHoverBehavior.h"
#include "CanvasItem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "InteractiveToolManager.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

///////////////////////////// UDungeonForgePaintBrushToolBuilder /////////////////////////////
UDungeonForgePaintBrushToolBuilder::UDungeonForgePaintBrushToolBuilder() {
	ToolClass = UDungeonForgePaintBrushTool::StaticClass();
}

void UDungeonForgePaintBrushToolBuilder::SetState(UDungeonForgeNodeSettings* InNodeSettings, ADungeon* InDungeon) {
	NodeSettings = InNodeSettings;
	Dungeon = InDungeon;
}

UInteractiveTool* UDungeonForgePaintBrushToolBuilder::BuildTool(const FToolBuilderState& SceneState) const {
	UInteractiveTool* Tool = Super::BuildTool(SceneState);
	if (UDungeonForgePaintBrushTool* TransformTool = Cast<UDungeonForgePaintBrushTool>(Tool)) {
		TransformTool->SetState(NodeSettings.Get(), Dungeon.Get());
	}
	return Tool;
}


///////////////////////////// UDungeonForgePaintBrushTool /////////////////////////////
void UDungeonForgePaintBrushTool::SetState(UDungeonForgeNodeSettings* InNodeSettings, ADungeon* InDungeon) {
	TargetNodeSettings = InNodeSettings;
	Dungeon = InDungeon;
}

FInputRayHit UDungeonForgePaintBrushTool::CanBeginClickDragSequence(const FInputDeviceRay& PressPos) {
	return FInputRayHit(0.0f);
}

UCanvasRenderTarget2D* UDungeonForgePaintBrushTool::GetCanvasTexture() const {
	UCanvasRenderTarget2D* RenderTexture = nullptr;
	if (TargetNodeSettings.IsValid()) {
		if (UDungeonForgeGraphNode* GraphNode = Cast<UDungeonForgeGraphNode>(TargetNodeSettings->GetOuter())) {
			if (UDungeonForgeBuilder* ForgeBuilder = Dungeon.IsValid() ? Cast<UDungeonForgeBuilder>(Dungeon->GetBuilder()) : nullptr) {
				if (const FDungeonForgeContext* NodeContext = ForgeBuilder->GetNodeInstanceContext(GraphNode->NodeGuid)) {
					const FDungeonForgeTextureContextBase* TextureNodeContext = StaticCast<const FDungeonForgeTextureContextBase*>(NodeContext);
					RenderTexture = TextureNodeContext->RenderTarget.Get();
				}
			}
		}
	}
	return RenderTexture;
}

TArray<UObject*> UDungeonForgePaintBrushTool::GetToolProperties(bool bEnabledOnly) const {
	return { BrushSettings };
}

void UDungeonForgePaintBrushTool::Setup() {
	Super::Setup();
	BrushSettings = NewObject<UDungeonForgePaintBrushSettings>(this);
	
	// add default mouse input behavior
	{
		UClickDragInputBehavior* ClickDragBehavior = NewObject<UClickDragInputBehavior>();
		ClickDragBehavior->Initialize(this);
		AddInputBehavior(ClickDragBehavior);

		UMouseHoverBehavior* HoverBehaviour = NewObject<UMouseHoverBehavior>();
		HoverBehaviour->Initialize(this);
		AddInputBehavior(HoverBehaviour);
	}

	UWorld* World = GetWorld();
	if (TargetNodeSettings.IsValid() && World) {
		UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"), nullptr, LOAD_None, nullptr);
		if (UMaterialInterface* PlaneMaterialTemplate = LoadObject<UMaterialInterface>(nullptr, TEXT("/DungeonArchitect/Core/Editors/ForgeEditor/Materials/M_ToolPaintBrushPlane_Inst.M_ToolPaintBrushPlane_Inst"), nullptr, LOAD_None, nullptr)) {
			PlaneMaterial = UMaterialInstanceDynamic::Create(PlaneMaterialTemplate, this);
			if (PlaneMaterial) {
				if (UTextureRenderTarget2D* RenderTarget = GetCanvasTexture()) {
					PlaneMaterial->SetTextureParameterValue(TEXT("Texture"), RenderTarget);
				}
			}
		}

		if (UMaterialInterface* BrushMaterialTemplate = LoadObject<UMaterialInterface>(nullptr, TEXT("/DungeonArchitect/Core/Editors/ForgeEditor/Materials/M_BrushStandard_Inst.M_BrushStandard_Inst"), nullptr, LOAD_None, nullptr)) {
			BrushMaterial = UMaterialInstanceDynamic::Create(BrushMaterialTemplate, this);
		}
		
		if (PlaneMesh && PlaneMaterial) {
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			TexturePlaneMesh = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

			if (TexturePlaneMesh) {
				if (UStaticMeshComponent* MeshComponent = TexturePlaneMesh->GetStaticMeshComponent()) {
					MeshComponent->SetStaticMesh(PlaneMesh);
					MeshComponent->SetMaterial(0, PlaneMaterial);
					MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Disable collision for visualization mesh

					// Initial transform update
					//FTransform PlaneTransform;
					if (IDungeonForgePaintBrushToolHandler::Execute_GetPaintCanvasTransform(TargetNodeSettings.Get(), TextureWorldTransform)) {
						if (UDungeonForgePaintableTextureSettings* PaintableTextureNodeSettings = Cast<UDungeonForgePaintableTextureSettings>(TargetNodeSettings.Get())) {
							FVector FinalScale = TextureWorldTransform.GetScale3D() * PaintableTextureNodeSettings->BaseScale;
							TextureWorldTransform.SetScale3D(FinalScale);

							constexpr float PlaneMeshSize = 100.0f;
							FTransform PlaneTransform = TextureWorldTransform;
							PlaneTransform.SetScale3D(PlaneTransform.GetScale3D() / PlaneMeshSize);
							TexturePlaneMesh->SetActorTransform(PlaneTransform);
						}
					}
				}
			}
		} else {
			// Log errors if mesh or material failed to load
			if (!PlaneMesh) UE_LOG(LogTemp, Warning, TEXT("UDungeonForgePaintBrushTool::Setup: Failed to load Plane Mesh '/Engine/BasicShapes/Plane.Plane'"));
			if (!PlaneMaterial) UE_LOG(LogTemp, Warning, TEXT("UDungeonForgePaintBrushTool::Setup: Failed to load Material '/DungeonArchitect/Core/Editors/ForgeEditor/Materials/M_VisualizeTexture_Inst.M_VisualizeTexture_Inst'"));
		}
	}
}

void UDungeonForgePaintBrushTool::Shutdown(EToolShutdownType ShutdownType) {
	Super::Shutdown(ShutdownType);

	if (TexturePlaneMesh) {
		TexturePlaneMesh->Destroy();
		TexturePlaneMesh = nullptr;
	}
}

void UDungeonForgePaintBrushTool::OnTick(float DeltaTime) {
	Super::OnTick(DeltaTime);
	
	// Get the queries API from the tool manager
	IToolsContextQueriesAPI* QueriesAPI = GetToolManager()->GetContextQueriesAPI();
	if (!QueriesAPI) {
		return;
	}

}

void UDungeonForgePaintBrushTool::Render(IToolsContextRenderAPI* RenderAPI) {
	Super::Render(RenderAPI);
}


void UDungeonForgePaintBrushTool::OnClickPress(const FInputDeviceRay& PressPos) {
	bDrawing = true;
	FVector LocalUV;
	if (GetLocalTextureUV(PressPos, LocalUV)) {
		PreviousStrokeUV = LocalUV;
		DrawStroke(LocalUV);
		UpdateCursorPosition(LocalUV);
	}
}

void UDungeonForgePaintBrushTool::OnClickDrag(const FInputDeviceRay& DragPos) {
	FVector LocalUV;
	if (GetLocalTextureUV(DragPos, LocalUV)) {
		DrawStroke(LocalUV);
		UpdateCursorPosition(LocalUV);
	}
}

void UDungeonForgePaintBrushTool::OnClickRelease(const FInputDeviceRay& ReleasePos) {
	bDrawing = false;
}

void UDungeonForgePaintBrushTool::OnTerminateDragSequence() {
}

float UDungeonForgePaintBrushTool::GetLocalBrushSize() const {
	if (BrushSettings) {
		return BrushSettings->BrushWorldSize / TextureWorldTransform.GetScale3D().X;
	}
	return 0.1f;
}

void UDungeonForgePaintBrushTool::DrawStroke(const FVector& InLocalUV) {
	UWorld* World = GetWorld();
	if (World && BrushMaterial) {
		if (UCanvasRenderTarget2D* CanvasRT = GetCanvasTexture()) {
			UCanvas* Canvas = nullptr;
			FVector2D CanvasSize;
			FDrawToRenderTargetContext DrawContext;
			UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(World, CanvasRT, Canvas, CanvasSize, DrawContext);

			float LocalBrushSize = GetLocalBrushSize();
			float PixelBrushSize = LocalBrushSize * FMath::Max(CanvasSize.X, CanvasSize.Y);
			FVector2D StrokeSize = FVector2D(PixelBrushSize, PixelBrushSize);
			FVector2D StartStrokePosition = FVector2D(PreviousStrokeUV) * CanvasSize;
			FVector2D EndStrokePosition = FVector2D(InLocalUV) * CanvasSize;
			float StrokeLength = FMath::FloorToInt((EndStrokePosition - StartStrokePosition).Length());
			StrokeLength = FMath::Max(1, StrokeLength);
			FVector2D StrokeMovementDelta = (StartStrokePosition - EndStrokePosition) / StrokeLength;
			for (int i = 0; i < StrokeLength; i++) {
				FVector2D Position = EndStrokePosition + StrokeMovementDelta * i;
				Position -= StrokeSize * 0.5f;
				FCanvasTileItem TileItem(Position, BrushMaterial->GetRenderProxy(), StrokeSize);
				TileItem.BlendMode = SE_BLEND_Translucent;
				Canvas->DrawItem(TileItem);
			}
			
			UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(World, DrawContext);
		}
	}

	PreviousStrokeUV = InLocalUV;
}

FInputRayHit UDungeonForgePaintBrushTool::BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos) {
	if (!TexturePlaneMesh) {
		return FInputRayHit();
	}
	
	FHitResult HitResult{};
	FVector LocalUV;
	if (GetLocalTextureUV(PressPos, LocalUV, &HitResult)) {
		return FInputRayHit(HitResult.Distance);
	}
	return FInputRayHit();
}
	
void UDungeonForgePaintBrushTool::OnBeginHover(const FInputDeviceRay& DevicePos) {
	bHovering = true;
	//UE_LOG(LogDungeonForgeEdMode, Log, TEXT(">>> Begin Hover"));
}

bool UDungeonForgePaintBrushTool::OnUpdateHover(const FInputDeviceRay& DevicePos) {
	FVector LocalUV;
	if (GetLocalTextureUV(DevicePos, LocalUV)) {
		UpdateCursorPosition(LocalUV);
	}
	
	return true;
}

bool UDungeonForgePaintBrushTool::GetLocalTextureUV(const FInputDeviceRay& DevicePos, FVector& OutUV, FHitResult* OutHitResult) const {
	const FRay& WorldRay = DevicePos.WorldRay;
	FHitResult HitResult{};
	FVector Start = WorldRay.Origin;
	constexpr float MaxRaycastDistance = 1e6f;
	FVector End = WorldRay.Origin + WorldRay.Direction * MaxRaycastDistance;
	FCollisionQueryParams CollisionQueryParams;
	if (TexturePlaneMesh->GetStaticMeshComponent()->LineTraceComponent(HitResult, Start, End, CollisionQueryParams)) {
		FVector WorldHitLocation = HitResult.Location;
		OutUV = TextureWorldTransform.InverseTransformPosition(WorldHitLocation) + FVector(0.5, 0.5, 0);
		if (OutHitResult) {
			*OutHitResult = HitResult;
		}
		
		return true;
	}
	return false;
}

void UDungeonForgePaintBrushTool::UpdateCursorPosition(const FVector& InLocalUV) const {
	if (PlaneMaterial) {
		PlaneMaterial->SetVectorParameterValue(TEXT("CursorPos"), InLocalUV);
		PlaneMaterial->SetScalarParameterValue(TEXT("CursorSize"), GetLocalBrushSize() * 0.5f);
	}
}

void UDungeonForgePaintBrushTool::OnEndHover() {
	bHovering = false;
	//UE_LOG(LogDungeonForgeEdMode, Log, TEXT(">>> End Hover"));
}

