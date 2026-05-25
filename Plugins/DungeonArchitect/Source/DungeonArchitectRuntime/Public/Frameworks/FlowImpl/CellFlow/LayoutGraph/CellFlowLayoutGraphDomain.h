//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Flow/Domains/FlowDomain.h"

typedef TSharedPtr<class IFlowAGNodeGroupGenerator> IFlowAGNodeGroupGeneratorPtr;
typedef TSharedPtr<class FFlowAbstractGraphConstraints> FFlowAbstractGraphConstraintsPtr;

class DUNGEONARCHITECTRUNTIME_API FCellFlowLayoutGraphDomain : public IFlowDomain {
public:
	virtual FName GetDomainID() const override;
	virtual FText GetDomainDisplayName() const override;
	virtual void GetDomainTasks(TArray<UClass*>& OutTaskClasses) const override;
    
#if WITH_EDITOR
	virtual UFlowExecTask* TryCreateCompatibleTask(UFlowExecTask* InTaskObject) override;
#endif // WITH_EDITOR
	
public:
	static const FName DomainID;
};

