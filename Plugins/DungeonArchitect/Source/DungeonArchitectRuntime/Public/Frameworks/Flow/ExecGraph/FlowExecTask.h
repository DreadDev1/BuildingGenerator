//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/Attributes.h"
#include "Frameworks/Flow/ExecGraph/FlowExecTaskStructs.h"

#include "Misc/Variant.h"
#include "UObject/GCObject.h"
#include "UObject/Interface.h"
#include "FlowExecTask.generated.h"

struct FDAAttribute;
class UFlowExecScriptGraphNode;

typedef TSharedPtr<class FFlowExecNodeState> FFlowExecNodeStatePtr;

class DUNGEONARCHITECTRUNTIME_API FFlowExecNodeState :  public FGCObject {
public:
    FFlowExecNodeStatePtr Clone();
    TObjectPtr<UObject> GetStateObject(const FName& InObjectID) const;
    void SetStateObject(const FName& InObjectID, const TObjectPtr<UObject>& InObject);
    virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
    virtual FString GetReferencerName() const override;

    template<typename T>
    T* GetState(const FName& InDomainID) {
        return Cast<T>(GetStateObject(InDomainID));
    }

    template<typename T>
    T* GetState() {
        return Cast<T>(GetStateObject(T::StateTypeID));
    }
    
private:
    TMap<FName, TObjectPtr<UObject>> StateObjects;
};

struct DUNGEONARCHITECTRUNTIME_API FFlowTaskExecutionSettings {
    TMap<FName, FVariant> BuilderAttributes;
    TArray<TWeakObjectPtr<AActor>> ContextActors;
};

/////////////////////////////////////////////////////
// DEPRECATED: UFlowExecTaskExtender
UCLASS(Abstract)
class DUNGEONARCHITECTRUNTIME_API UFlowExecTaskExtender : public UObject {
    GENERATED_BODY()

};
/////////////////////////////////////////////////////


enum class EFlowExecTaskInputConstraint {
    SingleInput,
    MultiInput,
    NoInput
};


UCLASS(Abstract)
class DUNGEONARCHITECTRUNTIME_API UFlowExecTask : public UObject {
    GENERATED_BODY()

public:
    virtual void Execute(const FFlowExecutionInput& Input, const FFlowTaskExecutionSettings& InExecSettings, FFlowExecutionOutput& Output) const;
    virtual void LookAhead(const FFlowExecutionInput& Input) const {}
    virtual bool GetParameter(const FString& InParameterName, FDAAttribute& OutValue) { return false; }
    virtual bool SetParameter(const FString& InParameterName, const FDAAttribute& InValue) { return false; }
    virtual bool SetParameterSerialized(const FString& InParameterName, const FString& InSerializedText) { return false; }

    FORCEINLINE EFlowExecTaskInputConstraint GetInputConstraint() const { return InputConstraint; }
    virtual TArray<int32> SelectIncomingNodeBranches(const FRandomStream& InRandom, const TArray<UFlowExecScriptGraphNode*> IncomingNodes);

#if WITH_EDITOR
    virtual FLinearColor GetNodeColor() const;
#endif // WITH_EDITOR
    
public:
    UPROPERTY(EditAnywhere, Category = "Node")
    FString Description;
 
    /**
       The variable name of the node. Use this to set the values of the node attributes in the form [NODE_NAME].[PARAMETER_NAME]

       For e.g. If this node is named "createTreasurePath" and there's a parameter named "pathSize" in the node (find this name by hovering over the parameter)
       then the parameter can be referenced like this "createTreasurePath.pathSize"
    */
    UPROPERTY(EditAnywhere, Category = "Advanced")
    FString NodeVariableName;

    
    /** DEPRECATED: Extends the functionality of the tasks.  This is useful for cross domain extensions */
    UPROPERTY()
    TArray<TObjectPtr<UFlowExecTaskExtender>> Extenders;

protected:
    EFlowExecTaskInputConstraint InputConstraint { EFlowExecTaskInputConstraint::SingleInput };
};


UINTERFACE(BlueprintType)
class DUNGEONARCHITECTRUNTIME_API UFlowExecCloneableState : public UInterface {
    GENERATED_UINTERFACE_BODY()
};

class DUNGEONARCHITECTRUNTIME_API IFlowExecCloneableState {
    GENERATED_IINTERFACE_BODY()
public:
    virtual void CloneFromStateObject(const UObject* SourceObject);
};

class UFlowAbstractNode;

