//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Volumes/DungeonVolume.h"
#include "Frameworks/Flow/Domains/LayoutGraph/Tasks/BaseFlowLayoutTaskPathBuilderBase.h"
#include "DungeonFlowTagVolume.generated.h"

UCLASS(Blueprintable)
class ADungeonFlowTagVolume : public ADungeonVolume {
	GENERATED_BODY()

public:
	ADungeonFlowTagVolume(const FObjectInitializer& ObjectInitializer);

	static void FindInWorld(const ADungeon* InDungeon, TArray<TWeakObjectPtr<ADungeonFlowTagVolume>>& OutVolumes);
	
public:
	UPROPERTY(EditAnywhere, Category = Dungeon)
	TArray<FFlowPathTagConstraints> PathConstraints;
};

