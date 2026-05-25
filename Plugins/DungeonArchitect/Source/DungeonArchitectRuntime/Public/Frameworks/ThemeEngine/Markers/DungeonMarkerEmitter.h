//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonBuilder.h"
#include "Core/DungeonModel.h"
#include "Core/DungeonQuery.h"
#include "DungeonMarkerEmitter.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(DungeonMarkerEmitterLog, Log, All);

UENUM()
enum class EDungeonMarkerEmitterExecStage : uint8 {
	/**
	 * Execute before the pattern matcher
	 * Markers emitted by this blueprint are available to the pattern matcher
	 * However, the Patter Matcher emitted markers are not available to this blueprint, since they haven't executed yet
	 */
	BeforePatternMatcher,

	/**
	 * Executes after the pattern matcher
	 * Markers emitted using the Pattern Matcher are available to this blueprint
	 * However, markers emitted from this blueprint will not be available to the pattern matcher
	 */
	AfterPatternMatcher
};

/**
* Implement this class in blueprint (or C++) to emit your own custom markers in the scene
*/
UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable, abstract)
class DUNGEONARCHITECTRUNTIME_API UDungeonMarkerEmitter : public UObject {
    GENERATED_BODY()

public:

    /** Called by the theming engine to emit markers */
    UFUNCTION(BlueprintNativeEvent, Category = "Dungeon")
    void EmitMarkers(UDungeonBuilder* Builder, UDungeonModel* Model, UDungeonConfig* Config, UDungeonQuery* Query);
    virtual void EmitMarkers_Implementation(UDungeonBuilder* Builder, UDungeonModel* Model, UDungeonConfig* Config,
                                            UDungeonQuery* Query);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
	EDungeonMarkerEmitterExecStage ExecutionStage = EDungeonMarkerEmitterExecStage::BeforePatternMatcher;
};

