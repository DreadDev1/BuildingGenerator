//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Teams/DAGTeamInfoBase.h"
#include "DAGTeamPublicInfo.generated.h"

class UDAG_TeamCreationComponent;
class UDAG_TeamDisplayAsset;
class UObject;
struct FFrame;

UCLASS()
class ADAG_TeamPublicInfo : public ADAG_TeamInfoBase
{
	GENERATED_BODY()

	friend UDAG_TeamCreationComponent; 

public:
	ADAG_TeamPublicInfo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UDAG_TeamDisplayAsset* GetTeamDisplayAsset() const { return TeamDisplayAsset; }

private:
	UFUNCTION()
	void OnRep_TeamDisplayAsset();

	void SetTeamDisplayAsset(TObjectPtr<UDAG_TeamDisplayAsset> NewDisplayAsset);

private:
	UPROPERTY(ReplicatedUsing=OnRep_TeamDisplayAsset)
	TObjectPtr<UDAG_TeamDisplayAsset> TeamDisplayAsset;
};

