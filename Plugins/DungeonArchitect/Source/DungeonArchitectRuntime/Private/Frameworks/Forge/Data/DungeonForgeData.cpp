//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Data/DungeonForgeData.h"

#include "Frameworks/Forge/Data/DungeonForgeSpatialData.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"

#include "UObject/Package.h"

UDungeonForgeData::UDungeonForgeData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (!HasAnyFlags(RF_ClassDefaultObject)) {
		InitUID();
	}
}

void UDungeonForgeData::VisitDataNetwork(TFunctionRef<void(const UDungeonForgeData*)> Action) const {
	Action(this);
}

UDungeonForgeData* UDungeonForgeData::DuplicateData() const {
	ensure(IsInGameThread());
	return Cast<UDungeonForgeData>(StaticDuplicateObject(this, GetTransientPackage()));
}

void UDungeonForgeData::PostEditImport() {
	UObject::PostEditImport();
	InitUID();
}

void UDungeonForgeData::InitUID() {
	UID = ++UIDCounter;
}

bool FDungeonForgeTaggedData::operator==(const FDungeonForgeTaggedData& Other) const {
	return Data == Other.Data &&
		Tags.Num() == Other.Tags.Num() &&
		Tags.Includes(Other.Tags) &&
		Pin == Other.Pin;
}

bool FDungeonForgeTaggedData::operator!=(const FDungeonForgeTaggedData& Other) const {
	return !operator==(Other);
}

/////////////////////////////// FDungeonForgeDataCollection ///////////////////////////////
int32 FDungeonForgeDataCollection::GetInputCountByPin(const FName& InPinLabel) const {
	int32 Count = 0;
	for (const FDungeonForgeTaggedData& Data : TaggedData)
	{
		if (Data.Pin == InPinLabel)
		{
			++Count;
		}
	}
	return Count;
}

int32 FDungeonForgeDataCollection::GetSpatialInputCountByPin(const FName& InPinLabel) const {
	int32 Count = 0;
	for (const FDungeonForgeTaggedData& Data : TaggedData)
	{
		if (Data.Pin == InPinLabel && Data.Data && Data.Data.IsA<UDungeonForgeSpatialData>())
		{
			++Count;
		}
	}
	return Count;
}

TArray<FDungeonForgeTaggedData> FDungeonForgeDataCollection::GetTaggedInputs(const FString& InTag) const {
	return GetTaggedTypedInputs<UDungeonForgeSpatialData>(InTag);
}

TArray<FDungeonForgeTaggedData> FDungeonForgeDataCollection::GetAllSettings() const {
	return TaggedData.FilterByPredicate([](const FDungeonForgeTaggedData& Data) {
		return Cast<UDungeonForgeNodeSettings>(Data.Data) != nullptr;
		});
}

const UDungeonForgeNodeSettingsInterface* FDungeonForgeDataCollection::GetSettingsInterface() const {
	return GetSettings<UDungeonForgeNodeSettingsInterface>();
}

const UDungeonForgeNodeSettingsInterface* FDungeonForgeDataCollection::GetSettingsInterface(const UDungeonForgeNodeSettingsInterface* InDefaultSettingsInterface) const {
	if (!InDefaultSettingsInterface || InDefaultSettingsInterface->GetSettings() == nullptr)
	{
		return GetSettingsInterface();
	}
	else
	{
		const FDungeonForgeTaggedData* MatchingData = TaggedData.FindByPredicate([InDefaultSettingsInterface](const FDungeonForgeTaggedData& Data) {
			if (const UDungeonForgeNodeSettingsInterface* DataSettingsInterface = Cast<UDungeonForgeNodeSettingsInterface>(Data.Data))
			{
				// Compare settings classes
				return DataSettingsInterface->GetSettings()->GetClass() == InDefaultSettingsInterface->GetSettings()->GetClass() ||
					DataSettingsInterface->GetSettings()->GetClass()->IsChildOf(InDefaultSettingsInterface->GetSettings()->GetClass());
			}

			return false;
		});

		return MatchingData ? Cast<const UDungeonForgeNodeSettingsInterface>(MatchingData->Data) : InDefaultSettingsInterface;
	}
}

