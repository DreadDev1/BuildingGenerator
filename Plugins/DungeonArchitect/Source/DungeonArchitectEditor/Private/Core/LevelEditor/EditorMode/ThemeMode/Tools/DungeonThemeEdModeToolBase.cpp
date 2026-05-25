//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ThemeMode/Tools/DungeonThemeEdModeToolBase.h"

#include "Core/Actors/DungeonMeshList.h"
#include "Core/Common/DungeonArchitectEditorLog.h"
#include "Core/Common/Utils/DungeonEditorUtils.h"
#include "Core/Common/Utils/DungeonScenePostProcess.h"
#include "Core/Editors/FlowEditor/DomainEditors/Impl/CellFlow/Graph2D/CellFlowDomainEdViewportClient2D.h"
#include "Core/Editors/ThemeEditor/Common/DungeonArchitectThemeEditorUtils.h"
#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorBase.h"
#include "Core/Editors/ThemeEditor/GraphEditor/ThemeGraphLayoutManager.h"
#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdMode.h"
#include "Core/Utils/DungeonUtils.h"
#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"

#include "ActorFactories/ActorFactory.h"
#include "BaseBehaviors/DoubleClickBehavior.h"
#include "BaseGizmos/CombinedTransformGizmo.h"
#include "BaseGizmos/TransformGizmoUtil.h"
#include "BaseGizmos/TransformProxy.h"
#include "Engine/Blueprint.h"
#include "EngineUtils.h"
#include "InteractiveToolManager.h"
#include "Tools/EdModeInteractiveToolsContext.h"

#define LOCTEXT_NAMESPACE "DungeonThemeEdModeToolBase"

void UDungeonThemeEdModeToolBase::Setup() {
	Super::Setup();

	if (UMaterialInterface* BasePostProcessMaterial = GetPostProcessMaterialTemplate()) {
		PostProcessor = NewObject<UDungeonScenePostProcess>(this);
		PostProcessor->Init(TargetWorld.Get(), BasePostProcessMaterial);
	}
	
	// Add double click behaviour
	{
		UDoubleClickInputBehavior* DoubleClickBehavior = NewObject<UDoubleClickInputBehavior>();
		DoubleClickBehavior->Initialize(this);
		AddInputBehavior(DoubleClickBehavior);
	}
}

void UDungeonThemeEdModeToolBase::Shutdown(EToolShutdownType ShutdownType) {
	Super::Shutdown(ShutdownType);

	if (PostProcessor) {
		PostProcessor->Destroy();
		PostProcessor = nullptr;
	}
}

void UDungeonThemeEdModeToolBase::Render(IToolsContextRenderAPI* RenderAPI) {
	Super::Render(RenderAPI);

	
}

void UDungeonThemeEdModeToolBase::SetWorld(UWorld* InWorld) {
	TargetWorld = InWorld;
}

bool UDungeonThemeEdModeToolBase::IsActorSelectionAllowed(AActor* InActor, bool bInSelection) {
	if (!bInSelection) {
		return true;
	}
	
	if (EdMode.IsValid()) {
		if (ADungeon* ActiveDungeon = EdMode->GetActiveDungeon()) {
			return FDungeonUtils::ActorBelongsToDungeon(InActor, ActiveDungeon);
		}
	}
	return false;
}

void UDungeonThemeEdModeToolBase::OnDungeonBuildComplete() {
}

void UDungeonThemeEdModeToolBase::HandleSelectedNodesChanged(const TSet<UObject*>& SelectedNodes) {
	
}

FDungeonThemeEdModeActorSelection UDungeonThemeEdModeToolBase::GetActorSelection() const {
	return EdMode.IsValid() ? EdMode->GetActorSelection() : FDungeonThemeEdModeActorSelection{};
}

