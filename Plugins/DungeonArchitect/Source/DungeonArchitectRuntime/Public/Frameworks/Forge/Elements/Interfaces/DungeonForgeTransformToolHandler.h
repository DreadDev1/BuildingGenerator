//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DungeonForgeTransformToolHandler.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UDungeonForgeTransformToolHandler : public UInterface
{
	GENERATED_BODY()
};

class DUNGEONARCHITECTRUNTIME_API IDungeonForgeTransformToolHandler
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon Forge|Transform Tool")
	bool GetTransformToolTransform(FTransform& OutTransform) const;
	virtual bool GetTransformToolTransform_Implementation(FTransform& OutTransform) const = 0;

	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon Forge|Transform Tool")
	void SetTransformToolTransform(const FTransform& NewTransform, bool bInteractionEnded);
	virtual void SetTransformToolTransform_Implementation(const FTransform& NewTransform, bool bInteractionEnded) = 0;

	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon Forge|Transform Tool")
	bool SupportsTransformToolRotation() const;
	virtual bool SupportsTransformToolRotation_Implementation() const { return false; }

	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon Forge|Transform Tool")
	bool SupportsTransformToolScale() const;
	virtual bool SupportsTransformToolScale_Implementation() const { return false; }

	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon Forge|Transform Tool")
	void OnTransformToolActivated();
	virtual void OnTransformToolActivated_Implementation() {};

	UFUNCTION(BlueprintNativeEvent, Category = "Dungeon Forge|Transform Tool")
	void OnTransformToolDeactivated();
	virtual void OnTransformToolDeactivated_Implementation() {};
};