void FDungeonForgeDataCollection::GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) const {
	for (const FDungeonForgeTaggedData& Data : TaggedData)
	{
		if (Data.Data)
		{
			Data.Data->VisitDataNetwork([&CumulativeResourceSize](const UDungeonForgeData* Data) {
				// Cast away const-ness. The extended mode of GetResourceSizeEx accounts memory for all objects outer'd
				// to this object, and that calls GetObjectsWithOuter which is non-const. We don't use this extended mode
				// and we need to be able to operate on const objects.
				const_cast<UDungeonForgeData*>(Data)->GetResourceSizeEx(CumulativeResourceSize);
			});
		}
	}
}

const UDungeonForgeNodeSettings* FDungeonForgeDataCollection::GetSettings(const UDungeonForgeNodeSettings* InDefaultSettings) const {
	if (!InDefaultSettings)
	{
		return GetSettings<UDungeonForgeNodeSettings>();
	}
	else
	{
		const FDungeonForgeTaggedData* MatchingData = TaggedData.FindByPredicate([InDefaultSettings](const FDungeonForgeTaggedData& Data) {
			return Data.Data &&
				(Data.Data->GetClass() == InDefaultSettings->GetClass() ||
					Data.Data->GetClass()->IsChildOf(InDefaultSettings->GetClass()));
			});

		return MatchingData ? Cast<const UDungeonForgeNodeSettings>(MatchingData->Data) : InDefaultSettings;
	}
}

bool FDungeonForgeDataCollection::operator==(const FDungeonForgeDataCollection& Other) const {
	if (bCancelExecution != Other.bCancelExecution || TaggedData.Num() != Other.TaggedData.Num())
	{
		return false;
	}

	// TODO: Once we make the arguments order irrelevant, then this should be updated
	for (int32 DataIndex = 0; DataIndex < TaggedData.Num(); ++DataIndex)
	{
		if (TaggedData[DataIndex] != Other.TaggedData[DataIndex])
		{
			return false;
		}
	}
	
	return true;
}

bool FDungeonForgeDataCollection::operator!=(const FDungeonForgeDataCollection& Other) const {
	return !operator==(Other);
}
 
TArray<FDungeonForgeTaggedData> FDungeonForgeDataCollection::GetInputs() const {
	return TaggedData.FilterByPredicate([](const FDungeonForgeTaggedData& Data) {
		return Cast<UDungeonForgeSpatialData>(Data.Data) != nullptr;
		});
}

TArray<FDungeonForgeTaggedData> FDungeonForgeDataCollection::GetInputsByPin(const FName& InPinLabel) const {
	return TaggedData.FilterByPredicate([&InPinLabel](const FDungeonForgeTaggedData& Data) {
		if (!ensure(Data.Data))
		{
			return false;
		}

		return Data.Pin == InPinLabel;
	});
}

TArray<FDungeonForgeTaggedData> FDungeonForgeDataCollection::GetSpatialInputsByPin(const FName& InPinLabel) const {
	return TaggedData.FilterByPredicate([&InPinLabel](const FDungeonForgeTaggedData& Data) {
		if (!ensure(Data.Data))
		{
			return false;
		}

		return Data.Pin == InPinLabel && Data.Data.IsA<UDungeonForgeSpatialData>();
	});
}

void FDungeonForgeDataCollection::AddReferences(FReferenceCollector& Collector) {
	for (FDungeonForgeTaggedData& Data : TaggedData)
	{
		if (Data.Data)
		{
			Collector.AddReferencedObject(Data.Data);
		}
	}
}

void FDungeonForgeDataCollection::AddData(const FDungeonForgeTaggedData& InData) {
	TaggedData.Add(InData);
}

void FDungeonForgeDataCollection::AddData(const TConstArrayView<FDungeonForgeTaggedData>& InData) {
	TaggedData.Append(InData);
}

void FDungeonForgeDataCollection::Reset() {
	// Implementation note: We are assuming that there is no need to remove the data from the rootset here.
	TaggedData.Reset();
	bCancelExecution = false;
}

