//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Flow/ExecGraph/Nodes/FlowExecEdGraphNodes.h"

#include "Frameworks/Flow/Domains/FlowDomain.h"
#include "Frameworks/Flow/ExecGraph/FlowExecEdGraphSchema.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTask.h"

#include "EdGraph/EdGraph.h"
#include "UObject/Class.h"

#define LOCTEXT_NAMESPACE "GridFlowExecEdGraphNode_Result"

class UFlowExecEdGraphSchema;

FText UFlowExecEdGraphNode_Result::GetNodeTitle(ENodeTitleType::Type TitleType) const {
    return LOCTEXT("ResultTextNodeTitle", "Result");
}

FText UFlowExecEdGraphNode_Task::GetNodeTitle(ENodeTitleType::Type TitleType) const {
    if (!TaskTemplate) {
        return LOCTEXT("TaskInvaliNodeTitle", "[INVALID]");
    }

    const FString Title = TaskTemplate->GetClass()->GetMetaData("Title");
    return FText::FromString(Title);
}

bool UFlowExecEdGraphNode_Task::CanPasteHere(const UEdGraph* TargetGraph) const {
    const UFlowExecEdGraphSchema* ExecGraphSchema = Cast<UFlowExecEdGraphSchema>(TargetGraph->GetSchema());
    if (ExecGraphSchema && TaskTemplate) {
        UClass* DesiredClass = TaskTemplate->GetClass();
        TArray<IFlowDomainWeakPtr> Domains = ExecGraphSchema->GetAllowedDomains();
        for (IFlowDomainWeakPtr DomainPtr : Domains) {
            IFlowDomainPtr Domain = DomainPtr.Pin();
            if (!Domain.IsValid()) continue;
        
            TArray<UClass*> DomainTaskClasses;
            Domain->GetDomainTasks(DomainTaskClasses);
            if (DomainTaskClasses.Contains(DesiredClass)) {
                return true;
            }
        }
    }
    return false;
}

#undef LOCTEXT_NAMESPACE

