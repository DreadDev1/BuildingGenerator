//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "EdGraphSchema_DungeonThemeExtensions.generated.h"

class FDungeonThemeSchemaExtensionProvider;
struct FEdGraphSchemaAction;
class UEdGraph;

UCLASS()
class DUNGEONARCHITECTEDITOR_API UEdGraphSchema_DungeonThemeExtensions : public UObject {
    GENERATED_BODY()

public:
    /** Returns the extension manager and creates it if missing */
    static UEdGraphSchema_DungeonThemeExtensions& Get();

    void AddExtension(TSharedPtr<FDungeonThemeSchemaExtensionProvider> Extension);

    void CreateCustomActions(TArray<TSharedPtr<FEdGraphSchemaAction>>& OutActions, const UEdGraph* Graph,
                             UEdGraph* OwnerOfTemporaries, bool bShowNewMesh, bool bShowNewMarker,
                             bool bShowMarkerEmitters);

private:
    static class UEdGraphSchema_DungeonThemeExtensions* SchemaExtensionSingleton;
    TArray<TSharedPtr<FDungeonThemeSchemaExtensionProvider>> Extensions;
};


class DUNGEONARCHITECTEDITOR_API FDungeonThemeSchemaExtensionProvider {
public:
    virtual ~FDungeonThemeSchemaExtensionProvider() {
    }

    virtual void CreateCustomActions(TArray<TSharedPtr<FEdGraphSchemaAction>>& OutActions, const UEdGraph* Graph,
                                     UEdGraph* OwnerOfTemporaries, bool bShowNewMesh, bool bShowNewMarker,
                                     bool bShowMarkerEmitters) = 0;
};

