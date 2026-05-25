//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonForgeGraphEdge.generated.h"

class UDungeonForgeGraphNode;
class UDungeonForgeGraphPin;

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeGraphEdge : public UObject {
	GENERATED_BODY()
public:
	UDungeonForgeGraphEdge(const FObjectInitializer& ObjectInitializer);

	// ~Begin UObject interface
	virtual void PostLoad() override;
	// ~End UObject interface

	/** Pin at upstream end of edge. */
	UPROPERTY()
	TObjectPtr<UDungeonForgeGraphPin> InputPin;

	/** Pin at downstream end of edge. */
	UPROPERTY()
	TObjectPtr<UDungeonForgeGraphPin> OutputPin;

	bool IsValid() const;
	UDungeonForgeGraphPin* GetOtherPin(const UDungeonForgeGraphPin* Pin);
	const UDungeonForgeGraphPin* GetOtherPin(const UDungeonForgeGraphPin* Pin) const;
};

