//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/Data/DungeonForgeData.h"
#include "Frameworks/Forge/DungeonForgeCommon.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphPin.h"
#include "DungeonForgeNodeSettings.generated.h"

class UDungeonForgeGraphPin;
class UDungeonForgeGraphNode;
class UDungeonForgeNodeSettings;
class ULineBatchComponent;
typedef TSharedPtr<class IDungeonForgeElement, ESPMode::ThreadSafe> FDungeonForgeElementPtr;

UENUM()
enum class EDungeonForgeSettingsType : uint8
{
	WorldLayout,
	DungeonLayout,
	InputOutput,
	Spatial,
	Literal,
	Density,
	Blueprint,
	Metadata,
	Filter,
	Sampler,
	Spawner,
	Subgraph,
	Debug,
	Generic,
	Param,
	HierarchicalGeneration,
	ControlFlow,
	PointOps,
	GraphParameters,
	Reroute,
	GPU,
	DynamicMesh,
};



#if WITH_EDITOR
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDungeonForgeSettingsChanged, UDungeonForgeNodeSettings*, EDungeonForgeGraphChangeType);
#endif

/**
* Pre-configured settings info
* Will be passed to the settings to pre-configure the settings on creation.
* Example: Maths operations: Add, Mul, etc...
*/
USTRUCT(BlueprintType)
struct DUNGEONARCHITECTRUNTIME_API FDungeonForgePreConfiguredSettingsInfo
{
	GENERATED_BODY()

	FDungeonForgePreConfiguredSettingsInfo() = default;

	explicit FDungeonForgePreConfiguredSettingsInfo(int32 InIndex, FText InLabel = FText{})
		: PreconfiguredIndex(InIndex)
		, Label(std::move(InLabel))
	{}

#if WITH_EDITOR
	FDungeonForgePreConfiguredSettingsInfo(int32 InIndex, FText InLabel, FText InTooltip)
		: PreconfiguredIndex(InIndex)
		, Label(std::move(InLabel))
		, Tooltip(std::move(InTooltip))
	{}
#endif // WITH_EDITOR

	/* Index used by the settings to know which preconfigured settings it needs to set. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "")
	int32 PreconfiguredIndex = -1;

	/* Label for the exposed asset. Can also be used instead of the index, if it is easier to deal with strings. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "")
	FText Label;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "")
	FText Tooltip;
#endif // WITH_EDITORONLY_DATA
};

UCLASS(Abstract)
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeNodeSettingsInterface : public UDungeonForgeData {
	GENERATED_BODY()
public:
	virtual UDungeonForgeNodeSettings* GetSettings() PURE_VIRTUAL(UDungeonForgeNodeSettingsInterface::GetSettings, return nullptr;);
	virtual const UDungeonForgeNodeSettings* GetSettings() const PURE_VIRTUAL(UDungeonForgeNodeSettingsInterface::GetSettings, return nullptr;);

	/** Dedicated method to change enable state because some nodes have more complex behavior on enable/disable (such as subgraphs) */
	void SetEnabled(bool bInEnabled);

	/** Whether this element can be disabled. */
	virtual bool CanBeDisabled() const { return true; }

	/** Whether this element supports Debug and Inspect features. */
	virtual bool CanBeDebugged() const { return true; }
	
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const { return NAME_None; }
	virtual FText GetDefaultNodeTitle() const { return FText::FromName(GetDefaultNodeName()); }
	
	virtual FText GetNodeTooltipText() const { return FText::GetEmpty(); }
	virtual FLinearColor GetNodeTitleColor() const { return FLinearColor::White; }
#endif // WITH_EDITOR
	
#if WITH_EDITOR
	FOnDungeonForgeSettingsChanged OnSettingsChangedDelegate;
#endif
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Debug, meta = (EditCondition = bDisplayDebuggingProperties, EditConditionHides, HideEditConditionToggle))
	bool bEnabled = true;

	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category = Debug, meta = (EditCondition = bDisplayDebuggingProperties, EditConditionHides, HideEditConditionToggle))
	bool bDebug = false;
	
#if WITH_EDITORONLY_DATA
	// This can be set false by inheriting nodes to hide the debugging properties.
	UPROPERTY(Transient, meta = (EditCondition = false, EditConditionHides))
	bool bDisplayDebuggingProperties = true;
#endif
};

UCLASS(Abstract)
class DUNGEONARCHITECTRUNTIME_API UDungeonForgeNodeSettings : public UDungeonForgeNodeSettingsInterface {
	GENERATED_BODY()
public:
	// ~Begin UDungeonForgeNodeSettingsInterface interface
	virtual UDungeonForgeNodeSettings* GetSettings() { return this; }
	virtual const UDungeonForgeNodeSettings* GetSettings() const { return this; }
	// ~End UDungeonForgeNodeSettingsInterface interface
	
	virtual UDungeonForgeGraphNode* CreateNode() const;

	/** Return the concatenation of InputPinProperties and FillOverridableParamsPins */
	TArray<FDungeonForgePinProperties> AllInputPinProperties() const;

