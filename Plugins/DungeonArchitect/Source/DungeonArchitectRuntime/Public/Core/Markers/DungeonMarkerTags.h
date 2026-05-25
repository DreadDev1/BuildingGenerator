//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Markers/DungeonMarker.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "DungeonMarkerTags.generated.h"

struct FDungeonMarkerInstance;

#define DA_MARKER_MGMT_PROPERTY_INT(PropertyName) \
    static void Set##PropertyName(FDungeonMarkerInstanceTags& InstanceTags, int32 Value) { SetProperty(InstanceTags.IntValues, TAG_NAME_##PropertyName, Value); } \
    static bool Get##PropertyName(const FDungeonMarkerInstanceTags& InstanceTags, int32& OutValue) { return GetProperty(InstanceTags.IntValues, TAG_NAME_##PropertyName, OutValue); } \
    static bool Contains##PropertyName(const FDungeonMarkerInstanceTags& InstanceTags, int32& OutValue) { return InstanceTags.IntValues.Contains(TAG_NAME_##PropertyName); } \
    static const FName TAG_NAME_##PropertyName;

struct FDungeonMarkerInstanceTagManagement {
private:
    template<typename T>
    static void SetProperty(TMap<FName, T>& ValueMap, const FName& InPropertyName, T Value) {
        ValueMap.FindOrAdd(InPropertyName) = Value;
    }

    template<typename T>
    static bool GetProperty(const TMap<FName, T>& ValueMap, const FName& InPropertyName, T& OutValue) {
        if (const T* ValuePtr = ValueMap.Find(InPropertyName)) {
            OutValue = *ValuePtr;
            return true;
        }
        return false;
    }
    
public:
    DA_MARKER_MGMT_PROPERTY_INT(ElevationFromGround)
    DA_MARKER_MGMT_PROPERTY_INT(CeilingElevation)
};

#undef DA_MARKER_MGMT_INT



// 2) Enum of known int tags
UENUM(BlueprintType)
enum class EDungeonMarkerIntTag : uint8
{
    ElevationFromGround,
    CeilingElevation,
};

UCLASS()
class UDungeonMarkerInstanceTagLib : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // Getter is Pure
    UFUNCTION(BlueprintPure, Category="Dungeon|Tags")
    static int32 GetDungeonMarkerIntTag(const FDungeonMarkerInstance& Instance, EDungeonMarkerIntTag Tag, int32 DefaultValue = 0);

private:
    static FName ToTagName(EDungeonMarkerIntTag Tag)
    {
        switch (Tag)
        {
        case EDungeonMarkerIntTag::ElevationFromGround:     return FName(TEXT("Room.Elevation.Current"));
        case EDungeonMarkerIntTag::CeilingElevation:        return FName(TEXT("Room.Elevation.Ceiling"));
        }
        return NAME_None;
    }
};

