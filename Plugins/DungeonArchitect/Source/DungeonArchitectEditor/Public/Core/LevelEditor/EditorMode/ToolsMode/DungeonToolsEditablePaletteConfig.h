//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Containers/Map.h"
#include "EditorConfigBase.h"
#include "ToolkitBuilderConfig.h"
#include "UObject/ObjectPtr.h"
#include "DungeonToolsEditablePaletteConfig.generated.h"

UCLASS(EditorConfig="EditableToolPalette")
class UDungeonToolsEditablePaletteConfig : public UEditorConfigBase, public IEditableToolPaletteConfigManager
{
	GENERATED_BODY()
	
public:

	// IEditableToolPaletteConfigManager Interface
	virtual FEditableToolPaletteSettings* GetMutablePaletteConfig(const FName& InstanceName) override;
	virtual const FEditableToolPaletteSettings* GetConstPaletteConfig(const FName& InstanceName) override;
	virtual void SavePaletteConfig(const FName& InstanceName) override;

	static void Initialize();
	static UDungeonToolsEditablePaletteConfig* Get() { return Instance; }
	static IEditableToolPaletteConfigManager* GetAsConfigManager() { return Instance; }

	UPROPERTY(meta=(EditorConfig))
	TMap<FName, FEditableToolPaletteSettings> EditableToolPalettes;

private:
	static TObjectPtr<UDungeonToolsEditablePaletteConfig> Instance;
};

