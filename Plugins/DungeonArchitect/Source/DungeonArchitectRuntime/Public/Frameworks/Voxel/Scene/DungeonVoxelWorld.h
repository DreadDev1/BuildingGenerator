//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Voxel/VDB/VDBLib.h"

#include "GameFramework/Actor.h"
#include "DungeonVoxelWorld.generated.h"

class UDynamicMeshComponent;
namespace UE {
	namespace Geometry {
		class FDynamicMesh3;
	}
}

UENUM(BlueprintType)
enum class EDAVoxelCSGOperation : uint8 {
	Add UMETA(DisplayName = "Add (Union)"),
	Subtract UMETA(DisplayName = "Subtract"),
	Intersect UMETA(DisplayName = "Intersect")
};

USTRUCT(BlueprintType)
struct FDAVoxelSphere {
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sphere")
	FVector WorldLocation = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sphere")
	float WorldRadius = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sphere")
	uint8 MaterialID = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sphere")
	EDAVoxelCSGOperation Operation = EDAVoxelCSGOperation::Add;
};

USTRUCT(BlueprintType)
struct FDAVoxelBox {
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Box")
	FVector WorldLocation = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Box")
	FVector WorldExtent = FVector(100.0f, 100.0f, 100.0f);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Box")
	FRotator WorldRotation = FRotator::ZeroRotator;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Box")
	uint8 MaterialID = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Box")
	EDAVoxelCSGOperation Operation = EDAVoxelCSGOperation::Add;
};

UCLASS()
class ADungeonVoxelWorld : public AActor {
	GENERATED_BODY()
public:
	ADungeonVoxelWorld();
	
	UFUNCTION(CallInEditor, BlueprintCallable, Category="Voxel")
	void BuildWorld();

	UFUNCTION(BlueprintCallable, Category="Voxel")
	void RemoveBlock(const FVector& Location, const FVector& Normal, float Radius);
	
private:
	void GenerateDensity();
	void GenerateMesh();
	
private:
	DA::VDB::FVoxelGrid VoxelGrid;

	UPROPERTY(EditAnywhere, Category=Voxel)
	TObjectPtr<UDynamicMeshComponent> MeshComponent;
	
	UPROPERTY(EditAnywhere, Category=Voxel)
	double VoxelSize = 12.5;
	
	UPROPERTY(EditAnywhere, Category=Voxel)
	int32 Seed = 0;
	
	UPROPERTY(EditAnywhere, Category=Voxel)
	bool bUseMarchingCubes = true;
	
	UPROPERTY(EditAnywhere, Category=Voxel, meta=(ClampMin=0, ClampMax=4))
	int32 MeshLODLevel = 0;
	
	UPROPERTY(EditAnywhere, Category="Voxel|Spheres")
	TArray<FDAVoxelSphere> Spheres;
	
	UPROPERTY(EditAnywhere, Category="Voxel|Boxes")
	TArray<FDAVoxelBox> Boxes;
};

