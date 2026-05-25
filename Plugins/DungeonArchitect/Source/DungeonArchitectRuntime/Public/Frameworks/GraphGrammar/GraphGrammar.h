//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "GraphGrammar.generated.h"

class UGrammarRuleScript;
class UGrammarExecutionScript;

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UGraphGrammarProduction : public UObject {
    GENERATED_UCLASS_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Rule")
    FText RuleName;

    UPROPERTY()
    TObjectPtr<UGrammarRuleScript> SourceGraph;

    UPROPERTY()
    TArray<TObjectPtr<UGrammarRuleScript>> DestGraphs;

};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UGrammarNodeType : public UObject {
    GENERATED_UCLASS_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "NodeType")
    FName TypeName;

    UPROPERTY(EditAnywhere, Category = "NodeType")
    FText Description;

    UPROPERTY()
    bool bWildcard;

#if WITH_EDITORONLY_DATA
    UPROPERTY(EditAnywhere, Category = "NodeType")
    FLinearColor NodeColor;
#endif // WITH_EDITORONLY_DATA
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UGraphGrammar : public UObject {
    GENERATED_UCLASS_BODY()

public:
    UPROPERTY()
    TArray<TObjectPtr<UGraphGrammarProduction>> ProductionRules;

    UPROPERTY()
    TObjectPtr<UGrammarExecutionScript> ExecutionGraphScript;

    UPROPERTY()
    TArray<TObjectPtr<UGrammarNodeType>> NodeTypes;

    UPROPERTY()
    TObjectPtr<UGrammarNodeType> WildcardType;

};

