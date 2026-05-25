//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGNodeMergeMarkerPoints.generated.h"

UCLASS(BlueprintType)
class DUNGEONARCHITECTRUNTIME_API UPCGMergeDungeonMarkerPointsSettings : public UPCGSettings {
	GENERATED_BODY()
public:
	UPCGMergeDungeonMarkerPointsSettings();

	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("MergeDungeonMarkerPoints")); }
	virtual FText GetDefaultNodeTitle() const override;
	virtual FText GetNodeTooltipText() const override;
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spatial; }
#endif

	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual bool UseSeed() const override;

protected:
	virtual FPCGElementPtr CreateElement() const override;
	// ~End UPCGSettings interface
};


class FPCGMergeDungeonMarkerDataElement : public IPCGElement {
public:
	virtual bool IsCacheable(const UPCGSettings* InSettings) const override { return false; }
	
protected:
	virtual FPCGContext* CreateContext() override;
	virtual bool PrepareDataInternal(FPCGContext* InContext) const override;
	virtual bool ExecuteInternal(FPCGContext* InContext) const override;
};


