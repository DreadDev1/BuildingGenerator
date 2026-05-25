//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "StructUtils/PropertyBag.h"

/** Extension helper class to allow templatization without dirtying the DungeonForgeGraph.h file */
class DUNGEONARCHITECTRUNTIME_API FDungeonForgeGraphParameterExtension
{
	template <typename T> static bool constexpr StaticFail{false};

public:
	// Catch all implicit templates for unsupported types
	template <typename T>
	static TValueOrError<T, EPropertyBagResult> GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName) { static_assert(StaticFail<T>, "Invalid graph parameter type"); return MakeError(EPropertyBagResult::PropertyNotFound); }
	template <typename T>
	static EPropertyBagResult SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const T& Value) { static_assert(StaticFail<T>, "Invalid graph parameter type"); return EPropertyBagResult::PropertyNotFound; }

	// Enum Objects are a special signature
	static TValueOrError<uint64, EPropertyBagResult> GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName, const UEnum* Enum);
	static EPropertyBagResult SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const uint64& Value, const UEnum* Enum);
};

// Template declarations necessary for DLL API
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<float, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<double, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<bool, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<uint8, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<int32, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<int64, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<FName, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<FString, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<FSoftObjectPath*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<TSoftObjectPtr<UObject>, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<TSoftClassPtr<UObject>, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<UObject*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<UClass*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<FVector*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<FRotator*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<FTransform*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<FVector4*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<FVector2D*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);
template<> DUNGEONARCHITECTRUNTIME_API TValueOrError<FQuat*, EPropertyBagResult> FDungeonForgeGraphParameterExtension::GetGraphParameter(const FInstancedPropertyBag& PropertyBag, const FName PropertyName);

template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const float& Value);
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const double& Value);
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const bool& bValue);
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const uint8& Value);
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const int32& Value);
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const int64& Value);
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FName& Value);
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FString& Value);
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FSoftObjectPath& Value);
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const TSoftObjectPtr<UObject>& Value);
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const TSoftClassPtr<UObject>& Value);
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, UObject* const& Value); // UObject* const& to respect template specialization
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, UClass* const& Value); // UClass* const& to respect template specialization
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FVector& Value);
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FRotator& Value);
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FTransform& Value);
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FVector4& Value);
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FVector2D& Value);
template<> DUNGEONARCHITECTRUNTIME_API EPropertyBagResult FDungeonForgeGraphParameterExtension::SetGraphParameter(FInstancedPropertyBag& PropertyBag, const FName PropertyName, const FQuat& Value);

