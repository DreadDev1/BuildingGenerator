//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Common/Utils/DungeonForgeGraphParameterExtension.h"

#include "Frameworks/Forge/Graph/DungeonForgeGraph.h"

////////////
// Getters
////////////
template<> TValueOrError<float, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueFloat(PropertyName);
}
template<> TValueOrError<double, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueDouble(PropertyName);
}
template<> TValueOrError<bool, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueBool(PropertyName);
}
template<> TValueOrError<uint8, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueByte(PropertyName);
}
template<> TValueOrError<int32, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueInt32(PropertyName);
}
template<> TValueOrError<int64, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueInt64(PropertyName);
}
template<> TValueOrError<FName, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueName(PropertyName);
}
template<> TValueOrError<FString, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueString(PropertyName);
}
template<> TValueOrError<FSoftObjectPath*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueStruct<FSoftObjectPath>(PropertyName);
}
template<> TValueOrError<TSoftObjectPtr<UObject>, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	TValueOrError<FSoftObjectPath, EPropertyBagResult> InitialResult = PropertyBag.GetValueSoftPath(PropertyName);
	if (InitialResult.HasError())
	{
		return MakeError(InitialResult.GetError());
	}
	TValueOrError<TSoftObjectPtr<UObject>, EPropertyBagResult> FinalResult(MakeValue(TSoftObjectPtr<UObject>(InitialResult.GetValue())));
	return FinalResult;
}
template<> TValueOrError<TSoftClassPtr<UObject>, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	TValueOrError<FSoftObjectPath, EPropertyBagResult> InitialResult = PropertyBag.GetValueSoftPath(PropertyName);
	if (InitialResult.HasError())
	{
		return MakeError(InitialResult.GetError());
	}
	TValueOrError<TSoftClassPtr<UObject>, EPropertyBagResult> FinalResult(MakeValue(TSoftClassPtr<UObject>(InitialResult.GetValue())));
	return FinalResult;
}
template<> TValueOrError<UObject*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueObject<UObject>(PropertyName);
}
template<> TValueOrError<UClass*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueClass(PropertyName);
}
template<> TValueOrError<FVector*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueStruct<FVector>(PropertyName);
}
template<> TValueOrError<FRotator*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueStruct<FRotator>(PropertyName);
}
template<> TValueOrError<FTransform*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueStruct<FTransform>(PropertyName);
}
template<> TValueOrError<FVector4*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueStruct<FVector4>(PropertyName);
}
template<> TValueOrError<FVector2D*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueStruct<FVector2D>(PropertyName);
}
template<> TValueOrError<FQuat*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName)
{
	return PropertyBag.GetValueStruct<FQuat>(PropertyName);
}

TValueOrError<uint64, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName, const UEnum* Enum)
{
	// TODO: To stay congruent with BP Enums and Property Bag, Value converted from uint8. Update this if/when this is improved externally
	TValueOrError<uint8, EPropertyBagResult> Result =  Enum ? PropertyBag.GetValueEnum(PropertyName, Enum) : PropertyBag.GetValueByte(PropertyName);
	if (Result.HasError())
	{
		return MakeError(Result.GetError());
	}

	return MakeValue(static_cast<uint64>(Result.GetValue()));
}

////////////
// Setters
////////////
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const float& Value)
{
	return PropertyBag.SetValueFloat(PropertyName, Value);
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const double& Value)
{
	return PropertyBag.SetValueDouble(PropertyName, Value);
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const bool& bValue)
{
	return PropertyBag.SetValueBool(PropertyName, bValue);
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const uint8& Value)
{
	return PropertyBag.SetValueByte(PropertyName, Value);
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const int32& Value)
{
	return PropertyBag.SetValueInt32(PropertyName, Value);
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const int64& Value)
{
	return PropertyBag.SetValueInt64(PropertyName, Value);
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FName& Value)
{
	return PropertyBag.SetValueName(PropertyName, Value);
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FString& Value)
{
	return PropertyBag.SetValueString(PropertyName, Value);
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FSoftObjectPath& Value)
{
	return PropertyBag.SetValueStruct(PropertyName, Value);
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const TSoftObjectPtr<UObject>& Value)
{
	return PropertyBag.SetValueSoftPath(PropertyName, Value.ToSoftObjectPath());
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const TSoftClassPtr<UObject>& Value)
{
	return PropertyBag.SetValueSoftPath(PropertyName, Value.ToSoftObjectPath());
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, UObject* const& Value)
{
	return PropertyBag.SetValueObject<UObject>(PropertyName, Value);
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, UClass* const& Value)
{
	return PropertyBag.SetValueClass(PropertyName, Value);
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FVector& Value)
{
	return PropertyBag.SetValueStruct(PropertyName, Value);
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FRotator& Value)
{
	return PropertyBag.SetValueStruct(PropertyName, Value);
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FTransform& Value)
{
	return PropertyBag.SetValueStruct(PropertyName, Value);
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FVector4& Value)
{
	return PropertyBag.SetValueStruct(PropertyName, Value);
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FVector2D& Value)
{
	return PropertyBag.SetValueStruct(PropertyName, Value);
}
template<> EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FQuat& Value)
{
	return PropertyBag.SetValueStruct(PropertyName, Value);
}

EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const uint64& Value, const UEnum* Enum)
{
	// TODO: To stay congruent with BP Enums and Property Bag, Value converted to uint8. Update this if/when this is improved externally
	return Enum ? PropertyBag.SetValueEnum(PropertyName, static_cast<uint8>(Value), Enum) : PropertyBag.SetValueByte(PropertyName, static_cast<uint8>(Value));
}

