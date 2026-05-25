//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/ThemeEngine/Rules/Selector/DungeonSelectorLogic.h"
#include "SnapConnectionSelectorLogic.generated.h"

class ADungeon;

USTRUCT(BlueprintType)
struct FSnapConnectionSelectorInfo {
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, Category="Dungeon Architect")
	FName ModuleCategory;

	UPROPERTY(BlueprintReadWrite, Category="Dungeon Architect")
	FGuid ModuleInstanceId;
	
};


UCLASS(Abstract, HideDropdown, NotBlueprintable)
class USnapConnectionSelectorLogic : public UDungeonSelectorLogic {
	GENERATED_BODY()
public:
	virtual bool InvokeSelectNode(ADungeon* InDungeon, const FGuid& InOwningModuleId, const FGuid& InRemoteModuleId, const FRandomStream& InRandomStream, const FTransform& MarkerTransform);

	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon")
	bool SelectNode(const FSnapConnectionSelectorInfo& OwningModuleInfo, const FSnapConnectionSelectorInfo& RemoteModuleInfo,
					const FRandomStream& RandomStream, const FTransform& MarkerTransform);

	virtual bool SelectNode_Implementation(const FSnapConnectionSelectorInfo& OwningModuleInfo, const FSnapConnectionSelectorInfo& RemoteModuleInfo,
					const FRandomStream& RandomStream, const FTransform& MarkerTransform);
};

