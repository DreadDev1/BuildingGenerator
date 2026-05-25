//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/DungeonForgeElement.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "DungeonForgeStaticTransformElement.generated.h"

namespace DungeonForgeStaticLocationPins {
	const FName Transform = TEXT("Transform");
}

UCLASS()
class UDungeonForgeStaticTransformSettings : public UDungeonForgeNodeSettings {
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Settings")
	FTransform Transform = FTransform::Identity;

public:
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override;
	virtual FText GetDefaultNodeTitle() const override;

	virtual FText GetNodeTooltipText() const override;
	virtual EDungeonForgeSettingsType GetType() const override;
#endif // WITH_EDITOR

	
protected:
	virtual TArray<FDungeonForgePinProperties> InputPinProperties() const override;
	virtual TArray<FDungeonForgePinProperties> OutputPinProperties() const override;
	virtual FDungeonForgeElementPtr CreateElement() const override;
};


class FDungeonForgeStaticTransformElement : public IDungeonForgeElement {
protected:
	virtual bool ExecuteImpl(const FDungeonForgeContextPtr& Context) const override;
};


