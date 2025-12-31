// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Room/DoorData.h"
#include "Doorway.generated.h"

/* Forward declarations */
class UBoxComponent;
class UStaticMeshComponent;
class USceneComponent;

/**
 * ADoorwayActor - Interactive doorway with frame and side fills
 * 
 * Hierarchy:
 *   Root (SceneComponent)
 *   ├─ FrameMesh (StaticMeshComponent)
 *   ├─ LeftSideMesh (StaticMeshComponent)  [Optional]
 *   ├─ RightSideMesh (StaticMeshComponent) [Optional]
 *   └─ InteractionBox (BoxComponent)
 * 
 * Configured by DoorData asset (meshes, side fills, etc.)
 */
UCLASS(Blueprintable, ClassGroup = (Dungeon))
class BUILDINGGENERATOR_API ADoorway :  public AActor
{
    GENERATED_BODY()

public:
    ADoorway();

protected:
    virtual void BeginPlay() override;

public:
    // ========================================================================
    // COMPONENTS
    // ========================================================================

    /* Root component for positioning */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootComp;

    /* Door frame mesh component */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* FrameMeshComponent;

    /* Left side fill mesh (optional) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* LeftSideMeshComponent;

    /* Right side fill mesh (optional) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* RightSideMeshComponent;

    /* Interaction/overlap trigger box */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* InteractionBox;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /* Door data asset (defines meshes, side fills, etc.) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doorway Config", Replicated)
    UDoorData* DoorData;

    /* Wall edge this doorway is on */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doorway Config", Replicated)
    EWallEdge WallEdge = EWallEdge::North;

    /* Whether this is a standard (automatic) doorway */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doorway Config")
    bool bIsStandardDoorway = true;

    // ========================================================================
    // DOORWAY STATE
    // ========================================================================

    /* Is the door currently open? */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doorway State", Replicated, ReplicatedUsing = OnRep_IsOpen)
    bool bIsOpen = false;

    /* Is the door locked? */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Doorway State", Replicated)
    bool bIsLocked = false;

    // ========================================================================
    // INITIALIZATION
    // ========================================================================

    /* Initialize doorway with configuration data
     * Called by RoomSpawner after spawning */
    UFUNCTION(BlueprintCallable, Category = "Doorway")
    void InitializeDoorway(UDoorData* InDoorData, EWallEdge InWallEdge, bool bInIsStandard);

    /* Setup visual components (frame + side fills) based on DoorData */
    UFUNCTION(BlueprintCallable, Category = "Doorway")
    void SetupVisuals();

    /* Setup side fill meshes (left/right) */
    UFUNCTION(BlueprintCallable, Category = "Doorway")
    void SetupSideFills();

    // ========================================================================
    // INTERACTION
    // ========================================================================

    /* Overlap events for interaction trigger */
    UFUNCTION()
    void OnInteractionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnInteractionBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    // ========================================================================
    // DOOR STATE FUNCTIONS
    // ========================================================================

    /* Open the door */
    UFUNCTION(BlueprintCallable, Category = "Doorway")
    void OpenDoor();

    /* Close the door */
    UFUNCTION(BlueprintCallable, Category = "Doorway")
    void CloseDoor();

    /* Toggle door open/closed */
    UFUNCTION(BlueprintCallable, Category = "Doorway")
    void ToggleDoor();

    /* Replication callback for door state */
    UFUNCTION()
    void OnRep_IsOpen();

    // ========================================================================
    // BLUEPRINT EVENTS
    // ========================================================================

    /* Blueprint event called when door opens */
    UFUNCTION(BlueprintImplementableEvent, Category = "Doorway Events")
    void OnDoorOpened();

    /* Blueprint event called when door closes */
    UFUNCTION(BlueprintImplementableEvent, Category = "Doorway Events")
    void OnDoorClosed();

    /* Blueprint event called when actor enters interaction range */
    UFUNCTION(BlueprintImplementableEvent, Category = "Doorway Events")
    void OnActorEnterRange(AActor* Actor);

    /* Blueprint event called when actor leaves interaction range */
    UFUNCTION(BlueprintImplementableEvent, Category = "Doorway Events")
    void OnActorExitRange(AActor* Actor);

    // ========================================================================
    // REPLICATION
    // ========================================================================

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};