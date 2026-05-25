//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/MarkerGenerator/MarkerGenProcessor.h"
#include "Frameworks/MarkerGenerator/PatternScript/PatternScriptNode.h"
#include "PatternScriptNodesImpl.generated.h"

// NOTE: This is an auto-generated file.  Do not modify,  update the definitions.py file instead

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UMGPatternScriptNode_CopyMarkers : public UMGPatternActionScriptNodeBase {
	GENERATED_BODY()
public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle() const override;
	virtual FName GetOutputExecPinId() const override { return OutPinID_Default; }
	virtual FString GetActionText() const override;
	virtual void Execute(const FMGActionExecContext& InContext) override;

public:
	UPROPERTY(EditAnywhere, Category="Copy Markers")
	FString SourceMarkerName;
	
	UPROPERTY(EditAnywhere, Category="Copy Markers")
	FString TargetMarkerName;
	
	static const FName InPinID_Default;
	static const FName OutPinID_Default;
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UMGPatternScriptNode_EmitMarker : public UMGPatternActionScriptNodeBase {
	GENERATED_BODY()
public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle() const override;
	virtual FName GetOutputExecPinId() const override { return OutPinID_Default; }
	virtual FString GetActionText() const override;
	virtual void Execute(const FMGActionExecContext& InContext) override;

public:
	UPROPERTY(EditAnywhere, Category="Emit Marker")
	FString MarkerName;
	
	/** Emit copies of this marker along all the vertical elevations of this room, on each level */
	UPROPERTY(EditAnywhere, Category="Emit Marker")
	bool bEmitCopiesAlongAllElevations;
	
	/** Copy the rotation from one of the markers found in this list */
	UPROPERTY(EditAnywhere, Category="Copy Transform")
	TArray<FString> CopyRotationFromMarkers;
	
	/** Copy the height from one of the markers found in this list */
	UPROPERTY(EditAnywhere, Category="Copy Transform")
	TArray<FString> CopyHeightFromMarkers;
	
	/** Markers emitted into the scene will be removed if they are found to be duplicates.
	 * Specify the type of duplication check you'd like to perform
	 * CheckLocationOnly - Only one marker of the same name is allowed at a certain location (e.g. can't have multiple rotated markers of the same name at the same location)
	 * CheckLocationAndRotation - Only one marker of a certain location and rotation is allowed.   This allows you to have multiple markers at the same place with different rotations
	 * CheckFullTransform - Scale is also considered */
	UPROPERTY(EditAnywhere, Category="Duplicate Removal")
	EMGMarkerListDuplicateCheckMethod DuplicateMarkerRemoveRule = EMGMarkerListDuplicateCheckMethod::CheckLocationOnly;
	
	static const FName InPinID_Default;
	static const FName OutPinID_Default;
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UMGPatternScriptNode_RemoveMarker : public UMGPatternActionScriptNodeBase {
	GENERATED_BODY()
public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle() const override;
	virtual FName GetOutputExecPinId() const override { return OutPinID_Default; }
	virtual FString GetActionText() const override;
	virtual void Execute(const FMGActionExecContext& InContext) override;

public:
	UPROPERTY(EditAnywhere, Category="Remove Marker")
	FString MarkerName;
	
	static const FName InPinID_Default;
	static const FName OutPinID_Default;
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UMGPatternScriptNode_RenameMarkers : public UMGPatternActionScriptNodeBase {
	GENERATED_BODY()
public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle() const override;
	virtual FName GetOutputExecPinId() const override { return OutPinID_Default; }
	virtual FString GetActionText() const override;
	virtual void Execute(const FMGActionExecContext& InContext) override;

public:
	UPROPERTY(EditAnywhere, Category="Rename Markers")
	FString SourceMarkerName;
	
	UPROPERTY(EditAnywhere, Category="Rename Markers")
	FString TargetMarkerName;
	
	static const FName InPinID_Default;
	static const FName OutPinID_Default;
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UMGPatternScriptNode_LogicalAnd : public UMGPatternConditionalScriptNodeBase {
	GENERATED_BODY()
public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle() const override;
	virtual FString GenerateRuleText() const override;
	virtual bool Execute(const FMGConditionExecContext& InContext) override;

public:
	static const FName InPinID_A;
	static const FName InPinID_B;
	static const FName OutPinID_Default;
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UMGPatternScriptNode_LogicalNot : public UMGPatternConditionalScriptNodeBase {
	GENERATED_BODY()
public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle() const override;
	virtual FString GenerateRuleText() const override;
	virtual bool Execute(const FMGConditionExecContext& InContext) override;

public:
	static const FName InPinID_Default;
	static const FName OutPinID_Default;
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UMGPatternScriptNode_LogicalOr : public UMGPatternConditionalScriptNodeBase {
	GENERATED_BODY()
public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle() const override;
	virtual FString GenerateRuleText() const override;
	virtual bool Execute(const FMGConditionExecContext& InContext) override;

public:
	static const FName InPinID_A;
	static const FName InPinID_B;
	static const FName OutPinID_Default;
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UMGPatternScriptNode_MarkerExists : public UMGPatternConditionalScriptNodeBase {
	GENERATED_BODY()
public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle() const override;
	virtual FString GenerateRuleText() const override;
	virtual bool Execute(const FMGConditionExecContext& InContext) override;

public:
	UPROPERTY(EditAnywhere, Category="Markers")
	FString MarkerName;
	
	UPROPERTY(EditAnywhere, Category="Elevation")
	TArray<int32> RequiredElevations;
	
	static const FName OutPinID_Default;
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UMGPatternScriptNode_MarkerListExists : public UMGPatternConditionalScriptNodeBase {
	GENERATED_BODY()
public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle() const override;
	virtual FString GenerateRuleText() const override;
	virtual bool Execute(const FMGConditionExecContext& InContext) override;

public:
	/** List of marker names to check for */
	UPROPERTY(EditAnywhere, Category="Markers")
	TArray<FString> MarkerNames;
	
	static const FName OutPinID_Default;
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UMGPatternScriptNode_OnPass : public UMGPatternActionScriptNodeBase {
	GENERATED_BODY()
public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle() const override;
	virtual FName GetOutputExecPinId() const override { return OutPinID_Default; }
	virtual FString GetActionText() const override;
	virtual void Execute(const FMGActionExecContext& InContext) override;

public:
	static const FName OutPinID_Default;
};


UCLASS()
class DUNGEONARCHITECTRUNTIME_API UMGPatternScriptNode_Result : public UMGPatternConditionalScriptNodeBase {
	GENERATED_BODY()
public:
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle() const override;
	virtual FString GenerateRuleText() const override;
	virtual bool Execute(const FMGConditionExecContext& InContext) override;

public:
	static const FName InPinID_Select;
};


