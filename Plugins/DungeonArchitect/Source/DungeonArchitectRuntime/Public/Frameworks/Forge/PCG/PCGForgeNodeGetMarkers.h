//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGForgeNodeGetMarkers.generated.h"

UCLASS(BlueprintType)
class UPCGForgeNodeGetMarkersSettings : public UPCGSettings {
	GENERATED_BODY()
public:
	UPCGForgeNodeGetMarkersSettings();

	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return FName(TEXT("GetDungeonForgeMarkerPoints")); }
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

class FPCGGetDungeonForgeMarkerDataElement : public IPCGElement {
public:
	virtual bool CanExecuteOnlyOnMainThread(FPCGContext* Context) const override { return true; }
	virtual bool IsCacheable(const UPCGSettings* InSettings) const override { return false; }
	
protected:
	virtual bool ExecuteInternal(FPCGContext* InContext) const override;
};

