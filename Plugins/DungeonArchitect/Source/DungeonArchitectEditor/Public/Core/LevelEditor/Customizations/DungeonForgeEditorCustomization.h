//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"

class UDungeonForgeEditorMode;
class IDetailCategoryBuilder;

class FDungeonForgeEditorNodeCustomizationBase  : public IDetailCustomization {
protected:
	static void RegisterToolButton(IDetailLayoutBuilder& InDetailBuilder, const FText& InFilterText, const FText& InText, const TFunction<FReply()>& InCallback);
	static IDetailCategoryBuilder& GetToolsCategory(IDetailLayoutBuilder& DetailBuilder);
	static UDungeonForgeEditorMode* GetForgeEditorMode();
};

class FDungeonForgeEditorNodeCustomization : public FDungeonForgeEditorNodeCustomizationBase {
public:
	typedef FDungeonForgeEditorNodeCustomizationBase Super;
	
	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	// End of IDetailCustomization interface
	
	static TSharedRef<IDetailCustomization> MakeInstance();
};
