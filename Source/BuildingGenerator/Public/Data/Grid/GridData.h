// GridData.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "GridData.generated.h"

// --- CORE CONSTANT DEFINITION ---
static constexpr float CELL_SIZE = 100.0f;

// Defines the content type of a 100cm grid cell
UENUM(BlueprintType)
enum class EGridCellType : uint8
{
	ECT_Empty 		UMETA(DisplayName = "Empty"),
	ECT_FloorMesh 	UMETA(DisplayName = "Floor Mesh"),
	ECT_WallMesh 	UMETA(DisplayName = "Wall Boundary"),
	ECT_Doorway 	UMETA(DisplayName = "Doorway Cell"),
	
	ECT_Reserved    UMETA(DisplayName = "Reserved Cell"),
	ECT_Custom      UMETA(DisplayName = "Custom Cell"),
	ECT_Void        UMETA(DisplayName = "Void Cell"),
};












