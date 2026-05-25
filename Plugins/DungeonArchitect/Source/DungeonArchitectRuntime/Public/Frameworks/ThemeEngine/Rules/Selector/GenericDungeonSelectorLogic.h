//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "GenericDungeonSelectorLogic.generated.h"

struct FDungeonMarkerInstance;
class UDungeonQuery;
class UDungeonBuilder;
class UDungeonConfig;
class UDungeonModel;
/**
 *
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable, HideDropDown)
class DUNGEONARCHITECTRUNTIME_API UGenericDungeonSelectorLogic : public UObject {
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon")
	bool SelectNode(UDungeonModel* Model, UDungeonConfig* Config, UDungeonBuilder* Builder, UDungeonQuery* Query,
					const FRandomStream& RandomStream, const FDungeonMarkerInstance& MarkerInstance);

	virtual bool SelectNode_Implementation(UDungeonModel* Model, UDungeonConfig* Config, UDungeonBuilder* Builder, UDungeonQuery* Query,
					const FRandomStream& RandomStream, const FDungeonMarkerInstance& MarkerInstance);
};


