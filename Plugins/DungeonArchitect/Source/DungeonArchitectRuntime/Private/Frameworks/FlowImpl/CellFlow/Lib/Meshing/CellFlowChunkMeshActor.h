//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Utils/FlowVisLib.h"

#include "GameFramework/Actor.h"
#include "CellFlowChunkMeshActor.generated.h"

class UDAProcMeshComponent;
UCLASS()
class ACellFlowChunkMesh : public AActor {
	GENERATED_BODY()
public:
	ACellFlowChunkMesh();
	FORCEINLINE UDAProcMeshComponent* GetMeshComponent() const { return MeshComponent; };
	
	void UploadGeometry(int32 LODIndex, int32 SectionIndex, const FFlowVisLib::FGeometry& SurfaceGeometry, bool bEnableCollision) const;

public:
	UPROPERTY()
	FGuid DungeonID{};
	
private:
	UPROPERTY()
	TObjectPtr<UDAProcMeshComponent> MeshComponent;
};

