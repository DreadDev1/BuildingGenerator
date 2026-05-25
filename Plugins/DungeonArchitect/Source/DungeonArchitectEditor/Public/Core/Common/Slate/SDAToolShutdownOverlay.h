//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UEdMode;
enum class EToolShutdownType : uint8;

class SDAToolShutdownOverlay : public SCompoundWidget {
public:
	SLATE_BEGIN_ARGS(SDAToolShutdownOverlay) {}
		SLATE_ATTRIBUTE(FText, DisplayText)
		SLATE_ATTRIBUTE(const FSlateBrush*, IconImage)
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs, const TWeakObjectPtr<UEdMode>& InEdMode);

private:
	FReply EndTool(EToolShutdownType EndType) const;
	bool CanAcceptActiveTool() const;
	bool CanCancelActiveTool() const;
	bool CanCompleteActiveTool() const;
	bool ActiveToolHasAccept() const;
	
private:
	TWeakObjectPtr<UEdMode> EdModePtr;
	FText DisplayText;
	const FSlateBrush* IconImage{};
}; 

