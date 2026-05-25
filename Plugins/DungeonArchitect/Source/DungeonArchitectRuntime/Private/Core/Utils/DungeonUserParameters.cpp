//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Utils/DungeonUserParameters.h"

#include "Templates/SubclassOf.h"

FDungeonUserParameterValueEntry::FDungeonUserParameterValueEntry() = default;

FDungeonUserParameterValueEntry::FDungeonUserParameterValueEntry(int32 InValue)
	: ActiveType(EDungeonUserParameterType::Int)
	, IntValue(InValue)
{
}

FDungeonUserParameterValueEntry::FDungeonUserParameterValueEntry(float InValue)
	: ActiveType(EDungeonUserParameterType::Float)
	, FloatValue(InValue)
{
}

FDungeonUserParameterValueEntry::FDungeonUserParameterValueEntry(const FVector& InValue)
	: ActiveType(EDungeonUserParameterType::Vector)
	, VectorValue(InValue)
{
}

FDungeonUserParameterValueEntry::FDungeonUserParameterValueEntry(const FTransform& InValue)
	: ActiveType(EDungeonUserParameterType::Transform)
	, TransformValue(InValue)
{
}

FDungeonUserParameterValueEntry::FDungeonUserParameterValueEntry(UObject* InValue)
	: ActiveType(EDungeonUserParameterType::Object)
	, ObjectValue(InValue)
{
}

FDungeonUserParameterCollection::FDungeonUserParameterCollection() = default;

void FDungeonUserParameterCollection::SetInt(FName Key, int32 Value) {
	Parameters.Add(Key, FDungeonUserParameterValueEntry(Value));
}

void FDungeonUserParameterCollection::SetFloat(FName Key, float Value) {
	Parameters.Add(Key, FDungeonUserParameterValueEntry(Value));
}

void FDungeonUserParameterCollection::SetObject(FName Key, UObject* Value) {
	Parameters.Add(Key, FDungeonUserParameterValueEntry(Value));
}

void FDungeonUserParameterCollection::SetVector(FName Key, const FVector& Value) {
	Parameters.Add(Key, FDungeonUserParameterValueEntry(Value));
}

void FDungeonUserParameterCollection::SetTransform(FName Key, const FTransform& Value) {
	Parameters.Add(Key, FDungeonUserParameterValueEntry(Value));
}

int32 FDungeonUserParameterCollection::GetInt(FName Key, int32 DefaultValue) const {
	const FDungeonUserParameterValueEntry* Entry = Parameters.Find(Key);
	if (Entry && Entry->ActiveType == EDungeonUserParameterType::Int) {
		return Entry->IntValue;
	}
	return DefaultValue;
}

float FDungeonUserParameterCollection::GetFloat(FName Key, float DefaultValue) const {
	const FDungeonUserParameterValueEntry* Entry = Parameters.Find(Key);
	if (Entry && Entry->ActiveType == EDungeonUserParameterType::Float) {
		return Entry->FloatValue;
	}
	return DefaultValue;
}

FVector FDungeonUserParameterCollection::GetVector(FName Key, const FVector& DefaultValue) const {
	const FDungeonUserParameterValueEntry* Entry = Parameters.Find(Key);
	if (Entry && Entry->ActiveType == EDungeonUserParameterType::Vector) {
		return Entry->VectorValue;
	}
	return DefaultValue;
}

FTransform FDungeonUserParameterCollection::GetTransform(FName Key, const FTransform& DefaultValue) const {
	const FDungeonUserParameterValueEntry* Entry = Parameters.Find(Key);
	if (Entry && Entry->ActiveType == EDungeonUserParameterType::Transform) {
		return Entry->TransformValue;
	}
	return DefaultValue;
}

UObject* FDungeonUserParameterCollection::GetObject(FName Key, UObject* DefaultValue) const {
	const FDungeonUserParameterValueEntry* Entry = Parameters.Find(Key);
	if (Entry && Entry->ActiveType == EDungeonUserParameterType::Object) {
		if (Entry->ObjectValue.IsValid()) {
			return Entry->ObjectValue.Get();
		}
	}
	return DefaultValue;
}

template <typename T>
T* FDungeonUserParameterCollection::GetObject(FName Key, T* DefaultValue) const {
	static_assert(TIsDerivedFrom<T, UObject>::IsDerived, "T must be derived from UObject");
	UObject* RawObject = GetObject(Key, nullptr); // Use base GetObject
	T* CastedObject = RawObject ? Cast<T>(RawObject) : nullptr;
	return CastedObject ? CastedObject : DefaultValue;
}

UObject* FDungeonUserParameterCollection::GetObjectOfClass(FName Key, TSubclassOf<UObject> ObjectClass,
	UObject* DefaultValue) const {
	UObject* FoundObject = GetObject(Key, nullptr);
	if (FoundObject && (!ObjectClass || FoundObject->IsA(ObjectClass))) {
		return FoundObject;
	}
	return DefaultValue;
}

bool FDungeonUserParameterCollection::Contains(FName Key) const {
	return Parameters.Contains(Key);
}

EDungeonUserParameterType FDungeonUserParameterCollection::GetParameterType(FName Key) const {
	const FDungeonUserParameterValueEntry* Entry = Parameters.Find(Key);
	return Entry ? Entry->ActiveType : EDungeonUserParameterType::None;
}

void FDungeonUserParameterCollection::Clear() {
	Parameters.Empty();
}

void FDungeonUserParameterCollection::Merge(const FDungeonUserParameterCollection& OtherCollection) {
	Parameters.Append(OtherCollection.Parameters);
}

