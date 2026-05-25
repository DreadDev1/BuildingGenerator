//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Graph/Slate/SDungeonForgeEditorGraphNode.h"

#include "Core/LevelEditor/Customizations/DungeonArchitectStyle.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphNodeBase.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

void SDungeonForgeEditorGraphNode::Construct(const FArguments& InArgs, UDungeonForgeEditorGraphNodeBase* InNode) {
	GraphNode = InNode;
	ForgeEditorGraphNode = InNode;

	if (InNode) {
		InNode->OnNodeChangedDelegate.BindSP(this, &SDungeonForgeEditorGraphNode::OnNodeChanged);
	}

	UpdateGraphNode();
}

void SDungeonForgeEditorGraphNode::GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const {
	check(ForgeEditorGraphNode);
	
	FVector2D OverlayOffset(0.0, 0.0);
	
	auto AddOverlayBrush = [&OverlayOffset, &Brushes](const FName& BrushName)
	{
		const FSlateBrush* Brush = FDungeonArchitectStyle::Get().GetBrush(BrushName);

		if (Brush)
		{
			FOverlayBrushInfo BrushInfo;
			BrushInfo.Brush = Brush;
			BrushInfo.OverlayOffset = OverlayOffset - Brush->GetImageSize() / 2.0;
			Brushes.Add(BrushInfo);

			OverlayOffset.Y += Brush->GetImageSize().Y;
		}
	};
	
	if (const UDungeonForgeGraphNode* ForgeNode = ForgeEditorGraphNode->GetForgeNode()) {
		if (ForgeNode->GetSettingsInterface() && ForgeNode->GetSettingsInterface()->bDebug) {
			AddOverlayBrush(TEXT("Forge.NodeOverlay.Debug"));
		}
	}
}

void SDungeonForgeEditorGraphNode::OnNodeChanged() {
	UpdateGraphNode();
}

