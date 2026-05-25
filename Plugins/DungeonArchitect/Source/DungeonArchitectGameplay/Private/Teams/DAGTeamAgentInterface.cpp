//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Teams/DAGTeamAgentInterface.h"

#include "System/DAGLogChannels.h"

#include "UObject/ScriptInterface.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(DAGTeamAgentInterface)

UDAG_TeamAgentInterface::UDAG_TeamAgentInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void IDAG_TeamAgentInterface::ConditionalBroadcastTeamChanged(TScriptInterface<IDAG_TeamAgentInterface> This, FGenericTeamId OldTeamID, FGenericTeamId NewTeamID)
{
	if (OldTeamID != NewTeamID)
	{
		const int32 OldTeamIndex = DAG_GenericTeamIdToInteger(OldTeamID); 
		const int32 NewTeamIndex = DAG_GenericTeamIdToInteger(NewTeamID);

		UObject* ThisObj = This.GetObject();
		UE_LOG(LogDAGTeams, Verbose, TEXT("[%s] %s assigned team %d"), *DAG_GetClientServerContextString(ThisObj), *GetPathNameSafe(ThisObj), NewTeamIndex);

		This.GetInterface()->GetTeamChangedDelegateChecked().Broadcast(ThisObj, OldTeamIndex, NewTeamIndex);
	}
}

