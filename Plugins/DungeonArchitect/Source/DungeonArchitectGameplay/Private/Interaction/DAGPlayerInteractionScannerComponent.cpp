//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Interaction/DAGPlayerInteractionScannerComponent.h"

#include "Interaction/DAGInteractableTarget.h"
#include "Interaction/DAGInteractionQuery.h"
#include "Interaction/DAGInteractionStatics.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogInteractionScanner, Log, All);

UDAG_PlayerInteractionScannerComponent::UDAG_PlayerInteractionScannerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TraceProfile(FCollisionProfileName(TEXT("BlockAll")))
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UDAG_PlayerInteractionScannerComponent::BeginPlay() {
	Super::BeginPlay();
	
	UWorld* World = GetWorld();
	World->GetTimerManager().SetTimer(TimerHandle, this, &ThisClass::PerformTrace, InteractionScanRate, true);
}

void UDAG_PlayerInteractionScannerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);
	
	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(TimerHandle);
	}
}

void UDAG_PlayerInteractionScannerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	ShowAndUpdateInteractionWidget();
}

void UDAG_PlayerInteractionScannerComponent::TriggerInteraction() {
	if (CurrentOptions.Num() == 0) {
		return;
	}

	for (const FDAG_InteractionOption& InteractionOption : CurrentOptions) {
		if (UObject* Object = InteractionOption.InteractableTarget.GetObject()) {
			if (AActor* Actor = Cast<AActor>(Object)) {
				IDAG_InteractableTarget::Execute_Interact(Actor, GetOwner());
			}
			else if (UActorComponent* ActorComponent = Cast<UActorComponent>(Object)) {
				IDAG_InteractableTarget::Execute_Interact(ActorComponent, GetOwner());
			}
		}
	}
}

void UDAG_PlayerInteractionScannerComponent::PerformTrace() {
	APawn* AvatarActor = GetPawnOrViewTarget<APawn>();
	if (!AvatarActor) {
		return;
	}

	UWorld* World = GetWorld();

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(AvatarActor);

	const bool bTraceComplex = false;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UAbilityTask_WaitForInteractableTargets_SingleLineTrace), bTraceComplex);
	Params.AddIgnoredActors(ActorsToIgnore);

	FVector TraceStart = AvatarActor->GetActorLocation();
	FVector TraceEnd;
	AimWithPlayerController(AvatarActor, Params, TraceStart, InteractionScanRange, OUT TraceEnd);

	FHitResult OutHitResult;
	LineTrace(OutHitResult, World, TraceStart, TraceEnd, TraceProfile.Name, Params);

	TArray<TScriptInterface<IDAG_InteractableTarget>> InteractableTargets;
	UDAG_InteractionStatics::AppendDAInteractableTargetsFromHitResult(OutHitResult, InteractableTargets);

	FDAG_InteractionQuery InteractionQuery;
	InteractionQuery.RequestingAvatar = AvatarActor;
	InteractionQuery.RequestingController = GetController<AController>(); 
	UpdateInteractableOptions(InteractionQuery, InteractableTargets);

	ShowAndUpdateInteractionWidget();
	
#ifdef ENABLE_DRAW_DEBUG
	if (bShowDebug)
	{
		FColor DebugColor = OutHitResult.bBlockingHit ? FColor::Red : FColor::Green;
		if (OutHitResult.bBlockingHit)
		{
			DrawDebugLine(World, TraceStart, OutHitResult.Location, DebugColor, false, InteractionScanRate);
			DrawDebugSphere(World, OutHitResult.Location, 5, 16, DebugColor, false, InteractionScanRate);
		}
		else
		{
			DrawDebugLine(World, TraceStart, TraceEnd, DebugColor, false, InteractionScanRate);
		}
	}
#endif // ENABLE_DRAW_DEBUG
}

void UDAG_PlayerInteractionScannerComponent::ShowAndUpdateInteractionWidget() {
	auto RemoveWidget = [this]() {
		if (InteractionWidget) {
			InteractionWidget->RemoveFromParent();
			InteractionWidget = nullptr;
		}
	};
	
	TSoftClassPtr<UUserWidget> WidgetClassPtr = CurrentOptions.Num() > 0 ? CurrentOptions[0].InteractionWidgetClass : nullptr;
	TSubclassOf<UUserWidget> WidgetClass = WidgetClassPtr.LoadSynchronous();
	if (!WidgetClass) {
		RemoveWidget();
		return;
	}
	AActor* InteractableActor = UDAG_InteractionStatics::GetActorFromDAInteractableTarget(CurrentOptions[0].InteractableTarget);
	if (!InteractableActor) {
		RemoveWidget();
		return;
	}

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController) {
		RemoveWidget();
		return;
	}

	const UDAG_InteractableTargetPivotPoint* PivotPoint = InteractableActor->FindComponentByClass<UDAG_InteractableTargetPivotPoint>();
	const USceneComponent* InteractionHostComponent = PivotPoint ? PivotPoint : InteractableActor->GetRootComponent();
	
	const FVector WorldLocation = InteractionHostComponent->GetComponentLocation();
	
	FVector2D ScreenPosition;
	const bool bIsOnScreen = UGameplayStatics::ProjectWorldToScreen(PlayerController, WorldLocation, ScreenPosition);
	if (bIsOnScreen) {
		if (!InteractionWidget || InteractionWidget->GetClass() != WidgetClass) {
			if (InteractionWidget) {
				InteractionWidget->RemoveFromParent();
			}
			InteractionWidget = CreateWidget<UUserWidget>(PlayerController, WidgetClass);
			if (InteractionWidget) {
				InteractionWidget->AddToViewport();
			}
		}

		if (InteractionWidget) {
			InteractionWidget->SetPositionInViewport(ScreenPosition);
		}
	}
	else if (InteractionWidget) {
		RemoveWidget();
	}
}

