//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Teams/DAGTeamStatics.h"

#include "System/DAGLogChannels.h"
#include "Teams/DAGTeamDisplayAsset.h"
#include "Teams/DAGTeamSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(DAGTeamStatics)

class UTexture;

//////////////////////////////////////////////////////////////////////

void UDAG_TeamStatics::DAG_FindTeamFromObject(const UObject* Agent, bool& bIsPartOfTeam, int32& TeamId, UDAG_TeamDisplayAsset*& DisplayAsset, bool bLogIfNotSet)
{
	bIsPartOfTeam = false;
	TeamId = INDEX_NONE;
	DisplayAsset = nullptr;

	if (UWorld* World = GEngine->GetWorldFromContextObject(Agent, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (UDAG_TeamSubsystem* TeamSubsystem = World->GetSubsystem<UDAG_TeamSubsystem>())
		{
			TeamId = TeamSubsystem->FindTeamFromObject(Agent);
			if (TeamId != INDEX_NONE)
			{
				bIsPartOfTeam = true;

				DisplayAsset = TeamSubsystem->GetTeamDisplayAsset(TeamId, INDEX_NONE);

				if ((DisplayAsset == nullptr) && bLogIfNotSet)
				{
					UE_LOG(LogDAGTeams, Log, TEXT("FindTeamFromObject(%s) called too early (found team %d but no display asset set yet"), *GetPathNameSafe(Agent), TeamId);
				}
			}
		}
		else
		{
			UE_LOG(LogDAGTeams, Error, TEXT("FindTeamFromObject(%s) failed: Team subsystem does not exist yet"), *GetPathNameSafe(Agent));
		}
	}
}

UDAG_TeamDisplayAsset* UDAG_TeamStatics::DAG_GetTeamDisplayAsset(const UObject* WorldContextObject, int32 TeamId)
{
	UDAG_TeamDisplayAsset* Result = nullptr;
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (UDAG_TeamSubsystem* TeamSubsystem = World->GetSubsystem<UDAG_TeamSubsystem>())
		{
			return TeamSubsystem->GetTeamDisplayAsset(TeamId, INDEX_NONE);
		}
	}
	return Result;
}

float UDAG_TeamStatics::DAG_GetTeamScalarWithFallback(UDAG_TeamDisplayAsset* DisplayAsset, FName ParameterName, float DefaultValue)
{
	if (DisplayAsset)
	{
		if (float* pValue = DisplayAsset->ScalarParameters.Find(ParameterName))
		{
			return *pValue;
		}
	}
	return DefaultValue;
}

FLinearColor UDAG_TeamStatics::DAG_GetTeamColorWithFallback(UDAG_TeamDisplayAsset* DisplayAsset, FName ParameterName, FLinearColor DefaultValue)
{
	if (DisplayAsset)
	{
		if (FLinearColor* pColor = DisplayAsset->ColorParameters.Find(ParameterName))
		{
			return *pColor;
		}
	}
	return DefaultValue;
}

UTexture* UDAG_TeamStatics::DAG_GetTeamTextureWithFallback(UDAG_TeamDisplayAsset* DisplayAsset, FName ParameterName, UTexture* DefaultValue)
{
	if (DisplayAsset)
	{
		if (TObjectPtr<UTexture>* pTexture = DisplayAsset->TextureParameters.Find(ParameterName))
		{
			return *pTexture;
		}
	}
	return DefaultValue;
}

