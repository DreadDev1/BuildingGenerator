//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Teams/DAGTeamInfoBase.h"
#include "DAGTeamPrivateInfo.generated.h"

class UObject;

UCLASS()
class ADAG_TeamPrivateInfo : public ADAG_TeamInfoBase
{
	GENERATED_BODY()

public:
	ADAG_TeamPrivateInfo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

