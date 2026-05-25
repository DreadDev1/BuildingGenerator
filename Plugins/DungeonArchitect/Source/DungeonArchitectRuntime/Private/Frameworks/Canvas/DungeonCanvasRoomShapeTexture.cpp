//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/DungeonCanvasRoomShapeTexture.h"

#include "Components/BillboardComponent.h"
#include "DynamicMeshBuilder.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PrimitiveSceneProxy.h"
#include "SceneManagement.h"
#include "UObject/ConstructorHelpers.h"

class FDungeonCanvasOverlayTextureSceneProxy : public FPrimitiveSceneProxy {
public:
	FDungeonCanvasOverlayTextureSceneProxy(const UDungeonCanvasRoomShapeTextureComponent* InComponent)
		: FPrimitiveSceneProxy(InComponent)
		, Opacity(InComponent->Opacity)
		, bEditorShowOnlyWhenSelected(InComponent->bEditorShowOnlyWhenSelected)
		, OverlayMaterial(InComponent->GetOverlayMaterial())
	{
		
	}

	virtual SIZE_T GetTypeHash() const override {
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}
	
	virtual uint32 GetMemoryFootprint() const override { 
		return sizeof(*this) + GetAllocatedSize();
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override {
		if (bEditorShowOnlyWhenSelected && !IsSelected()) {
			return;
		}
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++) {
			if (VisibilityMap & (1 << ViewIndex)) {
				const FSceneView* View = Views[ViewIndex];
				FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
				FTransform Transform = FTransform(GetLocalToWorld());

				const FLinearColor LineColor = FLinearColor(1, 0, 0, Opacity);
				for (int i = 0; i < 4; i++) {
					const FVector P0 = Transform.TransformPosition(FDAShapeTexture::LocalQuadPoints[i]);
					const FVector P1 = Transform.TransformPosition(FDAShapeTexture::LocalQuadPoints[(i + 1) % 4]);
					PDI->DrawTranslucentLine(P0, P1, LineColor, SDPG_World);
				}

				if (OverlayMaterial.IsValid()) {
					FDynamicMeshBuilder MeshBuilder(GMaxRHIFeatureLevel);
					// Draw the local quad
					{
						static const FVector3f TangentX(1, 0, 0);
						static const FVector3f TangentY(0, 1, 0);
						static const FVector3f TangentZ(0, 0, 1);

					
						const FColor QuadColor = FLinearColor(1, 1, 1, Opacity).ToFColor(true);
						const FVector& V0 = Transform.TransformPosition(FDAShapeTexture::LocalQuadPoints[0]);
						const FVector& V1 = Transform.TransformPosition(FDAShapeTexture::LocalQuadPoints[1]);
						const FVector& V2 = Transform.TransformPosition(FDAShapeTexture::LocalQuadPoints[2]);
						const FVector& V3 = Transform.TransformPosition(FDAShapeTexture::LocalQuadPoints[3]);
						const FVector2f T0 = FVector2f(FDAShapeTexture::QuadUV[0]);
						const FVector2f T1 = FVector2f(FDAShapeTexture::QuadUV[1]);
						const FVector2f T2 = FVector2f(FDAShapeTexture::QuadUV[2]);
						const FVector2f T3 = FVector2f(FDAShapeTexture::QuadUV[3]);
			
						const int32 VertexOffset = MeshBuilder.AddVertex(FVector3f(V0), T0, TangentX, TangentY, TangentZ, QuadColor);
						MeshBuilder.AddVertex(FVector3f(V1), T1, TangentX, TangentY, TangentZ, QuadColor);
						MeshBuilder.AddVertex(FVector3f(V2), T2, TangentX, TangentY, TangentZ, QuadColor);
						MeshBuilder.AddVertex(FVector3f(V3), T3, TangentX, TangentY, TangentZ, QuadColor);

						MeshBuilder.AddTriangle(VertexOffset + 1, VertexOffset, VertexOffset + 2);
						MeshBuilder.AddTriangle(VertexOffset + 2, VertexOffset, VertexOffset + 3);
					}
				
					MeshBuilder.GetMesh(FMatrix::Identity, OverlayMaterial->GetRenderProxy(), SDPG_World, true, false, ViewIndex, Collector);
				}
			}
		}
	}
	
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override {
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		return Result;
	}
private:
	float Opacity;
	bool bEditorShowOnlyWhenSelected;
	TWeakObjectPtr<UMaterialInstanceDynamic> OverlayMaterial{};
};


