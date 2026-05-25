//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Markers/DungeonMarker.h"
#include "ProceduralDungeonTransformLogic.generated.h"

class ADungeon;

/**
 *
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable, HideDropDown)
class DUNGEONARCHITECTRUNTIME_API UProceduralDungeonTransformLogic : public UObject {
	GENERATED_BODY()
public:
	virtual FTransform Execute(ADungeon* InDungeon, const FRandomStream& InRandom, const FDungeonMarkerInstance& InMarker) { return FTransform::Identity; }
};

/**
 *
 */
UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable)
class DUNGEONARCHITECTRUNTIME_API URandomTranslateProcTransformLogic : public UProceduralDungeonTransformLogic {
	GENERATED_BODY()
public:
	virtual FTransform Execute(ADungeon* InDungeon, const FRandomStream& InRandom, const FDungeonMarkerInstance& InMarker) override;

	UPROPERTY(EditAnywhere, Category="Random Transform")
	float MoveAlongX{};
	
	UPROPERTY(EditAnywhere, Category="Random Transform")
	float MoveAlongY{};
	
	UPROPERTY(EditAnywhere, Category="Random Transform")
	float MoveAlongZ{};
};


/**
 *
 */
UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable)
class DUNGEONARCHITECTRUNTIME_API URandomJitterProcTransformLogic : public UProceduralDungeonTransformLogic {
	GENERATED_BODY()
public:
	virtual FTransform Execute(ADungeon* InDungeon, const FRandomStream& InRandom, const FDungeonMarkerInstance& InMarker) override;

	UPROPERTY(EditAnywhere, Category=" Jitter")
	float JitterDistance{3};
	
	UPROPERTY(EditAnywhere, Category=" Jitter")
	bool JitterAlongXY{true};
	
	UPROPERTY(EditAnywhere, Category="Jitter")
	bool JitterAlongZ{};
};


/**
 * Rotate randomly along the Z axis
 */
UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable)
class DUNGEONARCHITECTRUNTIME_API URandomRotateZProcTransformLogic : public UProceduralDungeonTransformLogic {
	GENERATED_BODY()
public:
	virtual FTransform Execute(ADungeon* InDungeon, const FRandomStream& InRandom, const FDungeonMarkerInstance& InMarker) override;
};

/**
 * Rotate randomly along the Z axis in 90 degrees steps
 */
UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable)
class DUNGEONARCHITECTRUNTIME_API URandomRotateZ90ProcTransformLogic : public UProceduralDungeonTransformLogic {
	GENERATED_BODY()
public:
	virtual FTransform Execute(ADungeon* InDungeon, const FRandomStream& InRandom, const FDungeonMarkerInstance& InMarker) override;
};


/**
 * Rotate randomly along the Z axis
 */
UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable)
class DUNGEONARCHITECTRUNTIME_API URandomRotateProcTransformLogic : public UProceduralDungeonTransformLogic {
	GENERATED_BODY()
public:
	virtual FTransform Execute(ADungeon* InDungeon, const FRandomStream& InRandom, const FDungeonMarkerInstance& InMarker) override;

	UPROPERTY(EditAnywhere, Category="Rotation")
	FVector MinRotation = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, Category=" Rotation")
	FVector MaxRotation = FVector(360, 360, 360);
};


/**
 * Clamps the marker to the ceiling, if one is provided. Ignored otherwise
 */
UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType, Blueprintable)
class DUNGEONARCHITECTRUNTIME_API UClampToCeilingProcTransformLogic : public UProceduralDungeonTransformLogic {
	GENERATED_BODY()
public:
	virtual FTransform Execute(ADungeon* InDungeon, const FRandomStream& InRandom, const FDungeonMarkerInstance& InMarker) override;

};

