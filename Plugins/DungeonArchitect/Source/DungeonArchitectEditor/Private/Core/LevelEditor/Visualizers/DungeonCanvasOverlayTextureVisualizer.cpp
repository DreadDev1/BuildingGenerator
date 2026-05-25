//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Visualizers/DungeonCanvasOverlayTextureVisualizer.h"

#include "Frameworks/Canvas/DungeonCanvasRoomShapeTexture.h"

#include "DynamicMeshBuilder.h"
#include "EditorModes.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "SceneView.h"
#include "UObject/Package.h"

FDungeonCanvasOverlayTextureVisualizer::FDungeonCanvasOverlayTextureVisualizer() {
	WhiteTexture = LoadObject<UTexture>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"), nullptr,
		LOAD_None, nullptr);

	if (UDungeonCanvasRoomShapeTextureComponent* ComponentCDO = UDungeonCanvasRoomShapeTextureComponent::StaticClass()->GetDefaultObject<UDungeonCanvasRoomShapeTextureComponent>()) {
		if (UMaterialInterface* MaterialTemplate = ComponentCDO->OverlayMaterialTemplate.LoadSynchronous()) {
			OverlayMaterial = UMaterialInstanceDynamic::Create(MaterialTemplate, GetTransientPackage());
		}
	}
}

void FDungeonCanvasOverlayTextureVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) {
	FComponentVisualizer::DrawVisualization(Component, View, PDI);

	if (const UDungeonCanvasRoomShapeTextureComponent* OverlayTexComponent = Cast<UDungeonCanvasRoomShapeTextureComponent>(Component)) {
		bShowWhenSelected = OverlayTexComponent->bEditorShowOnlyWhenSelected;
		const FTransform Transform = OverlayTexComponent->GetComponentTransform();
		TArray<FVector> WorldPoints;
		for (int i = 0; i < 4; i++) {
			WorldPoints.Add(Transform.TransformPosition(FDAShapeTexture::LocalQuadPoints[i]));
		}

		const float Opacity = OverlayTexComponent->Opacity;
		const FLinearColor LineColor = FLinearColor(1, 0, 0, Opacity);
		for (int i = 0; i < 4; i++) {
			const FVector P0 = WorldPoints[i];
			const FVector P1 = WorldPoints[(i + 1) % 4];
			PDI->DrawTranslucentLine(P0, P1, LineColor, SDPG_World);
		}

		if (UTexture* MaterialTexture = OverlayTexComponent->GetOverlayTexture()) {
			OverlayMaterial->SetTextureParameterValue("Texture", MaterialTexture);
		}

		FDynamicMeshBuilder MeshBuilder(View->FeatureLevel);
		// Allocate the material proxy and register it so it can be deleted properly once the rendering is done with it.
		FDynamicColoredMaterialRenderProxy* SelectedColorInstance = new FDynamicColoredMaterialRenderProxy(
			OverlayMaterial->GetRenderProxy(), FLinearColor::White);
		PDI->RegisterDynamicResource(SelectedColorInstance);

		// Draw the local quad
		{
			static const FVector3f TangentX(1, 0, 0);
			static const FVector3f TangentY(0, 1, 0);
			static const FVector3f TangentZ(0, 0, 1);
			
			const FColor QuadColor = FLinearColor(1, 1, 1, Opacity).ToFColor(true);
			const FVector& V0 = WorldPoints[0];
			const FVector& V1 = WorldPoints[1];
			const FVector& V2 = WorldPoints[2];
			const FVector& V3 = WorldPoints[3];
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
		
		MeshBuilder.Draw(PDI, FMatrix::Identity, SelectedColorInstance, SDPG_World, true, false);
	}
}

void FDungeonCanvasOverlayTextureVisualizer::AddReferencedObjects(FReferenceCollector& Collector) {
	Collector.AddReferencedObject(OverlayMaterial);
}

FString FDungeonCanvasOverlayTextureVisualizer::GetReferencerName() const {
	static const FString NameString = TEXT("DungeonCanvasOverlayTextureVisualizer");
	return NameString;
}

