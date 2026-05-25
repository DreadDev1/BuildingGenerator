//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/DungeonForgeCommon.h"
#include "DungeonForgeData.generated.h"

class UDungeonDebugVisualizer;
class UDungeonForgeNodeSettings;
class UDungeonForgeNodeSettingsInterface;
class UDungeonForgeGraphNode;
/**
* Base class for any "data" class in the DungeonForge framework.
* This is an intentionally vague base class so we can have the required
* flexibility to pass in various concrete data types, settings, and more.
*/
UCLASS(BlueprintType, ClassGroup = (Procedural))
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeData : public UObject
{
	GENERATED_BODY()

public:
	UDungeonForgeData(const FObjectInitializer& ObjectInitializer);
	virtual EDungeonForgeDataType GetDataType() const { return EDungeonForgeDataType::None; }

	/** Executes a lambda over all connected data objects. */
	virtual void VisitDataNetwork(TFunctionRef<void(const UDungeonForgeData*)> Action) const;

	/** Unique ID for this object instance. */
	UPROPERTY(Transient)
	uint64 UID = 0;

	/** Return a copy of the data, with Metadata inheritence for spatial data. */
	virtual UDungeonForgeData* DuplicateData() const;

	// ~Begin UObject interface
	virtual void PostDuplicate(bool bDuplicateForPIE) override { InitUID(); }
	virtual void PostEditImport() override;
	// ~End UObject interface

#if WITH_EDITOR
	virtual void WriteDebugData(UDungeonDebugVisualizer* InDebugVisualizer) const {}
#endif // WITH_EDITOR
	
private:
	void InitUID();

	/** Serves unique ID values to instances of this object. */
	static inline std::atomic<uint64> UIDCounter{ 1 };
};


USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonForgeTaggedData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Data)
	TObjectPtr<const UDungeonForgeData> Data = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Data)
	TSet<FString> Tags;

	/** The label of the pin that this data was either emitted from or received on. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Data)
	FName Pin = NAME_None;

	// Special flag for data that are forwarded to other nodes, but without a pin. Useful for internal data.
	UPROPERTY()
	bool bPinlessData = false;
	
	/**
	* Special flag to be modified by execution when a data is used multiple times (in this node or other nodes),
	* to enable optimization when they are not. Always assume that it is true by default.
	*/
	UPROPERTY(BlueprintReadOnly, Transient, Category = Data)
	bool bIsUsedMultipleTimes = true;

#if !UE_BUILD_SHIPPING
	// For debug tracking.
	TWeakObjectPtr<const UDungeonForgeGraphNode> OriginatingNode;
#endif // !UE_BUILD_SHIPPING

	// Used to re-order elements when using per-data caching
	int32 OriginalIndex = INDEX_NONE;

	bool operator==(const FDungeonForgeTaggedData& Other) const;
	bool operator!=(const FDungeonForgeTaggedData& Other) const;
};

USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonForgeDataCollection
{
	GENERATED_BODY()

	/** Returns all spatial data in the collection */
	TArray<FDungeonForgeTaggedData> GetInputs() const;
	/** Returns all data on a given pin. */
	TArray<FDungeonForgeTaggedData> GetInputsByPin(const FName& InPinLabel) const;
	/** Returns all spatial data on a given pin */
	TArray<FDungeonForgeTaggedData> GetSpatialInputsByPin(const FName& InPinLabel) const;

	/** Returns all data and corresponding cached data CRCs for a given pin. */
	template<typename AllocatorType1, typename AllocatorType2>
	void GetInputsByPin(const FName& InPinLabel, TArray<FDungeonForgeTaggedData, AllocatorType1>& OutData) const
	{
		for (int I = 0; I < TaggedData.Num(); ++I) {
			if (ensure(TaggedData[I].Data) && TaggedData[I].Pin == InPinLabel) {
				OutData.Add(TaggedData[I]);
			}
		}
	}

	template<typename TData>
	const TData* GetInputByPinOfType(const FName& InPinLabel) const {
		TArray<FDungeonForgeTaggedData> PinDataList = GetInputsByPin(InPinLabel);
		for (FDungeonForgeTaggedData& PinData : PinDataList) {
			if (const TData* TargetData = Cast<const TData>(PinData.Data)) {
				return TargetData;
			}
		}
		return nullptr;
	}

	template<typename TData>
	TArray<const TData*> GetInputsByPinOfType(const FName& InPinLabel) const {
		TArray<const TData*> Result;
		TArray<FDungeonForgeTaggedData> PinDataList = GetInputsByPin(InPinLabel);
		for (FDungeonForgeTaggedData& PinData : PinDataList) {
			if (const TData* TargetData = Cast<const TData>(PinData.Data)) {
				Result.Add(TargetData);
			}
		}
		return Result;
	}
	
	/** Gets number of data items on a given pin */
	int32 GetInputCountByPin(const FName& InPinLabel) const;
	/** Gets number of spatial data items on a given pin */
	int32 GetSpatialInputCountByPin(const FName& InPinLabel) const;

	/** Returns all spatial data in the collection with the given tag */
	TArray<FDungeonForgeTaggedData> GetTaggedInputs(const FString& InTag) const;
	/** Returns all settings in the collection */
	TArray<FDungeonForgeTaggedData> GetAllSettings() const;

	/** Returns all data in the collection with the given tag and given type */
	template <typename DungeonForgeDataType>
	TArray<FDungeonForgeTaggedData> GetTaggedTypedInputs(const FString& InTag) const;

	const UDungeonForgeNodeSettingsInterface* GetSettingsInterface() const;
	const UDungeonForgeNodeSettingsInterface* GetSettingsInterface(const UDungeonForgeNodeSettingsInterface* InDefaultSettingsInterface) const;

	/** Memory size calculation. Forward call to the data objects in the collection. */
	void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) const;

	template<typename SettingsType>
	const SettingsType* GetSettings() const;

	const UDungeonForgeNodeSettings* GetSettings(const UDungeonForgeNodeSettings* InDefaultSettings) const;

	bool operator==(const FDungeonForgeDataCollection& Other) const;
	bool operator!=(const FDungeonForgeDataCollection& Other) const;
	void AddReferences(FReferenceCollector& Collector);

	/** Add data and CRC to collection. */
	void AddData(const FDungeonForgeTaggedData& InData);
	/** Add data and CRCs to collection. */
	void AddData(const TConstArrayView<FDungeonForgeTaggedData>& InData);

	/** Cleans up the collection, but does not unroot any previously rooted data. */
	void Reset();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Data)
	TArray<FDungeonForgeTaggedData> TaggedData;

	/** Deprecated - Will be removed in 5.4 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Data)
	bool bCancelExecutionOnEmpty = false;

	/** This flag is used to cancel further computation or for the debug/isolate feature */
	bool bCancelExecution = false;

	/** After the task is complete, bit j is set if output pin index j is deactivated. Stored here so that it can be retrieved from the cache. */
	uint64 InactiveOutputPinBitmask = 0;
};

template<typename SettingsType>
inline const SettingsType* FDungeonForgeDataCollection::GetSettings() const
{
	const FDungeonForgeTaggedData* MatchingData = TaggedData.FindByPredicate([](const FDungeonForgeTaggedData& Data) {
		return Cast<const SettingsType>(Data.Data) != nullptr;
		});

	return MatchingData ? Cast<const SettingsType>(MatchingData->Data) : nullptr;
}

template <typename DungeonForgeDataType>
inline TArray<FDungeonForgeTaggedData> FDungeonForgeDataCollection::GetTaggedTypedInputs(const FString& InTag) const
{
	return TaggedData.FilterByPredicate([&InTag](const FDungeonForgeTaggedData& Data) {
		return Data.Tags.Contains(InTag) && Cast<DungeonForgeDataType>(Data.Data);
	});
}

