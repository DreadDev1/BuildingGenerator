//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonUserParameters.generated.h"

UENUM(BlueprintType)
enum class EDungeonUserParameterType : uint8 {
	None UMETA(Hidden),
	Int UMETA(DisplayName="Integer"),
	Float UMETA(DisplayName="Float"),
	Vector UMETA(DisplayName="Vector"),
	Transform UMETA(DisplayName="Transform"),
	Object UMETA(DisplayName="Object")
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonUserParameterValueEntry {
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Parameter Entry")
	EDungeonUserParameterType ActiveType = EDungeonUserParameterType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Parameter Entry")
	int32 IntValue = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Parameter Entry")
	float FloatValue = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Parameter Entry")
	FVector VectorValue = FVector::ZeroVector;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Parameter Entry")
	FTransform TransformValue = FTransform::Identity;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Parameter Entry")
	TWeakObjectPtr<UObject> ObjectValue = nullptr;

	FDungeonUserParameterValueEntry();

	explicit FDungeonUserParameterValueEntry(int32 InValue);
	explicit FDungeonUserParameterValueEntry(float InValue);
	explicit FDungeonUserParameterValueEntry(const FVector& InValue);
	explicit FDungeonUserParameterValueEntry(const FTransform& InValue);
	explicit FDungeonUserParameterValueEntry(UObject* InValue);
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDungeonUserParameterCollection {
	GENERATED_BODY()

private:
	UPROPERTY()
	TMap<FName, FDungeonUserParameterValueEntry> Parameters;

public:
	FDungeonUserParameterCollection();
	void SetInt(FName Key, int32 Value);
	void SetFloat(FName Key, float Value);
	void SetObject(FName Key, UObject* Value);
	void SetVector(FName Key, const FVector& Value);
	void SetTransform(FName Key, const FTransform& Value);
	int32 GetInt(FName Key, int32 DefaultValue = 0) const;
	float GetFloat(FName Key, float DefaultValue = 0.0f) const;
	FVector GetVector(FName Key, const FVector& DefaultValue = FVector::Zero()) const;
	FTransform GetTransform(FName Key, const FTransform& DefaultValue = FTransform::Identity) const;
	
	UObject* GetObject(FName Key, UObject* DefaultValue = nullptr) const;
	
	template<typename T>
	T* GetObject(FName Key, T* DefaultValue = nullptr) const;
	
	UObject* GetObjectOfClass(FName Key, TSubclassOf<UObject> ObjectClass, UObject* DefaultValue = nullptr) const;
	bool Contains(FName Key) const;
	EDungeonUserParameterType GetParameterType(FName Key) const;
	void Clear();
	void Merge(const FDungeonUserParameterCollection& OtherCollection);
};