void UDAG_PlayerInteractionScannerComponent::AimWithPlayerController(const AActor* InSourceActor, FCollisionQueryParams Params, const FVector& TraceStart, float MaxRange, FVector& OutTraceEnd, bool bIgnorePitch) const {
	APlayerController* PC = GetController<APlayerController>();
	if (!PC) {
		return;
	}

	FVector ViewStart;
	FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewStart, ViewRot);

	const FVector ViewDir = ViewRot.Vector();
	FVector ViewEnd = ViewStart + (ViewDir * MaxRange);

	ClipCameraRayToAbilityRange(ViewStart, ViewDir, TraceStart, MaxRange, ViewEnd);

	FHitResult HitResult;
	LineTrace(HitResult, InSourceActor->GetWorld(), ViewStart, ViewEnd, TraceProfile.Name, Params);

	const bool bUseTraceResult = HitResult.bBlockingHit && (FVector::DistSquared(TraceStart, HitResult.Location) <= (MaxRange * MaxRange));

	const FVector AdjustedEnd = (bUseTraceResult) ? HitResult.Location : ViewEnd;

	FVector AdjustedAimDir = (AdjustedEnd - TraceStart).GetSafeNormal();
	if (AdjustedAimDir.IsZero()) {
		AdjustedAimDir = ViewDir;
	}

	if (!bTraceAffectsAimPitch && bUseTraceResult) {
		FVector OriginalAimDir = (ViewEnd - TraceStart).GetSafeNormal();

		if (!OriginalAimDir.IsZero()) {
			// Convert to angles and use original pitch
			const FRotator OriginalAimRot = OriginalAimDir.Rotation();

			FRotator AdjustedAimRot = AdjustedAimDir.Rotation();
			AdjustedAimRot.Pitch = OriginalAimRot.Pitch;

			AdjustedAimDir = AdjustedAimRot.Vector();
		}
	}

	OutTraceEnd = TraceStart + (AdjustedAimDir * MaxRange);
}

void UDAG_PlayerInteractionScannerComponent::LineTrace(FHitResult& OutHitResult, const UWorld* World, const FVector& Start,
                                                   const FVector& End, FName ProfileName, const FCollisionQueryParams Params) {
	check(World);

	OutHitResult = FHitResult();
	TArray<FHitResult> HitResults;
	World->LineTraceMultiByProfile(HitResults, Start, End, ProfileName, Params);

	OutHitResult.TraceStart = Start;
	OutHitResult.TraceEnd = End;

	if (HitResults.Num() > 0) {
		OutHitResult = HitResults[0];
	}
}

bool UDAG_PlayerInteractionScannerComponent::ClipCameraRayToAbilityRange(FVector CameraLocation, FVector CameraDirection,
		FVector AbilityCenter, float AbilityRange, FVector& ClippedPosition)
{
	FVector CameraToCenter = AbilityCenter - CameraLocation;
	float DotToCenter = FVector::DotProduct(CameraToCenter, CameraDirection);
	if (DotToCenter >= 0) {		//If this fails, we're pointed away from the center, but we might be inside the sphere and able to find a good exit point.
		float DistanceSquared = CameraToCenter.SizeSquared() - (DotToCenter * DotToCenter);
		float RadiusSquared = (AbilityRange * AbilityRange);
		if (DistanceSquared <= RadiusSquared) {
			float DistanceFromCamera = FMath::Sqrt(RadiusSquared - DistanceSquared);
			float DistanceAlongRay = DotToCenter + DistanceFromCamera;						//Subtracting instead of adding will get the other intersection point
			ClippedPosition = CameraLocation + (DistanceAlongRay * CameraDirection);		//Cam aim point clipped to range sphere
			return true;
		}
	}
	return false;
}

void UDAG_PlayerInteractionScannerComponent::UpdateInteractableOptions(const FDAG_InteractionQuery& InteractQuery,
		const TArray<TScriptInterface<IDAG_InteractableTarget>>& InteractableTargets)
{
	TArray<FDAG_InteractionOption> NewOptions;

	for (const TScriptInterface<IDAG_InteractableTarget>& InteractiveTarget : InteractableTargets) {
		IDAG_InteractableTarget::Execute_GatherInteractionOptions(InteractiveTarget.GetObject(), InteractQuery, NewOptions);
	}

	bool bOptionsChanged = false;
	if (NewOptions.Num() == CurrentOptions.Num()) {
		NewOptions.Sort();

		for (int OptionIndex = 0; OptionIndex < NewOptions.Num(); OptionIndex++) {
			const FDAG_InteractionOption& NewOption = NewOptions[OptionIndex];
			const FDAG_InteractionOption& CurrentOption = CurrentOptions[OptionIndex];

			if (NewOption != CurrentOption) {
				bOptionsChanged = true;
				break;
			}
		}
	}
	else
	{
		bOptionsChanged = true;
	}

	if (bOptionsChanged)
	{
		CurrentOptions = NewOptions;
		UpdateInteractions();
		InteractableObjectsChanged.Broadcast(CurrentOptions);
	}
}

void UDAG_PlayerInteractionScannerComponent::UpdateInteractions() {
	// TODO: Grab the indicator manager and show the interaction message widget on screen
	UE_LOG(LogInteractionScanner, Log, TEXT("Updating interactions"));
}

