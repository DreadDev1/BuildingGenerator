//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphSchemaActions.h"

#include "Core/Common/DungeonArchitectEditorLog.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorCommon.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraph.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphNode.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraph.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FDungeonForgeEditorGraphSchemaAction"


UEdGraphNode* FDungeonForgeEditorGraphSchemaAction_NewNativeElement::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	
	UDungeonForgeEditorGraph* EditorGraph = Cast<UDungeonForgeEditorGraph>(ParentGraph);
	if (!EditorGraph)
	{
		UE_LOG(LogDungeonForgeEdMode, Error, TEXT("Invalid EditorGraph"));
		return nullptr;
	}

	UDungeonForgeGraph* ForgeGraph = EditorGraph->GetForgeGraph();
	if (!ForgeGraph)
	{
		UE_LOG(LogDungeonForgeEdMode, Error, TEXT("Invalid ForgeGraph"));
		return nullptr;
	}

	// Important - do not reconstruct the editor graph node/pins midway through this function as this will invalidate FromPin.
	const FDungeonForgeDeferNodeReconstructScope DisableReconstruct(FromPin);

	const FScopedTransaction Transaction(*FDungeonForgeEditorCommon::ContextIdentifier, LOCTEXT("ForgeEditorNewNativeElement", "Forge Editor: New Native Element"), nullptr);
	EditorGraph->Modify();

	UDungeonForgeNodeSettings* DefaultNodeSettings = nullptr;
	UDungeonForgeGraphNode* NewForgeNode = ForgeGraph->AddNodeOfType(SettingsClass, DefaultNodeSettings);

	if (!NewForgeNode)
	{
		UE_LOG(LogDungeonForgeEdMode, Error, TEXT("Failed to add a node of type %s"), *SettingsClass->GetName());
		return nullptr;
	}

	if (DefaultNodeSettings)
	{
		DefaultNodeSettings->ApplyPreconfiguredSettings(PreconfiguredInfo);
		NewForgeNode->UpdateAfterSettingsChangeDuringCreation();
	}

	PostCreation(NewForgeNode);

	FGraphNodeCreator<UDungeonForgeEditorGraphNode> NodeCreator(*EditorGraph);
	UDungeonForgeEditorGraphNode* NewNode = NodeCreator.CreateUserInvokedNode(bSelectNewNode);
	NewNode->Construct(NewForgeNode);
	NewNode->NodePosX = Location.X;
	NewNode->NodePosY = Location.Y;
	NodeCreator.Finalize();

	NewForgeNode->PositionX = Location.X;
	NewForgeNode->PositionY = Location.Y;

	if (FromPin && ensure(FromPin->GetOwningNode()))
	{
		NewNode->AutowireNewNode(FromPin);
	}

	return NewNode;
}

/*
void FDungeonForgeEditorGraphSchemaAction_NewGetParameterElement::PostCreation(UDungeonForgeGraphNode* NewNode)
{
	check(NewNode);
	UDungeonForgeUserParameterGetSettings* Settings = CastChecked<UDungeonForgeUserParameterGetSettings>(NewNode->GetSettings());

	Settings->PropertyGuid = PropertyGuid;
	Settings->PropertyName = PropertyName;

	// We need to set the settings to update the pins.
	NewNode->SetSettingsInterface(Settings);
}
*/


UEdGraphNode* FDungeonForgeEditorGraphSchemaAction_NewSettingsAsset::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	UDungeonForgeEditorGraph* EditorGraph = Cast<UDungeonForgeEditorGraph>(ParentGraph);
	if (!EditorGraph)
	{
		UE_LOG(LogDungeonForgeEdMode, Error, TEXT("Invalid EditorGraph"));
		return nullptr;
	}

	UDungeonForgeGraph* ForgeGraph = EditorGraph->GetForgeGraph();
	if (!ForgeGraph)
	{
		UE_LOG(LogDungeonForgeEdMode, Error, TEXT("Invalid ForgeGraph"));
		return nullptr;
	}

	UDungeonForgeNodeSettings* Settings = CastChecked<UDungeonForgeNodeSettings>(SettingsObjectPath.TryLoad());
	if (!Settings)
	{
		UE_LOG(LogDungeonForgeEdMode, Error, TEXT("Invalid settings"));
		return nullptr;
	}

	// Important - do not reconstruct the editor graph node/pins midway through this function as this will invalidate FromPin.
	const FDungeonForgeDeferNodeReconstructScope DisableReconstruct(FromPin);

	return MakeSettingsNode(EditorGraph, FromPin, Settings, Location, bSelectNewNode);
}

