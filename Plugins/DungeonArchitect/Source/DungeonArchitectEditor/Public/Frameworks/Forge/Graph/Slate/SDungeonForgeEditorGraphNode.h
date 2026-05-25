//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "SGraphNode.h"

class UDungeonForgeEditorGraphNodeBase;

class SDungeonForgeEditorGraphNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SDungeonForgeEditorGraphNode){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UDungeonForgeEditorGraphNodeBase* InNode);

	//~ Begin SNodePanel::SNode Interface
	virtual void GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const override;
	//~ End SNodePanel::SNode Interface

protected:
	void OnNodeChanged();
	
private:
	TObjectPtr<UDungeonForgeEditorGraphNodeBase> ForgeEditorGraphNode = nullptr;
};

