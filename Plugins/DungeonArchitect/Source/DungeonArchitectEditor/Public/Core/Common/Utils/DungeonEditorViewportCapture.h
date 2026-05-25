//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "DungeonEditorViewportCapture.generated.h"

class UTextureRenderTarget2D;
class USceneCaptureComponent2D;

UCLASS()
class UDungeonEditorViewportCapture : public UObject {
	GENERATED_BODY()
public:
	UDungeonEditorViewportCapture();

	virtual void Initialize(UWorld* InWorld);
	virtual void Deinitialize();

	virtual void BeginDestroy() override;
    
	UFUNCTION()
	void SetRenderTargetSize(int32 Width, int32 Height);
    
	UFUNCTION()
	UTextureRenderTarget2D* GetOrCreateAlphaMaskRenderTarget();
    
	UFUNCTION()
	void SetAlphaMaskedActors(const TArray<AActor*>& InCaptureTargets);
    
	UFUNCTION()
	void CaptureAlphaMask(const FMinimalViewInfo& InViewInfo);
    
	UFUNCTION()
	virtual void ReleaseResources();

	UFUNCTION()
	virtual void ReclaimResources();
    
private:    
	bool CaptureScene(UTextureRenderTarget2D* InRenderTarget, const TArray<AActor*>& InCaptureActors, ESceneCaptureSource CaptureSource, UMaterialInterface* InOverrideMaterial, const FMinimalViewInfo& CaptureView) const;
	TArray<UPrimitiveComponent*> GatherPrimitivesForCapture(const TArray<AActor*>& InCaptureActors) const;
    
private:
	UPROPERTY(VisibleAnywhere, Category="Dungeon")
	int32 SurfaceWidth = 1;

	UPROPERTY(VisibleAnywhere, Category="Dungeon")
	int32 SurfaceHeight = 1;
    
	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> AlphaMaskRT;
    
	UPROPERTY(VisibleAnywhere, Category="Dungeon")
	TArray<TWeakObjectPtr<AActor>> AlphaMaskActorPtrs;
    
	UPROPERTY()
	TObjectPtr<USceneCaptureComponent2D> CaptureComponent;
    
	UPROPERTY(Transient)
	TObjectPtr<UWorld> PrivateWorld;
};

