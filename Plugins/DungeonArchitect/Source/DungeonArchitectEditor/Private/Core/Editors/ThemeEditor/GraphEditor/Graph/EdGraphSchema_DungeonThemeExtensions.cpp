//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphSchema_DungeonThemeExtensions.h"


UEdGraphSchema_DungeonThemeExtensions* UEdGraphSchema_DungeonThemeExtensions::SchemaExtensionSingleton = nullptr;

UEdGraphSchema_DungeonThemeExtensions& UEdGraphSchema_DungeonThemeExtensions::Get() {
    // Create it if we need to
    if (SchemaExtensionSingleton == nullptr) {
        SchemaExtensionSingleton = NewObject<UEdGraphSchema_DungeonThemeExtensions>();

        // Keep the singleton alive
        SchemaExtensionSingleton->AddToRoot();
    }

    return *SchemaExtensionSingleton;
}

void UEdGraphSchema_DungeonThemeExtensions::AddExtension(TSharedPtr<FDungeonThemeSchemaExtensionProvider> Extension) {
    Extensions.Add(Extension);
}

void UEdGraphSchema_DungeonThemeExtensions::CreateCustomActions(TArray<TSharedPtr<FEdGraphSchemaAction>>& OutActions,
                                                    const UEdGraph* Graph, UEdGraph* OwnerOfTemporaries,
                                                    bool bShowNewMesh, bool bShowNewMarker, bool bShowMarkerEmitters) {
    for (TSharedPtr<FDungeonThemeSchemaExtensionProvider> Extension : Extensions) {
        Extension->CreateCustomActions(OutActions, Graph, OwnerOfTemporaries, bShowNewMesh, bShowNewMarker,
                                       bShowMarkerEmitters);
    }
}