void UDungeonThemeEdModeToolBase::HandleNewActorsDropped(const TArray<AActor*> InDroppedActors) {
	TArray<UStaticMesh*> StaticMeshes;
	TArray<UClass*> ActorTemplates;
	UObject* SingleAsset{};

	// Extract assets from dropped actors
	for (AActor* DroppedActor : InDroppedActors) {
		if (!DroppedActor) continue;

		// Check if this is a Blueprint-spawned actor
		if (UClass* ActorClass = DroppedActor->GetClass()) {
			if (UBlueprint* Blueprint = Cast<UBlueprint>(ActorClass->ClassGeneratedBy)) {
				if (Blueprint->ParentClass && Blueprint->ParentClass->IsChildOf<AActor>()) {
					ActorTemplates.Add(Blueprint->GeneratedClass);
				}
				continue;
			}
		}
		
		UObject* AssetObject{};
		if (GEditor) {
			if (UActorFactory* ActorFactory = GEditor->FindActorFactoryForActorClass(DroppedActor->GetClass())) {
				AssetObject = ActorFactory->GetAssetFromActorInstance(DroppedActor);
			}
		}

		if (AssetObject) {
			if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetObject)) {
				StaticMeshes.Add(StaticMesh);
			}
			else {
				SingleAsset = AssetObject;
			}
		}
	}
	
	// Destroy all the dropped actors as we don't need them (they'll be spawned from the theme graph if the implementation choose to add it)
	for (AActor* InDroppedActor : InDroppedActors) {
		InDroppedActor->Destroy();
	}

	UObject* ResultAsset{};
	const int32 MultiItemCount = StaticMeshes.Num() + ActorTemplates.Num();
	if (MultiItemCount > 1) {
		// Create a temp UDungeonMeshList object for passing it around
		UDungeonMeshList* MeshList = NewObject<UDungeonMeshList>();
        
		for (UStaticMesh* Mesh : StaticMeshes) {
			FDungeonMeshListItem& Item = MeshList->StaticMeshes.AddDefaulted_GetRef();
			Item.StaticMesh = Mesh;
			Item.ItemGuid = FGuid::NewGuid();
		}
        
		for (UClass* Template : ActorTemplates) {
			FDungeonActorTemplateListItem& Item = MeshList->ActorTemplates.AddDefaulted_GetRef();
			Item.ClassTemplate = Template;
			Item.ItemGuid = FGuid::NewGuid();
		}
        
		MeshList->CalculateHashCode();
		ResultAsset = MeshList;
	}
	else if (StaticMeshes.Num() == 1) {
		ResultAsset = StaticMeshes[0];
	}
	else if (ActorTemplates.Num() == 1) {
		// For single blueprint, pass the blueprint asset or the class?
		// Passing the class here - adjust if you need the blueprint instead
		ResultAsset = ActorTemplates[0];
	}
	else if (SingleAsset) {
		ResultAsset = SingleAsset;
	}
	
	// Handle the asset dropped event
	if (ResultAsset) {
		OnAssetDragDrop(ResultAsset);
	}
}

void UDungeonThemeEdModeToolBase::OnTick(float DeltaTime) {
	Super::OnTick(DeltaTime);
	
	if (PostProcessor) {
		PostProcessor->Update(TargetWorld.Get());
	}
}

FInputRayHit UDungeonThemeEdModeToolBase::IsHitByClick(const FInputDeviceRay& ClickPos) {
	return FInputRayHit(0);
}

void UDungeonThemeEdModeToolBase::OnClicked(const FInputDeviceRay& ClickPos) {
	FViewport* FocusedViewport = GetToolManager()->GetContextQueriesAPI()->GetFocusedViewport();
	HHitProxy* HitResult = FocusedViewport->GetHitProxy(ClickPos.ScreenPosition.X, ClickPos.ScreenPosition.Y);
	if (HitResult && HitResult->IsA(HActor::StaticGetType())) {
		HActor* ActorHit = static_cast<HActor*>(HitResult);
		if(ActorHit->Actor) {
			OnActorDoubleClicked(ActorHit->Actor);
		}
	}
}

bool UDungeonThemeEdModeToolBase::ProcessDeleteSelectedActor() {
	if (EdMode.IsValid()) {
		const FDungeonThemeEdModeActorSelection ActorSelection = EdMode->GetActorSelection();
		if (ActorSelection.GetSelectedActor()) {
			// Delete the node from the theme graph that this spawned from
			if (UDungeonThemeAsset* ActiveThemeAsset = EdMode->GetThemeAsset()) {
				if (UEdGraphNode_DungeonThemeActorBase* VisualNode = FDungeonArchitectThemeEditorUtils::FindThemeNodeFromSpawnedActor(
						ActorSelection.GetSelectedActor(), ActiveThemeAsset->UpdateGraph))
				{
					EdMode->RequestDeleteThemeNode(VisualNode);
					return true;
				}
			}
		}
	}
	
	return false;
}

void UDungeonThemeEdModeToolBase::SetEdMode(const TWeakObjectPtr<UDungeonThemeEdMode>& InEdMode) {
	EdMode = InEdMode;
}

void UDungeonThemeEdModeToolBase::SetupGizmo() {
	TransformGizmo = UE::TransformGizmoUtil::CreateCustomRepositionableTransformGizmo( 
		GetToolManager()->GetPairedGizmoManager(), ETransformGizmoSubElements::FullTranslateRotateScale,
		this, TEXT("ThemeEdVisualToolSelectionInteraction") );
	
	if (TransformGizmo) {
		TransformGizmo->SetIsNonUniformScaleAllowedFunction([]() {return true;});
	}
}

void UDungeonThemeEdModeToolBase::DestroyGizmo() {
	GetToolManager()->GetPairedGizmoManager()->DestroyAllGizmosByOwner(this);
	TransformProxy = nullptr;
}

void UDungeonThemeEdModeToolBase::SetGizmoVisibility(bool bInVisible) const {
	if (TransformGizmo) {
		TransformGizmo->SetVisibility(bInVisible);
	}
}

void UDungeonThemeEdModeToolBase::SetGizmoTarget(USceneComponent* InSceneComponent) {
	if (TransformGizmo) {
		TransformProxy = NewObject<UTransformProxy>(this);
		TransformProxy->AddComponent(InSceneComponent);
		if (UInteractiveGizmoManager* GizmoManager = EdMode->GetInteractiveToolsContext()->GizmoManager) {
			TransformGizmo->SetActiveTarget(TransformProxy, GizmoManager);
		}
	}
}

#undef LOCTEXT_NAMESPACE

