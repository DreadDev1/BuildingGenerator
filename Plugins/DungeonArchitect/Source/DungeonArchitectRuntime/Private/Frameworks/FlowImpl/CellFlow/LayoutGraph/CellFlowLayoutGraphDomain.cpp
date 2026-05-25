//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/CellFlowLayoutGraphDomain.h"

#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateCellsGrid.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateCellsVoronoi.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateKeyLock.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreateMainPath.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskCreatePath.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskFinalize.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskScatterProps.h"
#include "Frameworks/FlowImpl/CellFlow/LayoutGraph/Tasks/CellFlowLayoutTaskSpawnItems.h"

#include "Engine/Engine.h"

#define LOCTEXT_NAMESPACE "CellFlowAbstractGraphDomain"

const FName FCellFlowLayoutGraphDomain::DomainID = TEXT("CellFlowLayoutGraph");
FName FCellFlowLayoutGraphDomain::GetDomainID() const {
	return DomainID;
}

FText FCellFlowLayoutGraphDomain::GetDomainDisplayName() const {
    return LOCTEXT("DisplayName", "Layout Graph");
}

void FCellFlowLayoutGraphDomain::GetDomainTasks(TArray<UClass*>& OutTaskClasses) const {
	static const TArray<UClass*> DomainTasks = {
		UCellFlowLayoutTaskCreateCellsGrid::StaticClass(),
		UCellFlowLayoutTaskCreateCellsVoronoi::StaticClass(),
		UCellFlowLayoutTaskCreateMainPath::StaticClass(),
		UCellFlowLayoutTaskCreatePath::StaticClass(),
		UCellFlowLayoutTaskSpawnItems::StaticClass(),
		UCellFlowLayoutTaskCreateKeyLock::StaticClass(),
		UCellFlowLayoutTaskScatterProps::StaticClass(),
		UCellFlowLayoutTaskFinalize::StaticClass(),
	};
	OutTaskClasses = DomainTasks;
}

#if WITH_EDITOR
UFlowExecTask* FCellFlowLayoutGraphDomain::TryCreateCompatibleTask(UFlowExecTask* InTaskObject) {
	if (!InTaskObject) {
		return nullptr;
	}

	TSubclassOf<UFlowExecTask> TargetTaskClass = nullptr;
	if (InTaskObject->IsA<UBaseFlowLayoutTaskCreateMainPath>()) {
		TargetTaskClass = UCellFlowLayoutTaskCreateMainPath::StaticClass();
	}
	else if (InTaskObject->IsA<UBaseFlowLayoutTaskCreatePath>()) {
		TargetTaskClass = UCellFlowLayoutTaskCreatePath::StaticClass();
	}
	else if (InTaskObject->IsA<UBaseFlowLayoutTaskSpawnItems>()) {
		TargetTaskClass = UCellFlowLayoutTaskSpawnItems::StaticClass();
	}
	else if (InTaskObject->IsA<UBaseFlowLayoutTaskCreateKeyLock>()) {
		TargetTaskClass = UCellFlowLayoutTaskCreateKeyLock::StaticClass();
	}
	else if (InTaskObject->IsA<UBaseFlowLayoutTaskFinalize>()) {
		TargetTaskClass = UCellFlowLayoutTaskFinalize::StaticClass();
	}

	if (!TargetTaskClass) {
		return nullptr;
	}

	UFlowExecTask* CompatibleTask = NewObject<UFlowExecTask>(InTaskObject->GetOuter(), TargetTaskClass);
	UEngine::CopyPropertiesForUnrelatedObjects(InTaskObject, CompatibleTask);
	return CompatibleTask;
}
#endif // WITH_EDITOR



#undef LOCTEXT_NAMESPACE