	/** For symmetry reason, do the same with output pins. For now forward just the call to OutputPinProperties */
	TArray<FDungeonForgePinProperties> AllOutputPinProperties() const;

	/** If the node has any dynamic pins that can change based on input or settings */
	virtual bool HasDynamicPins() const { return false; }

	virtual void ApplyPreconfiguredSettings(const FDungeonForgePreConfiguredSettingsInfo& PreconfigureInfo) {}

	/** Returns the current pin types, which can either be the static types from the pin properties, or a dynamic type based on connected edges.
	* By default we set output pin types to the union of the default input pin incident edge types, if it is dynamic and the default input exists.
	*/
	virtual EDungeonForgeDataType GetCurrentPinTypes(const UDungeonForgeGraphPin* InPin) const;

	
protected:
	// Returns an array of all the input pin properties. You should not add manually a "params" pin, it is handled automatically by FillOverridableParamsPins
	virtual TArray<FDungeonForgePinProperties> InputPinProperties() const;
	virtual TArray<FDungeonForgePinProperties> OutputPinProperties() const;

public:
	// Internal functions, should not be used by any user.
	// Return a different subset for input/output pin properties, in case of a default object.
	virtual TArray<FDungeonForgePinProperties> DefaultInputPinProperties() const;
	virtual TArray<FDungeonForgePinProperties> DefaultOutputPinProperties() const;
	virtual FDungeonForgeElementPtr CreateElement() const PURE_VIRTUAL(UPCGSettings::CreateElement, return nullptr;);
	
	/** By default, settings do not use a seed. Override this in the settings subclass to enable usage of the seed. UFUNCTION to be used by EditCondition. */
	UFUNCTION()
	virtual bool UseSeed() const { return false; }
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta=(EditCondition="UseSeed()", EditConditionHides, DungeonForge_Overridable))
	int Seed = 0xC35A9631; // Default seed is a random prime number, but will be overriden for new settings based on the class type name hash, making each settings class have a different default seed.

#if WITH_EDITORONLY_DATA
	UPROPERTY() //BlueprintReadWrite, EditAnywhere, Category = AssetInfo, AssetRegistrySearchable)
	bool bExposeToLibrary = true;

	UPROPERTY() //BlueprintReadWrite, EditAnywhere, Category = AssetInfo, AssetRegistrySearchable)
	FText Category;

	UPROPERTY() //BlueprintReadWrite, EditAnywhere, Category = AssetInfo, AssetRegistrySearchable)
	FText Description;
#endif


	
#if WITH_EDITOR
	/** Puts node title on node body, reducing overall node size */
	virtual bool ShouldDrawNodeCompact() const { return false; }
	/** Returns the icon to use instead of text in compact node form */
	virtual bool GetCompactNodeIcon(FName& OutCompactNodeIcon) const { return false; }
	/** Returns whether the user can directly interact with the node name */
	virtual bool CanUserEditTitle() const { return true; }
	
	virtual FName GetDefaultNodeName() const { return NAME_None; }
	virtual FText GetDefaultNodeTitle() const { return FText::FromName(GetDefaultNodeName()); }

	/** List of extra aliases that will be added to the node list in the Editor.Useful when we rename a node, but we still want the user to find the old one. */
	virtual TArray<FText> GetNodeTitleAliases() const { return {}; }

	virtual FText GetNodeTooltipText() const { return FText::GetEmpty(); }
	virtual FLinearColor GetNodeTitleColor() const { return FLinearColor::White; }
	virtual EDungeonForgeSettingsType GetType() const { return EDungeonForgeSettingsType::Generic; }

	/** Can override the label style for a pin. Return false if no override is available. */
	virtual bool GetPinLabelStyle(const UDungeonForgeGraphPin* InPin, FName& OutLabelStyle) const { return false; }

	/** Override this class to provide an UObject to jump to in case of double click on node
	 *  ie. returning a blueprint instance will open the given blueprint in its editor.
	 *  By default, it will return the underlying class, to try to jump to its header in code
     */
	virtual UObject* GetJumpTargetForDoubleClick() const;

	/** Return preconfigured info that will be filled in the editor palette action, allowing to create pre-configured settings */
	virtual TArray<FDungeonForgePreConfiguredSettingsInfo> GetPreconfiguredInfo() const { return {}; }

	/** If there are preconfigured info, we can skip the default settings and only expose pre-configured actions in the editor palette */
	virtual bool OnlyExposePreconfiguredSettings() const { return false; }

	/** If there are preconfigured info, decide if they are grouped in the palette in a folder with the node name, or if they are not grouped. */
	virtual bool GroupPreconfiguredSettings() const { return true; }

	/** Perform post-operations when an editor node is copied */
	virtual void PostPaste();
#endif // WITH_EDITOR
	
	/** Derived classes can implement this to expose additional information or context, such as an asset in use by the node. */
	virtual FString GetAdditionalTitleInformation() const { return FString(); }

	/** Display generated title line as primary title (example: BP nodes display the blueprint name as the primary title). */
	virtual bool HasFlippedTitleLines() const { return false; }

};


