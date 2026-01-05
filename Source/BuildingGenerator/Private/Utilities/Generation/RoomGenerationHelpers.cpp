// Fill out your copyright notice in the Description page of Project Settings.

#include "Utilities/Generation/RoomGenerationHelpers.h"

#include "Data/Generation/RoomGenerationTypes.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"

#pragma region Grid & Cell Operations
TArray<FIntPoint> URoomGenerationHelpers::GetEdgeCellIndices(EWallEdge Edge, FIntPoint GridSize)
{
	TArray<FIntPoint> Cells;
	switch (Edge)
	{
	case EWallEdge::North:   // North = +X direction, X = GridSize (beyond max)
		for (int32 Y = 0; Y < GridSize.Y; ++Y) Cells.Add(FIntPoint(GridSize.X, Y)); break;
        
	case EWallEdge:: South:  // South = -X direction, X = -1 (before min)
		for (int32 Y = 0; Y < GridSize.Y; ++Y) Cells.Add(FIntPoint(-1, Y)); break;
        
	case EWallEdge::East:   // East = +Y direction, Y = GridSize (beyond max)
		for (int32 X = 0; X < GridSize.X; ++X) Cells.Add(FIntPoint(X, GridSize.Y)); break;
        
	case EWallEdge:: West:   // West = -Y direction, Y = -1 (before min)
		for (int32 X = 0; X < GridSize.X; ++X) Cells.Add(FIntPoint(X, -1)); break;
            
	default: 
		break;
	}
    
	return Cells;
}

bool URoomGenerationHelpers::IsValidGridCoordinate(FIntPoint Coord, FIntPoint GridSize)
{
	return Coord.X >= 0 && Coord.X < GridSize. X && Coord.Y >= 0 && Coord.Y < GridSize.Y;
}

FIntPoint URoomGenerationHelpers::IndexToCoordinate(int32 Index, int32 GridWidth)
{
	if (GridWidth <= 0) return FIntPoint:: ZeroValue;
	
	int32 X = Index / GridWidth;
	int32 Y = Index % GridWidth;
	return FIntPoint(X, Y);
}

int32 URoomGenerationHelpers::CoordinateToIndex(FIntPoint Coord, int32 GridWidth)
{
	return Coord.Y * GridWidth + Coord.X;
}
#pragma endregion

#pragma region Grid Placement Utilities
bool URoomGenerationHelpers::IsAreaAvailable(const TArray<EGridCellType>& GridState, FIntPoint GridSize,
	FIntPoint StartCoord, FIntPoint Size, EGridCellType RequiredType)
{
	// Check if entire area fits within grid
	if (StartCoord.X + Size.X > GridSize.X || StartCoord.Y + Size.Y > GridSize.Y)
		return false;

	// Check if start coordinate is valid
	if (!IsValidGridCoordinate(StartCoord, GridSize))
		return false;

	// Check if any cell in the area is not the required type
	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		for (int32 X = 0; X < Size.X; ++X)
		{
			FIntPoint CheckCoord(StartCoord.X + X, StartCoord.Y + Y);
			int32 Index = CoordinateToIndex(CheckCoord, GridSize. X);

			if (! GridState. IsValidIndex(Index))
				return false;

			if (GridState[Index] != RequiredType)
				return false;
		}
	}

	return true;
}

void URoomGenerationHelpers::MarkCellsOccupied(TArray<EGridCellType>& GridState, FIntPoint GridSize, FIntPoint StartCoord,
FIntPoint Size, EGridCellType CellType)
{
	// Mark all cells in area
	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		for (int32 X = 0; X < Size.X; ++X)
		{
			FIntPoint CellCoord(StartCoord. X + X, StartCoord. Y + Y);

			// Validate coordinate
			if (!IsValidGridCoordinate(CellCoord, GridSize))
				continue;

			int32 Index = CoordinateToIndex(CellCoord, GridSize.X);

			if (GridState.IsValidIndex(Index))
			{
				GridState[Index] = CellType;
			}
		}
	}
}

