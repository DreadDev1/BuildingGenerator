// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/Room/WallData.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "RoomSpawnerHelpers.generated.h"

class UDebugHelpers;
struct FPlacedWallInfo;

UCLASS()
class BUILDINGGENERATOR_API URoomSpawnerHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
	public:
#pragma region Mesh Instance Management
	// INSTANCED STATIC MESH COMPONENT MANAGEMENT
	/** Get or create an ISM component for a specific mesh Creates new component if one doesn't exist for this mesh */
	static UInstancedStaticMeshComponent* GetOrCreateISMComponent( AActor* Owner,const TSoftObjectPtr<UStaticMesh>& MeshAsset,
	TMap<TSoftObjectPtr<UStaticMesh>, UInstancedStaticMeshComponent*>& ComponentMap,const FString& ComponentNamePrefix,
	bool bLogWarnings = true);

	/*Clear all ISM components in a component map Destroys components and clears the map */
	static void ClearISMComponentMap(TMap<TSoftObjectPtr<UStaticMesh>, UInstancedStaticMeshComponent*>& ComponentMap);

 	/* Spawn a mesh instance with local-to-world transform conversion*/
	UFUNCTION(BlueprintCallable, Category = "Dungeon Spawner|Mesh")
	static int32 SpawnMeshInstance( UInstancedStaticMeshComponent* ISMComponent, const FTransform& LocalTransform,
	const FVector& WorldOffset);

	/* Spawn multiple mesh instances from an array */
	static int32 SpawnMeshInstances(UInstancedStaticMeshComponent* ISMComponent, const TArray<FTransform>& LocalTransforms,
	const FVector& WorldOffset);
#pragma endregion
	
#pragma region Mesh Transform Utilities
	/* Convert local (component-space) transform to world transform */
	UFUNCTION(BlueprintPure, Category = "Dungeon Spawner|Transform")
	static FTransform LocalToWorldTransform(const FTransform& LocalTransform, const FVector& WorldOffset);

	/* Convert array of local transforms to world transforms */
	static TArray<FTransform> LocalToWorldTransforms(const TArray<FTransform>& LocalTransforms, const FVector& WorldOffset);
#pragma endregion
	
#pragma region Wall Spawning
	/** Spawn a complete wall segment (Base + Middle layers + Top)
	* @param Owner - Actor owning ISM components @param PlacedWall - contains transform / module data
	* @param WallComponents - Map of ISM components @param RoomOrigin - World position for room
	* @param ComponentPrefix - Prefix for ISM component names @param DebugHelpers - debug helper for logging */
	static void SpawnWallSegment(AActor* Owner, const FPlacedWallInfo& PlacedWall, TMap<TSoftObjectPtr<UStaticMesh>, UInstancedStaticMeshComponent*>& WallComponents,
	const FVector& RoomOrigin, const FString& ComponentPrefix = TEXT("WallISM_"), class UDebugHelpers* DebugHelpers = nullptr);
#pragma endregion
};