//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/ExecGraph/Tasks/CommonFlowGraphDomain.h"

#include "Frameworks/Flow/ExecGraph/Tasks/SelectFlowExecTask.h"

#define LOCTEXT_NAMESPACE "CoreFlowGraphDomain"

const FName FCommonFlowGraphDomain::DomainID = TEXT("CommonFlowGraphDomain");

FName FCommonFlowGraphDomain::GetDomainID() const {
	return DomainID;
}

FText FCommonFlowGraphDomain::GetDomainDisplayName() const {
	return LOCTEXT("DTMDisplayName", "Common");
}

void FCommonFlowGraphDomain::GetDomainTasks(TArray<UClass*>& OutTaskClasses) const {
	static const TArray<UClass*> DomainTasks = {
		USelectFlowExecTask::StaticClass()
	};
	OutTaskClasses = DomainTasks;
}


#undef LOCTEXT_NAMESPACE


