//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "DAGTeamCheats.generated.h"

class UObject;
struct FFrame;

/** Cheats related to teams */
UCLASS()
class UDAG_TeamCheats : public UCheatManagerExtension
{
	GENERATED_BODY()

public:
	// Moves this player to the next available team, wrapping around to the
	// first team if at the end of the list of teams
	UFUNCTION(Exec, BlueprintAuthorityOnly, Category="DAG")
	virtual void CycleTeam();

	// Moves this player to the specified team
	UFUNCTION(Exec, BlueprintAuthorityOnly, Category="DAG")
	virtual void SetTeam(int32 TeamID);

	// Prints a list of all of the teams
	UFUNCTION(Exec)
	virtual void ListTeams();
};