void FDungeonForgeEditorGraphSchemaAction_NewSettingsAsset::MakeSettingsNodesOrContextualMenu(const TSharedRef<class SWidget>& InPanel, FVector2D InScreenPosition, UEdGraph* InGraph, const TArray<FSoftObjectPath>& InSettingsPaths, const TArray<FVector2D>& InLocations, bool bInSelectNewNodes)
{
	UDungeonForgeEditorGraph* EditorGraph = Cast<UDungeonForgeEditorGraph>(InGraph);

	if (!EditorGraph)
	{
		return;
	}

	TArray<UDungeonForgeNodeSettings*> Settings;
	TArray<FVector2D> SettingsLocations;
	check(InSettingsPaths.Num() == InLocations.Num());

	for(int32 PathIndex = 0; PathIndex < InSettingsPaths.Num(); ++PathIndex)
	{
		const FSoftObjectPath& SettingsPath = InSettingsPaths[PathIndex];
		if (UDungeonForgeNodeSettings* LoadedSettings = Cast<UDungeonForgeNodeSettings>(SettingsPath.TryLoad()))
		{
			Settings.Add(LoadedSettings);
			SettingsLocations.Add(InLocations[PathIndex]);
		}
	}

	FModifierKeysState ModifierKeys = FSlateApplication::Get().GetModifierKeys();
	const bool bModifiedKeysActive = ModifierKeys.IsControlDown() || ModifierKeys.IsAltDown();

	if (bModifiedKeysActive)
	{
		MakeSettingsNodes(EditorGraph, Settings, SettingsLocations, bInSelectNewNodes);
	}
	else if(!Settings.IsEmpty())
	{
		FMenuBuilder MenuBuilder(true, nullptr);
		const FText SettingsTextName = ((Settings.Num() == 1) ? FText::FromName(Settings[0]->GetFName()) : LOCTEXT("ManySettings", "Settings"));

		MenuBuilder.BeginSection("SettingsDroppedOn", SettingsTextName);

		MenuBuilder.AddMenuEntry(
			FText::Format(LOCTEXT("CopySettings", "Copy {0}"), SettingsTextName),
			FText::Format(LOCTEXT("CopySettingsToolTip", "Copy the settings asset {0}, and keeps no reference to the original\n(Ctrl-drop to automatically create a copy)"), SettingsTextName),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&FDungeonForgeEditorGraphSchemaAction_NewSettingsAsset::MakeSettingsNodes, EditorGraph, Settings, SettingsLocations, bInSelectNewNodes), 
				FCanExecuteAction()
			)
		);

		TSharedRef<SWidget> PanelWidget = InPanel;
		// Show dialog to choose getter vs setter
		FSlateApplication::Get().PushMenu(
			PanelWidget,
			FWidgetPath(),
			MenuBuilder.MakeWidget(),
			InScreenPosition,
			FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
		);

		MenuBuilder.EndSection();
	}
}

void FDungeonForgeEditorGraphSchemaAction_NewSettingsAsset::MakeSettingsNodes(UDungeonForgeEditorGraph* InEditorGraph, TArray<UDungeonForgeNodeSettings*> InSettings, TArray<FVector2D> InSettingsLocations, bool bInSelectNewNodes)
{
	check(InSettings.Num() == InSettingsLocations.Num());
	for (int32 SettingsIndex = 0; SettingsIndex < InSettings.Num(); ++SettingsIndex)
	{
		MakeSettingsNode(InEditorGraph, nullptr, InSettings[SettingsIndex], InSettingsLocations[SettingsIndex], bInSelectNewNodes);
	}
}

UEdGraphNode* FDungeonForgeEditorGraphSchemaAction_NewSettingsAsset::MakeSettingsNode(UDungeonForgeEditorGraph* InEditorGraph, UEdGraphPin* InFromPin, UDungeonForgeNodeSettings* InSettings, FVector2D InLocation, bool bInSelectNewNode)
{
	if (!InEditorGraph || !InSettings)
	{
		return nullptr;
	}

	UDungeonForgeGraph* ForgeGraph = InEditorGraph->GetForgeGraph();
	if (!ForgeGraph)
	{
		return nullptr;
	}

	// Important - do not reconstruct the editor graph node/pins midway through this function as this will invalidate InFromPin.
	const FDungeonForgeDeferNodeReconstructScope DisableReconstruct(InFromPin);

	const FScopedTransaction Transaction(*FDungeonForgeEditorCommon::ContextIdentifier, LOCTEXT("DungeonForgeEditorNewSettingsElement", "Forge Editor: New Settings Element"), nullptr);
	InEditorGraph->Modify();

	UDungeonForgeNodeSettings* NewSettings = nullptr;
	UDungeonForgeGraphNode* NewForgeNode = ForgeGraph->AddNodeCopy(InSettings, NewSettings);

	if (!NewForgeNode)
	{
		UE_LOG(LogDungeonForgeEdMode, Error, TEXT("Unable to create node"));
		return nullptr;
	}

	FGraphNodeCreator<UDungeonForgeEditorGraphNode> NodeCreator(*InEditorGraph);
	UDungeonForgeEditorGraphNode* NewNode = NodeCreator.CreateUserInvokedNode(bInSelectNewNode);
	NewNode->Construct(NewForgeNode);
	NewNode->NodePosX = InLocation.X;
	NewNode->NodePosY = InLocation.Y;
	NodeCreator.Finalize();

	NewForgeNode->PositionX = InLocation.X;
	NewForgeNode->PositionY = InLocation.Y;

	if (InFromPin)
	{
		NewNode->AutowireNewNode(InFromPin);
	}

	return NewNode;
}

#undef LOCTEXT_NAMESPACE

