//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphNode.h"

#include "Core/Common/DungeonArchitectEditorLog.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

#include "Framework/Commands/GenericCommands.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"

#define LOCTEXT_NAMESPACE "ForgeEditorGraphNode"

UDungeonForgeEditorGraphNode::UDungeonForgeEditorGraphNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanRenameNode = true;
}

FText UDungeonForgeEditorGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (!ForgeNode)
	{
		return NSLOCTEXT("ForgeEditorGraphNode", "UnnamedNodeTitle", "Unnamed Node");
	}

	if (TitleType == ENodeTitleType::FullTitle)
	{
		return ForgeNode->GetNodeTitle(EDungeonForgeNodeTitleType::FullTitle);
	}
	else
	{
		return ForgeNode->GetNodeTitle(EDungeonForgeNodeTitleType::ListView);
	}
}

void UDungeonForgeEditorGraphNode::GetNodeContextMenuActions(UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	if (!Context->Node)
	{
		return;
	}

	FToolMenuSection& Section = Menu->AddSection("EdGraphSchemaGeneral", LOCTEXT("GeneralHeader", "General"));
	Section.AddMenuEntry(FGenericCommands::Get().Delete);
	Section.AddMenuEntry(FGenericCommands::Get().Cut);
	Section.AddMenuEntry(FGenericCommands::Get().Copy);
	Section.AddMenuEntry(FGenericCommands::Get().Duplicate);

	Super::GetNodeContextMenuActions(Menu, Context);
}

void UDungeonForgeEditorGraphNode::AllocateDefaultPins()
{
	if (ForgeNode)
	{
		CreatePins(ForgeNode->GetInputPins(), ForgeNode->GetOutputPins());
	}
}

void UDungeonForgeEditorGraphNode::OnRenameNode(const FString& NewName)
{
	if (!GetCanRenameNode())
	{
		return;
	}

	if(NewName.Len() >= NAME_SIZE)
	{
		UE_LOG(LogDungeonForgeEdMode, Error, TEXT("New name for Forge node is too long."));
		return;
	}

	if (!ForgeNode)
	{
		return;
	}

	if (ForgeNode->GetAuthoredTitleLine().ToString() != NewName)
	{
		Modify();
		ForgeNode->Modify();
		ForgeNode->NodeTitle = FName(*NewName);
	}
}

#undef LOCTEXT_NAMESPACE

