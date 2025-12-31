// Fill out your copyright notice in the Description page of Project Settings. 

#include "Utilities/Spawners/RoomSpawnerHelpers.h"
#include "Utilities/Generation/RoomGenerationHelpers.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Utilities/Debugging/DebugHelpers.h"


// INSTANCED STATIC MESH COMPONENT MANAGEMENT
UInstancedStaticMeshComponent* URoomSpawnerHelpers::GetOrCreateISMComponent(AActor* Owner, const TSoftObjectPtr<UStaticMesh>& MeshAsset,
TMap<TSoftObjectPtr<UStaticMesh>, UInstancedStaticMeshComponent*>& ComponentMap,const FString& ComponentNamePrefix,bool bLogWarnings)
{
	if (!Owner)
	{
		if (bLogWarnings) UE_LOG(LogTemp, Warning, TEXT("GetOrCreateISMComponent: Owner is null"));
		return nullptr;
	}

	// Return null if mesh asset is not set
	if (MeshAsset.IsNull())
	{
		if (bLogWarnings) UE_LOG(LogTemp, Warning, TEXT("GetOrCreateISMComponent: MeshAsset is null"));
		return nullptr;
	}

	// Check if we already have an ISM component for this mesh
	if (ComponentMap.Contains(MeshAsset)) return ComponentMap[MeshAsset];

	// Load and validate mesh using generation helper
	UStaticMesh* StaticMesh = URoomGenerationHelpers:: LoadAndValidateMesh(
	MeshAsset, ComponentNamePrefix,bLogWarnings);

	if (!StaticMesh) return nullptr;

	// Create new ISM component
	FString ComponentName = FString::Printf(TEXT("%s%s"), *ComponentNamePrefix, *MeshAsset. GetAssetName());
	
	UInstancedStaticMeshComponent* NewISM = NewObject<UInstancedStaticMeshComponent>(Owner, FName(*ComponentName));

	if (!NewISM)
	{
		if (bLogWarnings) UE_LOG(LogTemp, Warning, TEXT("GetOrCreateISMComponent: Failed to create component '%s'"), *ComponentName);
		return nullptr;
	}

	// Attach with KeepRelativeTransform and set relative location to zero
	NewISM->RegisterComponent();
	NewISM->AttachToComponent(Owner->GetRootComponent(), FAttachmentTransformRules:: KeepRelativeTransform);
 
	// Set component to origin RELATIVE to parent (not world space)
	NewISM->SetRelativeLocation(FVector::ZeroVector);
	NewISM->SetRelativeRotation(FRotator:: ZeroRotator);
	NewISM->SetRelativeScale3D(FVector::OneVector);
	
	// Set mesh
	NewISM->SetStaticMesh(StaticMesh);

	// Store component for reuse
	ComponentMap. Add(MeshAsset, NewISM);
	
	return NewISM;
}

void URoomSpawnerHelpers::ClearISMComponentMap(TMap<TSoftObjectPtr<UStaticMesh>, UInstancedStaticMeshComponent*>& ComponentMap)
{
	// Destroy all components
	for (auto& Pair : ComponentMap)
	{
		if (Pair.Value && Pair.Value->IsValidLowLevel())
		{
			Pair.Value->ClearInstances();
			Pair.Value->DestroyComponent();
		}
	}

	// Clear the map
	ComponentMap.Empty();
}

// MESH INSTANCE SPAWNING

int32 URoomSpawnerHelpers::SpawnMeshInstance(UInstancedStaticMeshComponent* ISMComponent,const FTransform& LocalTransform,
const FVector& WorldOffset)
{
	if (!ISMComponent) return -1;
	// Convert local transform to world transform
	FTransform WorldTransform = LocalToWorldTransform(LocalTransform, WorldOffset);

	// Add instance
	return ISMComponent->AddInstance(WorldTransform);
}

int32 URoomSpawnerHelpers::SpawnMeshInstances(UInstancedStaticMeshComponent* ISMComponent, const TArray<FTransform>& LocalTransforms,
const FVector& WorldOffset)
{
	if (!ISMComponent) return 0;
	int32 SpawnedCount = 0;

	for (const FTransform& LocalTransform : LocalTransforms)
	{
		int32 InstanceIndex = SpawnMeshInstance(ISMComponent, LocalTransform, WorldOffset);
		if (InstanceIndex >= 0) SpawnedCount++;
	}

	return SpawnedCount;
}
  
