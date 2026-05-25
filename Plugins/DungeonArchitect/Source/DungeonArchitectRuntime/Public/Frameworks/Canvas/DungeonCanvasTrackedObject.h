//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "DungeonCanvasTrackedObject.generated.h"

UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class DUNGEONARCHITECTRUNTIME_API UDungeonCanvasTrackedObject : public USceneComponent {
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	FName IconName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	bool bOrientToRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	bool bOccludesFogOfWar = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	bool bHideWhenOutOfSight = false;

	/** Higher numbers show up on top when overlapped with other icons. E.g. if you want your player icon to be drawn on top of everything else, bump this number up (e.g. 1000) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon Canvas")
	int32 ZOrder = 0;

public:
	UDungeonCanvasTrackedObject();
	virtual void BeginPlay() override;
	virtual void InitializeComponent() override;

};


