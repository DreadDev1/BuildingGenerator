//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"

#include "Frameworks/MarkerGenerator/MarkerGenModel.h"
#include "Frameworks/ThemeEngine/Graph/DungeonThemeCompiledGraphUpgrader.h"

UDungeonThemeAsset::UDungeonThemeAsset(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Version = 0;
    
    PreviewViewportProperties = ObjectInitializer.CreateDefaultSubobject<UDungeonEditorViewportProperties>( this, "PreviewProperties");
    MarkerGenerationModel = ObjectInitializer.CreateDefaultSubobject<UMarkerGenModel>(this, "MarkerGenerationModel");
}

void UDungeonThemeAsset::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector) {
    UDungeonThemeAsset* This = CastChecked<UDungeonThemeAsset>(InThis);

#if WITH_EDITORONLY_DATA
    Collector.AddReferencedObject(This->UpdateGraph, This);
#endif	// WITH_EDITORONLY_DATA

    Super::AddReferencedObjects(This, Collector);
}

bool UDungeonThemeAsset::IsLatestVersion() const {
    return Version == static_cast<int32>(EDungeonThemeAssetVersion::LatestVersion);
}

void UDungeonThemeAsset::Serialize(FArchive& Ar) {
#if WITH_EDITOR
    // Upgrade the asset before cooking
    if (Ar.IsCooking() && Ar.IsSaving() && !IsLatestVersion()) {
        FDungeonThemeCompiledGraphUpgrader::Upgrade(this);
    }
#endif

    UObject::Serialize(Ar);
}

