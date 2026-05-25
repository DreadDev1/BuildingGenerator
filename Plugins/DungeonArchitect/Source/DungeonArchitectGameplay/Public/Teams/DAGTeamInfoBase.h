//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "System/DAGGameplayTagStack.h"

#include "GameFramework/Info.h"
#include "DAGTeamInfoBase.generated.h"

namespace EEndPlayReason { enum Type : int; }

class UDAG_TeamCreationComponent;
class UDAG_TeamSubsystem;
class UObject;
struct FFrame;

UCLASS(Abstract)
class ADAG_TeamInfoBase : public AInfo
{
	GENERATED_BODY()

public:
	ADAG_TeamInfoBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	int32 GetTeamId() const { return TeamId; }

	//~AActor interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

protected:
	virtual void RegisterWithTeamSubsystem(UDAG_TeamSubsystem* Subsystem);
	void TryRegisterWithTeamSubsystem();

private:
	void SetTeamId(int32 NewTeamId);

	UFUNCTION()
	void OnRep_TeamId();

public:
	friend UDAG_TeamCreationComponent;

	UPROPERTY(Replicated)
	FDAG_GameplayTagStackContainer TeamTags;

private:
	UPROPERTY(ReplicatedUsing=OnRep_TeamId)
	int32 TeamId;
};

