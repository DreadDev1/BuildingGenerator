//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/Data/DungeonForgeTextureData.h"
#include "Frameworks/Forge/DungeonForgeContext.h"
#include "Frameworks/Forge/DungeonForgeElement.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "Frameworks/Forge/Elements/Interfaces/DungeonForgeTransformToolHandler.h"
#include "DungeonForgeTextureElementBase.generated.h"

class UCanvasRenderTarget2D;

class FDungeonForgeTextureDataFrameTransformer {
public:
	FDungeonForgeTextureDataFrameTransformer(float InBaseScale, int32 InWidth, int32 InHeight, const FTransform& InWorldTransform);
	FVector IndexToWorldPos(int32 Index) const;

private:
	float BaseScale{};
	int32 Width{};
	int32 Height{};
	FTransform WorldTransform;
	
	FVector LocalScale;
	FVector LocalOffset;
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeTextureSettingsBase
	: public UDungeonForgeNodeSettings
	, public IDungeonForgeTransformToolHandler
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Settings")
	int32 TargetResolution = 64;
	
	UPROPERTY(EditAnywhere, Category = "Settings")
	EDungeonForgeTextureColorChannel ColorChannel = EDungeonForgeTextureColorChannel::Red;
	
	UPROPERTY(EditAnywhere, Category = "Settings")
	FTransform Transform = FTransform::Identity;

	UPROPERTY(EditAnywhere, Category = "Settings", AdvancedDisplay)
	float BaseScale = 1000;

	UPROPERTY(EditAnywhere, Category = "Debug")
	TSoftObjectPtr<UMaterialInterface> DebugMaterial;
	
public:
	UDungeonForgeTextureSettingsBase();
	
	//~Begin IDungeonForgeTransformToolHandler Interface
	virtual bool GetTransformToolTransform_Implementation(FTransform& OutTransform) const override;
	virtual void SetTransformToolTransform_Implementation(const FTransform& NewTransform, bool bInteractionEnded) override;
	virtual bool SupportsTransformToolRotation_Implementation() const override { return true; }
	virtual bool SupportsTransformToolScale_Implementation() const override { return true; }
	//~End IDungeonForgeTransformToolHandler Interface
	
#if WITH_EDITOR
	virtual EDungeonForgeSettingsType GetType() const override;
#endif // WITH_EDITOR

	
protected:
	virtual TArray<FDungeonForgePinProperties> InputPinProperties() const override;
	virtual TArray<FDungeonForgePinProperties> OutputPinProperties() const override;
	virtual FDungeonForgeElementPtr CreateElement() const override;
};


struct FDungeonForgeTextureContextBase : FDungeonForgeContext {
	TWeakObjectPtr<UDungeonForgeTextureData> TextureData;
	TWeakObjectPtr<UCanvasRenderTarget2D> RenderTarget;
	FRenderCommandFence RenderFence;
	TArray<FColor> DownloadedPixels;
	int32 TargetResolution{};
	FIntPoint DownloadedSize;
};

typedef TSharedPtr<FDungeonForgeTextureContextBase, ESPMode::ThreadSafe> FDungeonForgeTextureContextBasePtr;
typedef TWeakPtr<FDungeonForgeTextureContextBase, ESPMode::ThreadSafe> FDungeonForgeTextureContextBaseWeakPtr;

class DUNGEONARCHITECTRUNTIME_API FDungeonForgeTextureElementBase : public IDungeonForgeElement {
protected:
	virtual bool ExecuteImpl(const FDungeonForgeContextPtr& Context) const override;
	virtual bool TickImpl(const FDungeonForgeContextPtr& Context, float DeltaTime) const override;

	virtual bool GetSourceTextureSize(const FDungeonForgeContextPtr& InContext, int32& OutWidth, int32& OutHeight) const;
	virtual bool InitializeCanvasTexture(UWorld* InWorld, const FDungeonForgeContextPtr& InContext, UCanvasRenderTarget2D* CanvasRT) const;
	virtual void InitializeTextureData(UDungeonForgeTextureData* TextureData, UDungeonForgeTextureSettingsBase* NodeSettings) const {}
	virtual FDungeonForgeContextPtr CreateContext() override;

private:
	bool DownloadCanvasTextureFromGPU(const FDungeonForgeTextureContextBasePtr& Context) const;
	static bool IsDownloadComplete(const FDungeonForgeTextureContextBasePtr& Context);
	static void HandleDownloadComplete(const FDungeonForgeTextureContextBasePtr& Context);
};


