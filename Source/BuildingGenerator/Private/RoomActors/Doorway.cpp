// Fill out your copyright notice in the Description page of Project Settings.

#include "RoomActors/Doorway.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"

ADoorway::ADoorway()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;

    // ========================================================================
    // CREATE COMPONENTS
    // ========================================================================

    // Root component
    RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(RootComp);

    // Door frame mesh
    FrameMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
    FrameMeshComponent->SetupAttachment(RootComp);
    FrameMeshComponent->SetCollisionEnabled(ECollisionEnabled:: QueryAndPhysics);
    FrameMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);

    // Left side fill (optional)
    LeftSideMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftSideMesh"));
    LeftSideMeshComponent->SetupAttachment(RootComp);
    LeftSideMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    LeftSideMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);

    // Right side fill (optional)
    RightSideMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightSideMesh"));
    RightSideMeshComponent->SetupAttachment(RootComp);
    RightSideMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    RightSideMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);

    // Interaction box
    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    InteractionBox->SetupAttachment(RootComp);
    InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    InteractionBox->SetBoxExtent(FVector(150.0f, 150.0f, 200.0f));  // Default size

    // Bind overlap events
    InteractionBox->OnComponentBeginOverlap. AddDynamic(this, &ADoorway::OnInteractionBoxBeginOverlap);
    InteractionBox->OnComponentEndOverlap.AddDynamic(this, &ADoorway::OnInteractionBoxEndOverlap);
}

void ADoorway::BeginPlay()
{
    Super::BeginPlay();

    // Setup visuals if DoorData is assigned
    if (DoorData)
    {
        SetupVisuals();
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void ADoorway::InitializeDoorway(UDoorData* InDoorData, EWallEdge InWallEdge, bool bInIsStandard)
{
    DoorData = InDoorData;
    WallEdge = InWallEdge;
    bIsStandardDoorway = bInIsStandard;

    if (DoorData)
    {
        SetupVisuals();
    }
}

void ADoorway::SetupVisuals()
{
    if (!DoorData)
    {
        UE_LOG(LogTemp, Warning, TEXT("ADoorway:: SetupVisuals - No DoorData assigned! "));
        return;
    }

    // ========================================================================
    // SETUP DOOR FRAME
    // ========================================================================

    UStaticMesh* FrameMesh = DoorData->FrameSideMesh. LoadSynchronous();
    if (FrameMesh)
    {
        FrameMeshComponent->SetStaticMesh(FrameMesh);
        
        // Apply rotation offset from DoorData
        FrameMeshComponent->SetRelativeRotation(DoorData->FrameRotationOffset);
        
        UE_LOG(LogTemp, Log, TEXT("ADoorway::SetupVisuals - Frame mesh set"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ADoorway::SetupVisuals - Failed to load frame mesh"));
    }

    // ========================================================================
    // SETUP SIDE FILLS
    // ========================================================================

    SetupSideFills();

    // ========================================================================
    // SETUP INTERACTION BOX
    // ========================================================================

    if (DoorData->ConnectionBoxExtent != FVector::ZeroVector)
    {
        InteractionBox->SetBoxExtent(DoorData->ConnectionBoxExtent);
    }
}

void ADoorway::SetupSideFills()
{
    if (!DoorData)
    {
        return;
    }

    float CellSize = 100.0f;  // CELL_SIZE constant
    int32 FrameWidth = DoorData->FrameFootprintY;

    // Calculate side fill positions (1 cell each side)
    float SideOffset = (FrameWidth / 2.0f + 0.5f) * CellSize;  // Half frame + half cell

    // ========================================================================
    // SETUP LEFT SIDE FILL
    // ========================================================================

    switch (DoorData->SideFillType)
    {
        case EDoorwaySideFill::CustomMeshes:
        {
            // Load left side mesh
            UStaticMesh* LeftMesh = DoorData->LeftSideMesh.LoadSynchronous();
            if (LeftMesh)
            {
                LeftSideMeshComponent->SetStaticMesh(LeftMesh);
                LeftSideMeshComponent->SetRelativeLocation(FVector(0, -SideOffset, 0));  // Left = negative Y
                LeftSideMeshComponent->SetVisibility(true);
                
                UE_LOG(LogTemp, Log, TEXT("ADoorway::SetupSideFills - Left side mesh set"));
            }
            else
            {
                LeftSideMeshComponent->SetVisibility(false);
            }

            // Load right side mesh
            UStaticMesh* RightMesh = DoorData->RightSideMesh. LoadSynchronous();
            if (RightMesh)
            {
                RightSideMeshComponent->SetStaticMesh(RightMesh);
                RightSideMeshComponent->SetRelativeLocation(FVector(0, SideOffset, 0));  // Right = positive Y
                RightSideMeshComponent->SetVisibility(true);
                
                UE_LOG(LogTemp, Log, TEXT("ADoorway::SetupSideFills - Right side mesh set"));
            }
            else
            {
                RightSideMeshComponent->SetVisibility(false);
            }
            break;
        }

        case EDoorwaySideFill::WallModules:
        {
            // TODO: Implement wall module side fills
            // For now, hide side components
            LeftSideMeshComponent->SetVisibility(false);
            RightSideMeshComponent->SetVisibility(false);
            UE_LOG(LogTemp, Warning, TEXT("ADoorway::SetupSideFills - WallModules not yet implemented"));
            break;
        }

        case EDoorwaySideFill:: CornerPieces:
        {
            // TODO: Implement corner piece side fills
            LeftSideMeshComponent->SetVisibility(false);
            RightSideMeshComponent->SetVisibility(false);
            UE_LOG(LogTemp, Warning, TEXT("ADoorway::SetupSideFills - CornerPieces not yet implemented"));
            break;
        }

        case EDoorwaySideFill::None:
        default: 
        {
            // No side fills - hide components
            LeftSideMeshComponent->SetVisibility(false);
            RightSideMeshComponent->SetVisibility(false);
            break;
        }
    }
}

// ============================================================================
// INTERACTION
// ============================================================================

void ADoorway::OnInteractionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor != this)
    {
        UE_LOG(LogTemp, Log, TEXT("ADoorway:: OnInteractionBoxBeginOverlap - Actor entered:  %s"), *OtherActor->GetName());
        
        // Call Blueprint event
        OnActorEnterRange(OtherActor);
    }
}

void ADoorway::OnInteractionBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor && OtherActor != this)
    {
        UE_LOG(LogTemp, Log, TEXT("ADoorway::OnInteractionBoxEndOverlap - Actor exited: %s"), *OtherActor->GetName());
        
        // Call Blueprint event
        OnActorExitRange(OtherActor);
    }
}

