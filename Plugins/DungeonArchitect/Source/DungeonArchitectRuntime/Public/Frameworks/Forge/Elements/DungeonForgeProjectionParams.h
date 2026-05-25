//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonForgeProjectionParams.generated.h"

UENUM()
enum class EDungeonForgeProjectionColorBlendMode : uint8
{
	SourceValue,
	TargetValue,
	Add,
	Subtract,
	Multiply
};

UENUM()
enum class EDungeonForgeProjectionTagMergeMode : uint8
{
	Source,
	Target,
	Both
};

/** Parameters that control projection behaviour. */
USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonForgeProjectionParams
{
	GENERATED_BODY()

	/** Project positions. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Apply Data")
	bool bProjectPositions = true;

	/** Project rotations. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Apply Data")
	bool bProjectRotations = true;

	/** Project scales. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Apply Data")
	bool bProjectScales = false;

	/** The blend mode for colors during the projection */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Apply Data")
	EDungeonForgeProjectionColorBlendMode ColorBlendMode = EDungeonForgeProjectionColorBlendMode::SourceValue;

	/** Attributes to either explicitly exclude or include in the projection operation, depending on the Attribute Mode setting. Leave empty to gather all attributes and their values. Format is comma separated list like: Attribute1,Attribute2 .*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Apply Data")
	FString AttributeList = TEXT("");

	/** Controls whether the data tags are taken from the source, the target or both. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Apply Data")
	EDungeonForgeProjectionTagMergeMode TagMergeOperation = EDungeonForgeProjectionTagMergeMode::Source;
};

