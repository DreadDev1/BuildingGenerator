//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphNodeBase.h"
#include "DungeonForgeEditorGraphNode.generated.h"

UCLASS()
class UDungeonForgeEditorGraphNode : public UDungeonForgeEditorGraphNodeBase {
	GENERATED_BODY()
public:
	UDungeonForgeEditorGraphNode(const FObjectInitializer& ObjectInitializer);
	
	// ~Begin UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	virtual void AllocateDefaultPins() override;
	virtual void OnRenameNode(const FString& NewName) override;
	// ~End UEdGraphNode interface
};