bool URoomGenerationHelpers::TryPlaceMeshInGrid(TArray<EGridCellType>& GridState, FIntPoint GridSize, FIntPoint StartCoord,
FIntPoint Size, EGridCellType TargetCellType, EGridCellType PlacementType)
{
	// Check if area is available (using the target type passed in)
	if (!IsAreaAvailable(GridState, GridSize, StartCoord, Size, TargetCellType)) return false;

	// Mark cells as occupied
	MarkCellsOccupied(GridState, GridSize, StartCoord, Size, PlacementType);

	return true;
}
#pragma endregion

#pragma region Rotation & Footprint Operations
FIntPoint URoomGenerationHelpers::GetRotatedFootprint(FIntPoint OriginalFootprint, int32 RotationDegrees)
{
	// Normalize rotation to 0-360 range
	RotationDegrees = RotationDegrees % 360;
	if (RotationDegrees < 0) RotationDegrees += 360;

	// 90° and 270° rotations swap X and Y
	if (RotationDegrees == 90 || RotationDegrees == 270)
	{ return FIntPoint(OriginalFootprint.Y, OriginalFootprint.X); }

	// 0° and 180° keep original dimensions
	return OriginalFootprint;
}

bool URoomGenerationHelpers::DoesRotationSwapDimensions(int32 RotationDegrees)
{
	RotationDegrees = RotationDegrees % 360;
	if (RotationDegrees < 0) RotationDegrees += 360;

	return (RotationDegrees == 90 || RotationDegrees == 270);
}
#pragma endregion

#pragma region Wall Edge Operations
FRotator URoomGenerationHelpers::GetWallRotationForEdge(EWallEdge Edge)
{
	// All walls face INWARD toward room interior
	switch (Edge)
	{
		case EWallEdge::North:  // X = Max, face South (-X, into room)
			return FRotator(0.0f, 180.0f, 0.0f);

		case EWallEdge:: South:  // X = 0, face North (+X, into room)
			return FRotator(0.0f, 0.0f, 0.0f);

		case EWallEdge::East:   // Y = Max, face West (-Y, into room)
			return FRotator(0.0f, 270.0f, 0.0f);

		case EWallEdge::West:   // Y = 0, face East (+Y, into room)
			return FRotator(0.0f, 90.0f, 0.0f);

		default:
			return FRotator::ZeroRotator;
	}
}

FVector URoomGenerationHelpers::CalculateWallPosition(EWallEdge Edge, int32 StartCell, int32 SpanLength, FIntPoint GridSize,
float CellSize, float NorthOffset, float SouthOffset, float EastOffset, float WestOffset)
{
	float HalfSpan = (SpanLength * CellSize) * 0.5f;
	FVector Position = FVector:: ZeroVector;

	switch (Edge)
	{
		case EWallEdge::North:  // North wall:  X = GridSize.X
		Position = FVector((GridSize.X * CellSize) + NorthOffset, (StartCell * CellSize) + HalfSpan,0.0f);
		break;

		case EWallEdge::South:  // South wall: X = 0
		Position = FVector(0.0f + SouthOffset,(StartCell * CellSize) + HalfSpan, 0.0f);
		break;

		case EWallEdge::East:   // East wall: Y = GridSize.Y
		Position = FVector((StartCell * CellSize) + HalfSpan, (GridSize.Y * CellSize) + EastOffset, 0.0f);
		break;

		case EWallEdge::West:   // West wall: Y = 0
		Position = FVector((StartCell * CellSize) + HalfSpan, 0.0f + WestOffset, 0.0f);
		break;

		default:
		break;
	}

	return Position;
}

FVector URoomGenerationHelpers::CalculateDoorwayPosition(EWallEdge Edge, int32 StartCell, 
int32 WidthInCells,	FIntPoint GridSize, float CellSize)
{
	// Calculate center of doorway span
	float DoorwayCenter = (StartCell + WidthInCells / 2.0f) * CellSize;
    
	FVector Position;
    
	switch (Edge)
	{
	case EWallEdge::North:
		// North edge: X = GridSize (boundary), Y varies
		Position = FVector(GridSize.X * CellSize, DoorwayCenter, 0.0f);
		break;
            
	case EWallEdge::South:
		// South edge: X = 0 (boundary), Y varies
		Position = FVector(0.0f, DoorwayCenter, 0.0f);
		break;
            
	case EWallEdge::East:
		// East edge: X varies, Y = GridSize (boundary)
		Position = FVector(DoorwayCenter, GridSize.Y * CellSize, 0.0f);
		break;
            
	case EWallEdge::West:
		// West edge: X varies, Y = 0 (boundary)
		Position = FVector(DoorwayCenter, 0.0f, 0.0f);
		break;
            
	default:
		Position = FVector::ZeroVector;
		break;
	}
    
	return Position;
}
#pragma endregion