FDungeonCanvasRoomShapeTextureList FDungeonCanvasRoomShapeTextureList::TransformBy(const FTransform& InTransform) const {
	FDungeonCanvasRoomShapeTextureList Result = *this;
	    
	for (FDAShapeTexture& Shape : Result.TextureShapes) {
		Shape.Transform = Shape.Transform * InTransform;
	}
	
	return Result;
}

UDungeonCanvasRoomShapeTextureComponent::UDungeonCanvasRoomShapeTextureComponent()
	: OverlayMaterialTemplate(FSoftObjectPath(TEXT("/DungeonArchitect/Core/Runtime/Features/DungeonCanvas/Materials/Instances/M_CanvasTextureOverlay_Inst.M_CanvasTextureOverlay_Inst")))
{
	bWantsInitializeComponent = true;
	
	if (GIsEditor && !IsRunningCommandlet()) {
		struct FConstructorStatics {
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> TexWhite;
			FConstructorStatics()
				: TexWhite(TEXT("/Engine/EngineResources/WhiteSquareTexture"))
			{}
		};
        static FConstructorStatics ConstructorStatics;
		WhiteTexture = ConstructorStatics.TexWhite.Get();
	}
}

#if WITH_EDITOR
void UDungeonCanvasRoomShapeTextureComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UDungeonCanvasRoomShapeTextureComponent, OverlayTexture)) {
		if (!OverlayMaterial) {
			RecreateOverlayMaterial();
		}
		if (OverlayMaterial) {
			OverlayMaterial->SetTextureParameterValue("Texture", GetOverlayTexture());
			MarkRenderStateDirty();
		}
	}
}
#endif // WITH_EDITOR

FPrimitiveSceneProxy* UDungeonCanvasRoomShapeTextureComponent::CreateSceneProxy() {
	const UWorld* World = GetWorld();
	if (!World->IsGameWorld() && bVisualizeTexture) {
		if (!OverlayMaterial) {
			RecreateOverlayMaterial();
		}
		if (OverlayMaterial) {
			OverlayMaterial->SetTextureParameterValue("Texture", GetOverlayTexture());
		}
		
		return new FDungeonCanvasOverlayTextureSceneProxy(this);
	}
	return nullptr;
}

void UDungeonCanvasRoomShapeTextureComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const {
	if (OverlayMaterial) {
		OutMaterials.Add(OverlayMaterial);
	}
}

TObjectPtr<UMaterialInstanceDynamic> UDungeonCanvasRoomShapeTextureComponent::GetOverlayMaterial() const {
	return OverlayMaterial;
}

void UDungeonCanvasRoomShapeTextureComponent::InitializeComponent() {
	Super::InitializeComponent();

	if (!OverlayMaterial) {
		RecreateOverlayMaterial();
	}
	if (OverlayMaterial) {
		OverlayMaterial->SetTextureParameterValue("Texture", GetOverlayTexture());
		MarkRenderStateDirty();
	}
}

FBoxSphereBounds UDungeonCanvasRoomShapeTextureComponent::CalcBounds(const FTransform& LocalToWorld) const {
	constexpr float Extent = 50;
	const FBox LocalBounds(FVector(-Extent, -Extent, -10), FVector(Extent, Extent, 10));
	FTransform Transform = GetComponentTransform();
	FVector Scale = GetComponentScale();
	Scale.Z = 1;
	Transform.SetScale3D(Scale);
	
	const FBox WorldBox = LocalBounds.TransformBy(Transform);
	return FBoxSphereBounds(WorldBox);
}

void UDungeonCanvasRoomShapeTextureComponent::RecreateOverlayMaterial() {
	if (UMaterialInterface* MaterialTemplateParent = OverlayMaterialTemplate.LoadSynchronous()) {
		OverlayMaterial = UMaterialInstanceDynamic::Create(MaterialTemplateParent, this);
		OverlayMaterial->SetTextureParameterValue("Texture", GetOverlayTexture());
		MarkRenderStateDirty();
	}
}

UTexture* UDungeonCanvasRoomShapeTextureComponent::GetOverlayTexture() const {
	return OverlayTexture.IsValid() ? OverlayTexture.LoadSynchronous() : WhiteTexture;
}

ADungeonCanvasRoomShapeTextureActor::ADungeonCanvasRoomShapeTextureActor() {
	bRelevantForLevelBounds = false;
	OverlayTextureComponent = CreateDefaultSubobject<UDungeonCanvasRoomShapeTextureComponent>("OverlayTexture");
	SetHidden(true);
	SetCanBeDamaged(false);

	RootComponent = OverlayTextureComponent;
	
#if WITH_EDITORONLY_DATA
	Billboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>("Billboard");
	if (Billboard) {
		Billboard->SetupAttachment(RootComponent);
	}
#endif
}

