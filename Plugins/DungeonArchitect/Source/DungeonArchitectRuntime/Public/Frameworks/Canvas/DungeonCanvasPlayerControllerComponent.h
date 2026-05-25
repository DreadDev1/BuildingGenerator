//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Canvas/DungeonCanvas.h"

#include "Components/ControllerComponent.h"
#include "DungeonCanvasPlayerControllerComponent.generated.h"

class UActorComponent;
class APawn;

/**
 * Add this to your player controller to take care of setting up the player icon and fog of war on your canvas
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class UDungeonCanvasPlayerControllerComponent : public UControllerComponent {
	GENERATED_BODY()
public:
	UDungeonCanvasPlayerControllerComponent(const FObjectInitializer& ObjectInitializer);

	virtual void InitializeComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void ReceivedPlayer() override;

	UPROPERTY(EditAnywhere, Category="Dungeon Canvas")
	FDungeonCanvasItemFogOfWarSettings FogOfWarSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	FName IconName = "Player";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	bool bOrientToRotation = true;

	/** Higher numbers show up on top when overlapped with other icons. E.g. if you want your player icon to be drawn on top of everything else, bump this number up (e.g. 1000) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	int32 ZOrder = 0;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
protected:
	// The replicated pawn
	UPROPERTY(ReplicatedUsing = OnRep_Pawn)
	TObjectPtr<APawn> ReplicatedPawn;

	// This function is called on the client when the ReplicatedPawn is updated
	UFUNCTION()
	void OnRep_Pawn();
	
private:
	UFUNCTION()
	void OnPossess(APawn* OldPawn, APawn* NewPawn);

	void AddPlayerToCanvas(APawn* Pawn);
	void RemovePlayerFromCanvas(APawn* Pawn);

private:
	UPROPERTY()
	TArray<TObjectPtr<UActorComponent>> ManagedComponents;
};

