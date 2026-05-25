//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Snap/Lib/Connection/SnapConnectionSelectorLogic.h"

#include "Core/Dungeon.h"
#include "Frameworks/Snap/Lib/SnapDungeonModelBase.h"

////////////////////////////// USnapMapConnectionSelectorLogic //////////////////////////////
bool USnapConnectionSelectorLogic::InvokeSelectNode(ADungeon* InDungeon, const FGuid& InOwningModuleId, const FGuid& InRemoteModuleId, const FRandomStream& InRandomStream, const FTransform& MarkerTransform) {
	if (const USnapDungeonModelBase* SnapModel = InDungeon ? Cast<USnapDungeonModelBase>(InDungeon->GetModel()) : nullptr) {
		FName OwningModuleCategory;
		FName RemoteModuleCategory;
		for (const FSnapModuleInstanceSerializedData& ModuleInstance : SnapModel->ModuleInstances) {
			if (ModuleInstance.ModuleInstanceId == InOwningModuleId) {
				OwningModuleCategory = ModuleInstance.Category;
			}
			else if (ModuleInstance.ModuleInstanceId == InRemoteModuleId) {
				RemoteModuleCategory = ModuleInstance.Category;
			}
		}
		const FSnapConnectionSelectorInfo OwningModule { OwningModuleCategory, InOwningModuleId };
		const FSnapConnectionSelectorInfo RemoteModule { RemoteModuleCategory, InRemoteModuleId };
		return SelectNode(OwningModule, RemoteModule, InRandomStream, MarkerTransform);
	}
	return false;
}

bool USnapConnectionSelectorLogic::SelectNode_Implementation(const FSnapConnectionSelectorInfo& OwningModuleInfo, const FSnapConnectionSelectorInfo& RemoteModuleInfo, const FRandomStream& RandomStream, const FTransform& MarkerTransform) {
	return false;
}

