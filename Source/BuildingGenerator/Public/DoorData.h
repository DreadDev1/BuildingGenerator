#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/StaticMesh.h"
#include "Sound/SoundBase.h"
#include "DoorData.generated.h"

UENUM(BlueprintType)
enum class EDoorInteraction : uint8
{
	AutoTrigger		UMETA(DisplayName = "Auto Trigger"),
	RequiresInput	UMETA(DisplayName = "Requires Input")
};

// Width of the door opening in cells. TwoCell = 200cm, FourCell = 400cm.
// CellOffset in FDoorPlacement marks the first cell of the span.
// ColumnMesh is placed at CellOffset-1 and CellOffset+Width (the flanking cells).
UENUM(BlueprintType)
enum class EDoorWidth : uint8
{
	TwoCell  UMETA(DisplayName = "Two Cell (200cm)"),
	FourCell UMETA(DisplayName = "Four Cell (400cm)")
};

UCLASS(BlueprintType)
class BUILDINGGENERATOR_API UDoorData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoorData|Mesh")
	TObjectPtr<UStaticMesh> DoorMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoorData|Mesh")
	TObjectPtr<UStaticMesh> FrameMesh;

	// Placed at the cell immediately to the left (CellOffset-1) and right (CellOffset+Width)
	// of the door opening, replacing the wall module at those cells. Both sides use the same mesh.
	// Leave null to place wall modules at the flanking cells instead.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoorData|Mesh")
	TObjectPtr<UStaticMesh> ColumnMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoorData|Layout")
	EDoorWidth DoorWidth = EDoorWidth::TwoCell;

	// When false (default), the cells flanking the door opening (CellOffset-1 and CellOffset+DoorWidth)
	// are not reserved — they receive normal wall modules from WallData.
	// When true, those cells are reserved for ColumnMesh. If ColumnMesh is null the cell is left empty.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoorData|Layout")
	bool bUseColumns = false;

	// Applied in wall-forward space (X = depth into room, Y = along face, Z = up).
	// Corrects for door meshes whose interior face does not land flush on the wall boundary.
	// Example: door mesh sits 10cm too far back → set X = 10 to push it forward.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoorData|Placement")
	FVector DoorPlacementOffset = FVector::ZeroVector;

	// Same convention as DoorPlacementOffset, applied to ColumnMesh only.
	// Set independently when the column mesh needs a different depth correction than the door mesh.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoorData|Placement")
	FVector ColumnPlacementOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoorData|Collision")
	FVector CollisionBoxExtent = FVector(50.f, 50.f, 100.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoorData|Interaction")
	EDoorInteraction InteractionType = EDoorInteraction::AutoTrigger;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoorData|Interaction")
	bool bIsExteriorDoor = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoorData|Audio")
	TObjectPtr<USoundBase> OpenSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DoorData|Audio")
	TObjectPtr<USoundBase> CloseSound;
};