// ============================================================================
// DOOR STATE FUNCTIONS
// ============================================================================

void ADoorway::OpenDoor()
{
    if (bIsLocked)
    {
        UE_LOG(LogTemp, Warning, TEXT("ADoorway::OpenDoor - Door is locked!"));
        return;
    }

    if (! bIsOpen)
    {
        bIsOpen = true;
        
        UE_LOG(LogTemp, Log, TEXT("ADoorway::OpenDoor - Door opened"));
        
        // Call Blueprint event
        OnDoorOpened();
    }
}

void ADoorway:: CloseDoor()
{
    if (bIsOpen)
    {
        bIsOpen = false;
        
        UE_LOG(LogTemp, Log, TEXT("ADoorway:: CloseDoor - Door closed"));
        
        // Call Blueprint event
        OnDoorClosed();
    }
}

void ADoorway::ToggleDoor()
{
    if (bIsOpen)
    {
        CloseDoor();
    }
    else
    {
        OpenDoor();
    }
}

void ADoorway::OnRep_IsOpen()
{
    // Handle replication of door state
    if (bIsOpen)
    {
        OnDoorOpened();
    }
    else
    {
        OnDoorClosed();
    }
}

// ============================================================================
// REPLICATION
// ============================================================================

void ADoorway::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ADoorway, DoorData);
    DOREPLIFETIME(ADoorway, WallEdge);
    DOREPLIFETIME(ADoorway, bIsOpen);
    DOREPLIFETIME(ADoorway, bIsLocked);
}