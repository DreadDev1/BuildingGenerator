//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"

#include "EdGraph/EdGraphSchema.h"
#include "Templates/SubclassOf.h"
#include "DungeonForgeEditorGraphSchemaActions.generated.h"

class UDungeonForgeGraphNode;
class UDungeonForgeNodeSettings;
class UDungeonForgeEditorGraph;


USTRUCT()
struct FDungeonForgeEditorGraphSchemaAction_NewNativeElement : public FEdGraphSchemaAction
{
	GENERATED_BODY()

	// Inherit the base class's constructors
	using FEdGraphSchemaAction::FEdGraphSchemaAction;

	// Simple type info
	static FName StaticGetTypeId()
	{
		static FName Type("FDungeonForgeEditorGraphSchemaAction_NewNativeElement");
		return Type;
	}
	
	UPROPERTY()
	TSubclassOf<UDungeonForgeNodeSettings> SettingsClass;

	UPROPERTY()
	FDungeonForgePreConfiguredSettingsInfo PreconfiguredInfo;

	//~ Begin FEdGraphSchemaAction Interface
	virtual FName GetTypeId() const override { return StaticGetTypeId(); }
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	//~ End FEdGraphSchemaAction Interface

protected:
	virtual void PostCreation(UDungeonForgeGraphNode* NewNode) {}
};

/*
USTRUCT()
struct FDungeonForgeEditorGraphSchemaAction_NewGetParameterElement : public FDungeonForgeEditorGraphSchemaAction_NewNativeElement
{
	GENERATED_BODY()

	// Inherit the base class's constructors
	using FDungeonForgeEditorGraphSchemaAction_NewNativeElement::FDungeonForgeEditorGraphSchemaAction_NewNativeElement;

	UPROPERTY()
	FGuid PropertyGuid;

	UPROPERTY()
	FName PropertyName;

protected:
	virtual void PostCreation(UDungeonForgeGraphNode* NewNode) override;
};
*/

USTRUCT()
struct FDungeonForgeEditorGraphSchemaAction_NewSettingsAsset : public FEdGraphSchemaAction
{
	GENERATED_BODY()

	// Inherit the base class's constructors
	using FEdGraphSchemaAction::FEdGraphSchemaAction;
	
	// Simple type info
	static FName StaticGetTypeId()
	{
		static FName Type("FDungeonForgeEditorGraphSchemaAction_NewNode");
		return Type;
	}

	UPROPERTY()
	FSoftObjectPath SettingsObjectPath;

	//~ Begin FEdGraphSchemaAction Interface
	virtual FName GetTypeId() const override { return StaticGetTypeId(); }
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
	//~ End FEdGraphSchemaAction Interface

	static void MakeSettingsNodesOrContextualMenu(const TSharedRef<class SWidget>& InPanel, FVector2D InScreenPosition, UEdGraph* InGraph, const TArray<FSoftObjectPath>& InSettingsPaths, const TArray<FVector2D>& InLocations, bool bInSelectNewNodes);
	static void MakeSettingsNodes(UDungeonForgeEditorGraph* InEditorGraph, TArray<UDungeonForgeNodeSettings*> InSettings, TArray<FVector2D> InSettingsLocations, bool bInSelectNewNodes);
	static UEdGraphNode* MakeSettingsNode(UDungeonForgeEditorGraph* InEditorGraph, UEdGraphPin* InFromPin, UDungeonForgeNodeSettings* InSettings, FVector2D InLocation, bool bInSelectNewNode);
};

