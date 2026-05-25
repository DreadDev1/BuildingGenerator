//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonForgeAsset.generated.h"

class UDungeonForgeGraph;

UENUM()
enum class EDungeonForgeAssetVersion : uint8
{
	None = 0,
    
	// Initial version with Props array
	V1_Initial,
    
	// -----<new versions can be added above this line>-----
	LastVersionPlusOne,
	LatestVersion = LastVersionPlusOne - 1
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeAsset : public UObject {
	GENERATED_BODY()
	
public:
	UDungeonForgeAsset(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	UPROPERTY()
	int32 Version;

	UPROPERTY()
	TObjectPtr<UDungeonForgeGraph> ForgeGraph;
};

