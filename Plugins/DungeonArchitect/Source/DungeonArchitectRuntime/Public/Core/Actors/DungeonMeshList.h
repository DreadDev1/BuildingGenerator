//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Actor.h"
#include "DungeonMeshList.generated.h"

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDungeonMeshListItem {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Dungeon Architect")
	TObjectPtr<UStaticMesh> StaticMesh{};
	
	UPROPERTY(EditAnywhere, Category="Dungeon Architect")
	FTransform Transform { FTransform::Identity };

	UPROPERTY(EditAnywhere, Category="Dungeon Architect")
	FGuid ItemGuid;

	FGuid GetNodeItemGuid(const FGuid& InNodeGuid) const;
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDungeonActorTemplateListItem {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Dungeon Architect")
	TSubclassOf<AActor> ClassTemplate;
	
	UPROPERTY(EditAnywhere, Category="Dungeon Architect")
	FTransform Transform { FTransform::Identity };
	
	UPROPERTY(EditAnywhere, Category="Dungeon Architect")
	FGuid ItemGuid;
	
	FGuid GetNodeItemGuid(const FGuid& InNodeGuid) const;
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonMeshList : public UObject {
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TArray<FDungeonMeshListItem> StaticMeshes;
	
	UPROPERTY()
	TArray<FDungeonActorTemplateListItem> ActorTemplates;

	UPROPERTY()
	bool bCanEverAffectNavigation = true;
	
	UPROPERTY()
	uint32 HashCode{};
	
	void CalculateHashCode();
};