#pragma region Mesh Operations
UStaticMesh* URoomGenerationHelpers::LoadAndValidateMesh( const TSoftObjectPtr<UStaticMesh>& MeshAsset, 
const FString& ContextName, bool bLogWarning)
{
	if (MeshAsset.IsNull())
	{
		if (bLogWarning)
		{ UE_LOG(LogTemp, Warning, TEXT("LoadAndValidateMesh: Null mesh asset for context '%s'"), *ContextName); }
		return nullptr;
	}

	UStaticMesh* Mesh = MeshAsset.LoadSynchronous();
	if (!Mesh && bLogWarning)
	{ UE_LOG(LogTemp, Warning, TEXT("LoadAndValidateMesh: Failed to load mesh for context '%s'"), *ContextName); }
	return Mesh;
}

FTransform URoomGenerationHelpers::CalculateMeshTransform(FIntPoint GridPosition, FIntPoint MeshSize, float CellSize,
int32 Rotation,	float ZOffset)
{
	// Calculate center of mesh footprint
	float OffsetX = (MeshSize.X * CellSize) * 0.5f;
	float OffsetY = (MeshSize.Y * CellSize) * 0.5f;

	FVector LocalPos = FVector(GridPosition.X * CellSize + OffsetX,	GridPosition.Y * CellSize + OffsetY, ZOffset);

	FRotator MeshRotation(0.0f, Rotation, 0.0f);

	return FTransform(MeshRotation, LocalPos, FVector:: OneVector);
	
}
#pragma endregion

#pragma region Transform Operations
bool URoomGenerationHelpers:: GetMeshSocketTransform(
UStaticMesh* Mesh, FName SocketName, FVector& OutLocation, FRotator& OutRotation)
{
	if (!Mesh) return false;

	UStaticMeshSocket* Socket = Mesh->FindSocket(SocketName);
	if (Socket)
	{
		OutLocation = Socket->RelativeLocation;
		OutRotation = Socket->RelativeRotation;
		return true;
	}

	return false;
}

bool URoomGenerationHelpers:: GetMeshSocketTransformWithFallback(UStaticMesh* Mesh, FName SocketName, 
FVector& OutLocation, FRotator& OutRotation, FVector FallbackLocation, FRotator FallbackRotation)
{
	if (GetMeshSocketTransform(Mesh, SocketName, OutLocation, OutRotation)) return true;

	// Use fallback
	OutLocation = FallbackLocation;
	OutRotation = FallbackRotation;
	return false;
}

FTransform URoomGenerationHelpers::CalculateSocketWorldTransform(UStaticMesh* Mesh, FName SocketName,
const FTransform& ParentTransform, FVector FallbackOffset)
{
	FVector SocketLocation;
	FRotator SocketRotation;

	if (! GetMeshSocketTransform(Mesh, SocketName, SocketLocation, SocketRotation))
	{
		// Use fallback
		SocketLocation = FallbackOffset;
		SocketRotation = FRotator::ZeroRotator;
	}

	// Create socket transform and chain with parent
	FTransform SocketTransform(SocketRotation, SocketLocation);
	return SocketTransform * ParentTransform;
}
#pragma endregion

#pragma region Weighted Selection
const FWallModule* URoomGenerationHelpers::SelectWeightedWallModule(const TArray<FWallModule>& Modules)
{
	return SelectWeightedRandom<FWallModule>(Modules,
		[](const FWallModule& Module) { return Module.PlacementWeight; });
}

const FMeshPlacementInfo* URoomGenerationHelpers::SelectWeightedMeshPlacement(const TArray<FMeshPlacementInfo>& MeshPool)
{
	return SelectWeightedRandom<FMeshPlacementInfo>(MeshPool,
		[](const FMeshPlacementInfo& Info) { return Info.PlacementWeight; });
}
#pragma endregion