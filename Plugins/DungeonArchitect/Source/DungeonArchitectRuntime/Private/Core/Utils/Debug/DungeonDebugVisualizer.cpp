//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Utils/Debug/DungeonDebugVisualizer.h"

#include "Components/LineBatchComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

void UDungeonDebugVisualizer::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	// Do not call the base tick, as we don't want the objects to expire and have infinite lifetime
	// This also avoids iterating over all the items in the scene
}

void UDungeonDebugVisualizer::Clear() {
	Flush();
	
	for (AActor* Actor : DebugActors) {
		if (IsValid(Actor)) {
			Actor->Destroy();
		}
	}
	DebugActors.Empty();
}

void UDungeonDebugVisualizer::DrawTexturedQuad(const FTransform& InTransform, UTexture* InTexture, UMaterialInterface* InRenderMaterial) {
	UWorld* World = GetWorld();
	if (!World || !InTexture) return;

	constexpr float MeshSize = 100;
	FVector MeshScale = InTransform.GetScale3D() / MeshSize;
	FTransform MeshTransform = InTransform;
	MeshTransform.SetScale3D(MeshScale);
	AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), MeshTransform);
	UStaticMeshComponent* MeshComp = MeshActor->GetStaticMeshComponent();

	if (UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane"))) {
		MeshComp->SetStaticMesh(PlaneMesh);

		UMaterialInterface* BaseMaterial = InRenderMaterial;
		if (!BaseMaterial) {
			BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/DungeonArchitect/Core/Editors/ForgeEditor/Materials/M_VisualizeTexture_Inst"));
		}
		
		if (BaseMaterial) {
			UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, MeshActor);
			DynMaterial->SetTextureParameterValue(TEXT("Texture"), InTexture);
			MeshComp->SetMaterial(0, DynMaterial);
		}
	}
    
	DebugActors.Add(MeshActor);
}

