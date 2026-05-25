//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Common/Utils/DungeonScenePostProcess.h"

#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/BaseFlowLayoutTaskSpawnItems.h"

#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialInstanceDynamic.h"

////////////// UDungeonThemeEdModePostProcessBase //////////////
void UDungeonScenePostProcess::Init(UWorld* InWorld, UMaterialInterface* InMasterMaterial) {
	MasterMaterial = InMasterMaterial;
	UpdateResources(InWorld);
}

void UDungeonScenePostProcess::Destroy() {
	// Clean up the post process volume
	if (PostProcessVolume)
	{
		PostProcessVolume->Destroy();
		PostProcessVolume = nullptr;
	}
	PostProcessMaterial = nullptr;
}

void UDungeonScenePostProcess::Update(UWorld* InTargetWorld) {
	if (!InTargetWorld) {
		return;
	}
	
	UpdateResources(InTargetWorld);
}

void UDungeonScenePostProcess::UpdateResources(UWorld* InWorld) {
	bool bAssignMaterial = false;
	if (!PostProcessMaterial) {
		if (MasterMaterial) {
			PostProcessMaterial = UMaterialInstanceDynamic::Create(MasterMaterial, this);
			bAssignMaterial = true;
		}
	}
	
	if (!PostProcessVolume) {
		// Spawn the post process volume
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient; // Mark as transient so it gets cleaned up
		PostProcessVolume = InWorld->SpawnActor<APostProcessVolume>(SpawnParams);
		PostProcessVolume->bUnbound = true;
		bAssignMaterial = true;
	}
	
	if (bAssignMaterial && PostProcessVolume && PostProcessMaterial) {
		FWeightedBlendable NewBlendable;
		NewBlendable.Object = PostProcessMaterial.Get();
		NewBlendable.Weight = 1.0f;
		PostProcessVolume->Settings.WeightedBlendables.Array.Add(NewBlendable);
	}
}

