//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DungeonToolsEditorModeSettings.generated.h"

UCLASS(config=Editor)
class UDungeonToolsEditorModeSettings : public UDeveloperSettings {
	GENERATED_BODY()
public:
	
	// UDeveloperSettings overrides
	virtual FName GetContainerName() const override { return FName("Project"); }
	virtual FName GetCategoryName() const override { return FName("Plugins"); }
	virtual FName GetSectionName() const override { return FName("DungeonArchitect"); }

	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;

	DECLARE_MULTICAST_DELEGATE_TwoParams(UDungeonToolsEditorModeSettingsModified, UObject*, FProperty*);
	UDungeonToolsEditorModeSettingsModified OnModified;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		OnModified.Broadcast(this, PropertyChangedEvent.Property);

		Super::PostEditChangeProperty(PropertyChangedEvent);
	}

public:
	//
	// User Interface
	//

	/** If true, the standard UE Editor Gizmo Mode (ie selected via the Level Editor Viewport toggle) will be used to configure the Modeling Gizmo, otherwise a Combined Gizmo will always be used. It may be necessary to exit and re-enter Modeling Mode after changing this setting. */
	UPROPERTY(config, EditAnywhere, Category = "Modeling Mode|User Interface")
	bool bRespectLevelEditorGizmoMode = false;
};



/**
 * Defines a color to be used for a particular Tool Palette Section
 */
USTRUCT()
struct FDungeonModeCustomSectionColor
{
	GENERATED_BODY()

	/** Name of Section in Modeling Mode Tool Palette */
	UPROPERTY(EditAnywhere, Category = "SectionColor")
	FString SectionName = TEXT("");

	/** Custom Header Color */
	UPROPERTY(EditAnywhere, Category = "SectionColor")
	FLinearColor Color = FLinearColor::Gray;
};


/**
 * Defines a color to be used for a particular Tool Palette Tool
 */
USTRUCT()
struct FDungeonModeCustomToolColor
{
	GENERATED_BODY()

	/**
	 * Name of Section or Tool in Modeling Mode Tool Palette
	 *
	 * Format:
	 * SectionName        (Specifies a default color for all tools in the section.)
	 * SectionName.ToolName        (Specifies an override color for a specific tool in the given section.)
	 */
	UPROPERTY(EditAnywhere, Category = "ToolColor")
	FString ToolName = TEXT("");

	/** Custom Tool Color */
	UPROPERTY(EditAnywhere, Category = "ToolColor")
	FLinearColor Color = FLinearColor::Gray;
};


UCLASS(config=Editor)
class DUNGEONARCHITECTEDITOR_API UDungeonToolsModeCustomizationSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// UDeveloperSettings overrides

	virtual FName GetContainerName() const { return FName("Editor"); }
	virtual FName GetCategoryName() const { return FName("Plugins"); }
	virtual FName GetSectionName() const { return FName("DungeonArchitect"); }

	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;

public:

	/** Toggle between the Legacy Modeling Mode Palette and the new UI (requires exiting and re-entering the Mode) */
	UPROPERTY(config, EditAnywhere, Category = "Modeling Mode|UI Customization")
	bool bUseLegacyModelingPalette = false;

	/** Add the names of Modeling Mode Tool Palette Sections to have them appear at the top of the Tool Palette, in the order listed below. */
	UPROPERTY(config, EditAnywhere, Category = "Modeling Mode|UI Customization")
	TArray<FString> ToolSectionOrder;

	/** Tool Names listed in the array below will appear in a Favorites section at the top of the Modeling Mode Tool Palette */
	UE_DEPRECATED(5.3, "Modeling Mode favorites are now set through FEditablePalette or the Mode UI itself")
	TArray<FString> ToolFavorites;
	
	/** Custom Section Header Colors for listed Sections in the Modeling Mode Tool Palette */
	UPROPERTY(config, EditAnywhere, Category = "Modeling Mode|UI Customization")
	TArray<FDungeonModeCustomSectionColor> SectionColors;

	/**
	 * Custom Tool Colors for listed Tools in the Modeling Mode Tool Palette.
	 * 
	 * Format:
	 * SectionName        (Specifies a default color for all tools in the section.)
	 * SectionName.ToolName        (Specifies an override color for a specific tool in the given section.)
	 */
	UPROPERTY(config, EditAnywhere, Category = "Modeling Mode|UI Customization")
	TArray<FDungeonModeCustomToolColor> ToolColors;


	/**
	 * If true, the category labels will be shown on the toolbar buttons, else they will be hidden
	 */
	UPROPERTY(config, EditAnywhere, Category = "Modeling Mode|UI Customization")
	bool bShowCategoryButtonLabels = true;
	
	/**
	 * If true, Tool buttons will always be shown when in a Tool. By default they will be hidden.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Modeling Mode|UI Customization")
	bool bAlwaysShowToolButtons = false;

public:

	// saved-state for various mode settings that are configured via UI toggles/etc, and not exposed in settings dialog

	UPROPERTY(config)
	int32 LastMeshSelectionDragMode = 0;

public:

	// saved-state for various mode settings that does not persist between editor runs

	UPROPERTY()
	int32 LastMeshSelectionElementType = 0;

	UPROPERTY()
	int32 LastMeshSelectionTopologyMode = 0;

	UPROPERTY()
	bool bLastMeshSelectionVolumeToggle = true;

	UPROPERTY()
	bool bLastMeshSelectionStaticMeshToggle = true;


};

