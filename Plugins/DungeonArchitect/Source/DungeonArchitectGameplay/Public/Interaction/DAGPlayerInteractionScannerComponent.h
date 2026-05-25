//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "Engine/CollisionProfile.h"
#include "DAGPlayerInteractionScannerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDAG_InteractableObjectsChangedEvent, const TArray<FDAG_InteractionOption>&, InteractableOptions);

struct FDAG_InteractionQuery;
class IDAG_InteractableTarget;

UCLASS()
class UDAG_PlayerInteractionScannerComponent : public UControllerComponent {
	GENERATED_BODY()
public:
	UDAG_PlayerInteractionScannerComponent(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category="Interaction")
	void TriggerInteraction();
	
public:
	UPROPERTY(EditAnywhere, Category="Interaction")
	float InteractionScanRange = 200;
	
	UPROPERTY(EditAnywhere, Category="Interaction")
	float InteractionScanRate = 0.100;
	
	UPROPERTY(EditAnywhere, Category="Interaction")
	bool bShowDebug = false;

	UPROPERTY(EditAnywhere, Category="Interaction")
	FCollisionProfileName TraceProfile;

	// Does the trace affect the aiming pitch
	UPROPERTY(EditAnywhere, Category="Interaction")
	bool bTraceAffectsAimPitch = false;

	UPROPERTY(BlueprintAssignable, Category="Interaction")
	FDAG_InteractableObjectsChangedEvent InteractableObjectsChanged;

private:
	void PerformTrace();
	void AimWithPlayerController(const AActor* InSourceActor, FCollisionQueryParams Params, const FVector& TraceStart, float MaxRange, FVector& OutTraceEnd, bool bIgnorePitch = false) const;
	void ShowAndUpdateInteractionWidget();
	
	static void LineTrace(FHitResult& OutHitResult, const UWorld* World, const FVector& Start, const FVector& End, FName ProfileName, const FCollisionQueryParams Params);
	static bool ClipCameraRayToAbilityRange(FVector CameraLocation, FVector CameraDirection, FVector AbilityCenter, float AbilityRange, FVector& ClippedPosition);
	void UpdateInteractableOptions(const FDAG_InteractionQuery& InteractQuery, const TArray<TScriptInterface<IDAG_InteractableTarget>>& InteractableTargets);
	void UpdateInteractions();

private:
	TArray<FDAG_InteractionOption> CurrentOptions;
	FTimerHandle TimerHandle;

	UPROPERTY()
	TObjectPtr<UUserWidget> InteractionWidget = nullptr;;

	/** The host that implements the interactable target. We use this to update the position of the interact widget on every tick */
	
};

