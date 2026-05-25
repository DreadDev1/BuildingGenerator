//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/DungeonCanvasPlayerControllerComponent.h"

#include "Core/Dungeon.h"
#include "Frameworks/Canvas/DungeonCanvas.h"
#include "Frameworks/Canvas/DungeonCanvasTrackedObject.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

UDungeonCanvasPlayerControllerComponent::UDungeonCanvasPlayerControllerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bWantsInitializeComponent = true;
	SetIsReplicatedByDefault(true);
}

void UDungeonCanvasPlayerControllerComponent::InitializeComponent() {
	Super::InitializeComponent();
	
	if (APlayerController* PlayerController = GetController<APlayerController>()) {
		PlayerController->OnPossessedPawnChanged.AddDynamic(this, &UDungeonCanvasPlayerControllerComponent::OnPossess);
	}
}

void UDungeonCanvasPlayerControllerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);
	if (APlayerController* PlayerController = GetController<APlayerController>()) {
		PlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &UDungeonCanvasPlayerControllerComponent::OnPossess);
	}
	
}

void UDungeonCanvasPlayerControllerComponent::ReceivedPlayer() {
	Super::ReceivedPlayer();
	
}

void UDungeonCanvasPlayerControllerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UDungeonCanvasPlayerControllerComponent, ReplicatedPawn);
}

void UDungeonCanvasPlayerControllerComponent::OnRep_Pawn() {
	if (ReplicatedPawn) {
		AddPlayerToCanvas(ReplicatedPawn);
	}
}

void UDungeonCanvasPlayerControllerComponent::OnPossess(APawn* OldPawn, APawn* NewPawn) {
	if (HasAuthority()) {
		ReplicatedPawn = NewPawn;
	}
	if (NewPawn && IsLocalController() && !NewPawn->IsA<ASpectatorPawn>()) {
		AddPlayerToCanvas(NewPawn);
	}
}

void UDungeonCanvasPlayerControllerComponent::AddPlayerToCanvas(APawn* Pawn) {
	if (Pawn) {
		// Setup the player
		if (ADungeon* DungeonActor = Cast<ADungeon>(UGameplayStatics::GetActorOfClass(Pawn->GetWorld(), ADungeon::StaticClass()))) {
			if (UDungeonCanvasComponent* DungeonCanvasComponent = DungeonActor->GetComponentByClass<UDungeonCanvasComponent>()) {
				DungeonCanvasComponent->AddFogOfWarExplorer(Pawn, FogOfWarSettings);

				UDungeonCanvasTrackedObject* TrackedObject = Pawn->GetComponentByClass<UDungeonCanvasTrackedObject>();
				if (!TrackedObject) {
					TrackedObject = Cast<UDungeonCanvasTrackedObject>(Pawn->AddComponentByClass(UDungeonCanvasTrackedObject::StaticClass(), false, FTransform::Identity, false));
					if (TrackedObject) {
						ManagedComponents.Add(TrackedObject);
					}
				}

				if (TrackedObject) {
					TrackedObject->IconName = IconName;
					TrackedObject->bOrientToRotation = bOrientToRotation;
					TrackedObject->ZOrder = ZOrder;
					TrackedObject->bOccludesFogOfWar = false;
					TrackedObject->bHideWhenOutOfSight = false;
				}
			}
		}
	}
}

void UDungeonCanvasPlayerControllerComponent::RemovePlayerFromCanvas(APawn* Pawn) {
	// Remove the managed component
	UDungeonCanvasTrackedObject* TrackedObject = Pawn->GetComponentByClass<UDungeonCanvasTrackedObject>();
	if (!TrackedObject && ManagedComponents.Contains(TrackedObject)) {
		ManagedComponents.Remove(TrackedObject);
		TrackedObject->DestroyComponent();
	}
}

