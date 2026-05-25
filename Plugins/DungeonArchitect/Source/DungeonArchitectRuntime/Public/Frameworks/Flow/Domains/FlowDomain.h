//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class UFlowExecTask;

class DUNGEONARCHITECTRUNTIME_API IFlowDomain {
public:
    virtual ~IFlowDomain() {}
    virtual FName GetDomainID() const = 0;
    virtual FText GetDomainDisplayName() const = 0;
    virtual void GetDomainTasks(TArray<UClass*>& OutTaskClasses) const = 0;

#if WITH_EDITOR
    virtual UFlowExecTask* TryCreateCompatibleTask(UFlowExecTask* InTaskObject) { return nullptr; }
#endif // WITH_EDITOR
    
};
typedef TSharedPtr<IFlowDomain> IFlowDomainPtr;
typedef TWeakPtr<IFlowDomain> IFlowDomainWeakPtr;


class DUNGEONARCHITECTRUNTIME_API FPreviewFlowDomain : public IFlowDomain {
public:
    FPreviewFlowDomain(const FName& InDomainID, const FText& InDisplayName);
    virtual FName GetDomainID() const override { return DomainID; }
    virtual FText GetDomainDisplayName() const override { return DisplayName; }
    virtual void GetDomainTasks(TArray<UClass*>& OutTaskClasses) const override {}
    
private:
    FName DomainID;
    FText DisplayName;
};

