//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/DungeonForgeElement.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "DungeonForgeApplyPCG.generated.h"

class UPCGGraph;
class UPCGComponent;

UCLASS()
class UDungeonForgeApplyPCGSettings : public UDungeonForgeNodeSettings {
	GENERATED_BODY()
public:	
	UPROPERTY(EditAnywhere, Category = "Settings")
	TObjectPtr<UPCGGraph> PCGGraph;
	
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

struct FDungeonForgeApplyPCGContext : FDungeonForgeContext {
	TWeakObjectPtr<UPCGComponent> PCGComponent;
};
typedef TSharedPtr<FDungeonForgeApplyPCGContext, ESPMode::ThreadSafe> FDungeonForgeApplyPCGContextPtr;

class FDungeonForgeApplyPCGElement : public IDungeonForgeElement {
protected:
	virtual bool ExecuteImpl(const FDungeonForgeContextPtr& Context) const override;
	virtual bool TickImpl(const FDungeonForgeContextPtr& Context, float DeltaTime) const override;
	virtual FDungeonForgeContextPtr CreateContext() override;

private:
	static void HandleGenerationComplete(const FDungeonForgeApplyPCGContextPtr& InContext);
	static bool IsGenerationComplete(const FDungeonForgeApplyPCGContextPtr& Context);
};

