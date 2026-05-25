//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/FlowAssetBase.h"
#include "CellFlowAsset.generated.h"

enum class ECellFlowAssetVersion {
	InitialVersion = 0,

	//----------- Versions should be placed above this line -----------------
	LastVersionPlusOne,
	LatestVersion = LastVersionPlusOne - 1
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UCellFlowAsset : public UFlowAssetBase {
	GENERATED_BODY()
public:
	UPROPERTY()
	int32 Version = 0;
	
};