// TRANSFORM UTILITIES
FTransform URoomSpawnerHelpers::LocalToWorldTransform(const FTransform& LocalTransform, const FVector& WorldOffset)
{
	FTransform WorldTransform = LocalTransform;
	WorldTransform.SetLocation(WorldOffset + LocalTransform.GetLocation());
	return WorldTransform;
}

TArray<FTransform> URoomSpawnerHelpers::LocalToWorldTransforms(
const TArray<FTransform>& LocalTransforms, const FVector& WorldOffset)
{
	TArray<FTransform> WorldTransforms;
	WorldTransforms.Reserve(LocalTransforms.Num());

	for (const FTransform& LocalTransform : LocalTransforms)
	{
		WorldTransforms. Add(LocalToWorldTransform(LocalTransform, WorldOffset));
	}

	return WorldTransforms;
}

#pragma region Wall Spawning
void URoomSpawnerHelpers::SpawnWallSegment(AActor* Owner, const FPlacedWallInfo& PlacedWall,
	TMap<TSoftObjectPtr<UStaticMesh>, UInstancedStaticMeshComponent*>& WallComponents, const FVector& RoomOrigin,
	const FString& ComponentPrefix, class UDebugHelpers* DebugHelpers)
{
	// SPAWN BASE MESH (Required - Base Layer)
	UInstancedStaticMeshComponent* BottomISM = GetOrCreateISMComponent(
		Owner,
		PlacedWall.WallModule.BaseMesh,
		WallComponents,
		ComponentPrefix,
		true
	);

	if (BottomISM)
	{
		int32 InstanceIndex = SpawnMeshInstance(BottomISM, PlacedWall.BottomTransform, RoomOrigin);

		if (InstanceIndex >= 0 && DebugHelpers)
		{
			DebugHelpers->LogVerbose(FString::Printf(
				TEXT("  Spawned base mesh at edge %d, cell %d (instance %d)"),
				(int32)PlacedWall.Edge, PlacedWall.StartCell, InstanceIndex
			));
		}
	}

	// SPAWN MIDDLE1 MESH (Optional - First Middle Layer)
	if (!PlacedWall.WallModule.MiddleMesh1.IsNull())
	{
		UInstancedStaticMeshComponent* Middle1ISM = GetOrCreateISMComponent(
			Owner,
			PlacedWall.WallModule.MiddleMesh1,
			WallComponents,
			ComponentPrefix,
			true
		);

		if (Middle1ISM)
		{
			int32 InstanceIndex = SpawnMeshInstance(Middle1ISM, PlacedWall.Middle1Transform, RoomOrigin);

			if (InstanceIndex >= 0 && DebugHelpers)
			{
				DebugHelpers->LogVerbose(FString::Printf(
					TEXT("  Spawned middle1 mesh at edge %d, cell %d (instance %d)"),
					(int32)PlacedWall.Edge, PlacedWall.StartCell, InstanceIndex
				));
			}
		}
	}

	// SPAWN MIDDLE2 MESH (Optional - Second Middle Layer)
	if (!PlacedWall. WallModule.MiddleMesh2.IsNull())
	{
		UInstancedStaticMeshComponent* Middle2ISM = GetOrCreateISMComponent(
			Owner,
			PlacedWall.WallModule. MiddleMesh2,
			WallComponents,
			ComponentPrefix,
			true
		);

		if (Middle2ISM)
		{
			int32 InstanceIndex = SpawnMeshInstance(Middle2ISM, PlacedWall.Middle2Transform, RoomOrigin);

			if (InstanceIndex >= 0 && DebugHelpers)
			{
				DebugHelpers->LogVerbose(FString::Printf(
					TEXT("  Spawned middle2 mesh at edge %d, cell %d (instance %d)"),
					(int32)PlacedWall.Edge, PlacedWall.StartCell, InstanceIndex
				));
			}
		}
	}

	// SPAWN TOP MESH (Optional - Top Layer/Cap)
	if (!PlacedWall. WallModule.TopMesh. IsNull())
	{
		UInstancedStaticMeshComponent* TopISM = GetOrCreateISMComponent(
			Owner,
			PlacedWall. WallModule.TopMesh,
			WallComponents,
			ComponentPrefix,
			true
		);

		if (TopISM)
		{
			int32 InstanceIndex = SpawnMeshInstance(TopISM, PlacedWall.TopTransform, RoomOrigin);

			if (InstanceIndex >= 0 && DebugHelpers)
			{
				DebugHelpers->LogVerbose(FString::Printf(
					TEXT("  Spawned top mesh at edge %d, cell %d (instance %d)"),
					(int32)PlacedWall.Edge, PlacedWall.StartCell, InstanceIndex
				));
			}
		}
	}
}
#pragma endregion