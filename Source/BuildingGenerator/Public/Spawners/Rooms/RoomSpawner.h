// Fill out your copyright notice in the Description page of Project Settings. 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Generators/Rooms/RoomGenerator.h"
#include "Utilities/Debugging/DebugHelpers.h"
#include "Data/Room/RoomData.h"
#include "RoomSpawner.generated.h"

class ADoorway;
class UWallData;
class UTextRenderComponent;
class UInstancedStaticMeshComponent;
/**
 * RoomSpawner - Actor responsible for spawning and visualizing rooms in the level
 * Holds RoomGenerator for logic and DebugHelpers for visualization Provides CallInEditor functions for designer workflow */
UCLASS(Abstract)
class BUILDINGGENERATOR_API ARoomSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARoomSpawner();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* SceneRoot;
	// Room generator instance (logic layer)
	UPROPERTY()
	URoomGenerator* RoomGenerator;

#pragma region Debug Components
	// Debug visualization component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UDebugHelpers* DebugHelpers;
#pragma endregion 

#pragma region Room Generation Properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Configuration")
	URoomData* RoomData;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Room Configuration", meta = (ClampMin = "4", ClampMax = "50"))
	FIntPoint RoomGridSize = FIntPoint(10, 10);
#pragma endregion

#pragma region Editor Functions
#if WITH_EDITOR
#pragma region Room Grid Generation
	/* Generate the room grid (visualization only at this stage) Creates empty grid and displays it with coordinates */
	UFUNCTION(CallInEditor, Category = "Room Generation|Generation")
	void GenerateRoomGrid();
	
	/* Clear the room grid and all visualizations */
	UFUNCTION(CallInEditor, Category = "Room Generation|Clearing")
	virtual void ClearRoomGrid();
#pragma endregion
	
#pragma region Floor Mesh Generation
	/** Generate floor meshes based on RoomData FloorData */
	UFUNCTION(CallInEditor, Category = "Room Generation|Generation")
	void GenerateFloorMeshes();

	/* Clear all spawned floor meshes */
	UFUNCTION(CallInEditor, Category = "Room Generation|Clearing")
	void ClearFloorMeshes();
#pragma endregion

#pragma region Wall Mesh Generation
	/** Generate wall meshes based on RoomData WallData */
	UFUNCTION(CallInEditor, Category = "Room Generation|Generation")
	void GenerateWallMeshes();

	/* Clear all spawned wall meshes */
	UFUNCTION(CallInEditor, Category = "Room Generation|Clearing")
	void ClearWallMeshes();
	
	// Helper functions
	void SpawnWallSegment(const FPlacedWallInfo& PlacedWall, const FVector& RoomOrigin);
#pragma endregion
	
#pragma region Corner Mesh Generation
	/** Generate corner meshes for all 4 corners */
	UFUNCTION(CallInEditor, Category = "Room Generation|Generation")
	void GenerateCornerMeshes();

	/* Clear all spawned corner meshes */
	UFUNCTION(CallInEditor, Category = "Room Generation|Clearing")
	void ClearCornerMeshes();
#pragma endregion
	
#pragma region Doorway Mesh Generation
	/** Generate doorway meshes (frames only, actors later) */
	UFUNCTION(CallInEditor, Category = "Room Generation|Generation")
	void GenerateDoorwayMeshes();
		
	/** Clear all spawned doorway meshes */
	UFUNCTION(CallInEditor, Category = "Room Generation|Clearing")
	void ClearDoorwayMeshes();
#pragma endregion
	
#pragma region Ceiling Mesh Generation
	/* Generate ceiling meshes */
	UFUNCTION(CallInEditor, Category = "Room Generation|Generation")
	void GenerateCeilingMeshes();

	/* Clear ceiling meshes */
	UFUNCTION(CallInEditor, Category = "Room Generation|Clearing")
	void ClearCeilingMeshes();	
#pragma endregion
	
#pragma region Debug Functions
#pragma region Grid Coordinate Text Rendering
	/* Toggle grid outline display */
	UFUNCTION(CallInEditor, Category = "Room Generation|Toggles")
	void ToggleGrid();
	
	/* Toggle coordinate display */
	UFUNCTION(CallInEditor, Category = "Room Generation|Toggles")
	void ToggleCoordinates();
	
	/* Toggle cell state visualization */
	UFUNCTION(CallInEditor, Category = "Room Generation|Toggles")
	void ToggleCellStates();
	
	/* Create a text render component at specified world location Called by DebugHelpers via delegate */
	virtual UTextRenderComponent* CreateTextRenderComponent(FVector WorldPosition, FString Text, FColor Color, float Scale);

	/* Destroy a text render component Called by DebugHelpers via delegate */
	virtual void DestroyTextRenderComponent(UTextRenderComponent* TextComp);
#pragma endregion
	
	/* Refresh visualization (useful after changing debug settings) */
	UFUNCTION(CallInEditor, Category = "Room Generation|Visualization")
	void RefreshVisualization();
#pragma endregion
	
#endif
#pragma endregion
	
	/* Get the room generator instance  */
	URoomGenerator* GetRoomGenerator() const { return RoomGenerator; }

	/* Check if room is generated */
	bool IsRoomGenerated() const { return bIsGenerated; }

protected:
	// Ensure RoomGenerator is created and initialized (lightweight)
	virtual bool EnsureGeneratorReady();
	
	/* Update visualization based on current grid state */
	virtual void UpdateVisualization();
	
private:
	
	// Flag to track if room is generated
	bool bIsGenerated;
	
#pragma region Mesh Components & Actors
	// Track spawned floor mesh instances
	UPROPERTY()
	TMap<TSoftObjectPtr<UStaticMesh>, UInstancedStaticMeshComponent*> FloorMeshComponents;

	// Track spawned wall mesh instances
	UPROPERTY()
	TMap<TSoftObjectPtr<UStaticMesh>, UInstancedStaticMeshComponent*> WallMeshComponents;
	
	// Track spawned corner mesh instances
	UPROPERTY()
	TMap<TSoftObjectPtr<UStaticMesh>, UInstancedStaticMeshComponent*> CornerMeshComponents;
	
	// Track spawned corner mesh instances
	UPROPERTY()
	TMap<TSoftObjectPtr<UStaticMesh>, UInstancedStaticMeshComponent*> CeilingMeshComponents;
	
	/* Spawned doorway actors (replaces ISM doorway system) */
	UPROPERTY()
	TArray<ADoorway*> SpawnedDoorwayActors;

	/* Blueprint class to use for doorway actors
	 * Defaults to ADoorwayActor, but can be overridden with Blueprint subclass */
	UPROPERTY(EditAnywhere, Category = "Room Generation|Doorways")
	TSubclassOf<ADoorway> DoorwayActorClass;
#pragma endregion

#pragma region Debug Functions
	/* Log room statistics to output */
	void LogRoomStatistics();

	/* Log floor statistics to output */
	void LogFloorStatistics();
#pragma endregion
};