//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Customizations/DungeonArchitectContentBrowserExtensions.h"

#include "Frameworks/MiniMap/LevelMinimapGenerator.h"

#include "ContentBrowserDelegates.h"
#include "ContentBrowserModule.h"
#include "Engine/World.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

DEFINE_LOG_CATEGORY_STATIC(LogDAContentBrowserEx, Log, All);
#define LOCTEXT_NAMESPACE "DungeonArchitectContentBrowserExtensions"

class IDAContentBrowserMenuAction {
public:
	virtual ~IDAContentBrowserMenuAction() = default;
	virtual bool IsAssetSupported(const FAssetData& InAssetData) const = 0;
	virtual void AddActionEntries(FMenuBuilder& MenuBuilder, TArray<FAssetData> AssetDataList) const = 0;
};
typedef TSharedPtr<IDAContentBrowserMenuAction> IDAContentBrowserMenuActionPtr;

namespace DAContentBrowserMenuActions {
	static FContentBrowserMenuExtender_SelectedAssets ContentBrowserExtenderDelegate;
	static FDelegateHandle ContentBrowserExtenderDelegateHandle;
	static TArray<IDAContentBrowserMenuActionPtr> Actions;
}


class FDungeonArchitectContentBrowserExtensions_Impl {
public:
	static void CreateSubMenuEntries(FMenuBuilder& MenuBuilder, TMap<IDAContentBrowserMenuActionPtr, TArray<FAssetData>> SupportedActions) {
		for (const auto& Entry : SupportedActions) {
			IDAContentBrowserMenuActionPtr Action = Entry.Key;
			const TArray<FAssetData>& SupportedAssets = Entry.Value;
			if (Action.IsValid()) {
				Action->AddActionEntries(MenuBuilder, SupportedAssets);
			}
		}
	}

	static void CreateActionsSubMenu(FMenuBuilder& MenuBuilder, TMap<IDAContentBrowserMenuActionPtr, TArray<FAssetData>> SupportedActions) {
		const FSlateIcon SubMenuIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details");
		MenuBuilder.AddSubMenu(LOCTEXT("DungeonArchitectTitle", "Dungeon Architect"),
							   LOCTEXT("DungeonArchitectActionsToolTip", "Dungeon Architect Actions"),
							   FNewMenuDelegate::CreateStatic(&FDungeonArchitectContentBrowserExtensions_Impl::CreateSubMenuEntries, MoveTemp(SupportedActions)),
							   false, SubMenuIcon
		);
	}

	static TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets) {
		TSharedRef<FExtender> Extender(new FExtender());

		TMap<IDAContentBrowserMenuActionPtr, TArray<FAssetData>> SupportedActions;
		for (const FAssetData& SelectedAsset : SelectedAssets) {
			for (const IDAContentBrowserMenuActionPtr& Action : DAContentBrowserMenuActions::Actions) {
				if (Action.IsValid() && Action->IsAssetSupported(SelectedAsset)) {
					TArray<FAssetData>& SupportedAssets = SupportedActions.FindOrAdd(Action);
					SupportedAssets.Add(SelectedAsset);
				}
			}
		}
		
		if (SupportedActions.Num() > 0) {
			// Add asset actions extender
			Extender->AddMenuExtension(
				"CommonAssetActions",
				EExtensionHook::After,
				nullptr,
				FMenuExtensionDelegate::CreateStatic(&FDungeonArchitectContentBrowserExtensions_Impl::CreateActionsSubMenu, MoveTemp(SupportedActions)));
		}

		
		return Extender;
	}

	static TArray<FContentBrowserMenuExtender_SelectedAssets>& GetExtenderDelegates()
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
		return ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	}

	static void InstallHooks() {
		DAContentBrowserMenuActions::ContentBrowserExtenderDelegate = FContentBrowserMenuExtender_SelectedAssets::CreateStatic(&FDungeonArchitectContentBrowserExtensions_Impl::OnExtendContentBrowserAssetSelectionMenu);

		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = GetExtenderDelegates();
		CBMenuExtenderDelegates.Add(DAContentBrowserMenuActions::ContentBrowserExtenderDelegate);
		DAContentBrowserMenuActions::ContentBrowserExtenderDelegateHandle = CBMenuExtenderDelegates.Last().GetHandle();
	}

	static void RemoveHooks() {
		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = GetExtenderDelegates();
		CBMenuExtenderDelegates.RemoveAll([](const FContentBrowserMenuExtender_SelectedAssets& Delegate){ return Delegate.GetHandle() == DAContentBrowserMenuActions::ContentBrowserExtenderDelegateHandle; });

	}
};

//////////////////////////////////////// Action Implementations ////////////////////////////////////////
class FDAContentBrowserMenuAction_GenerateLevelMinimap : public IDAContentBrowserMenuAction {
public:
	virtual bool IsAssetSupported(const FAssetData& InAssetData) const override {
		return InAssetData.IsInstanceOf(UWorld::StaticClass());
	}

	static void GenerateMinimap(TArray<FAssetData> AssetDataList) {
		UE_LOG(LogDAContentBrowserEx, Log, TEXT("Generating level minimap mesh"));

		for (const FAssetData& AssetData : AssetDataList) {
			if (AssetData.IsInstanceOf(UWorld::StaticClass(), EResolveClass::Yes)) {
				UWorld* LoadedWorld = Cast<UWorld>(AssetData.GetAsset());
				ULevelMinimapGenerator::GenerateMinimapMeshFromWorld(LoadedWorld);
			} 
		} 
	}
	
	virtual void AddActionEntries(FMenuBuilder& MenuBuilder, TArray<FAssetData> AssetDataList) const override {
		const FSlateIcon IconGeneric = FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details");
		const FUIAction Action(FExecuteAction::CreateStatic(&FDAContentBrowserMenuAction_GenerateLevelMinimap::GenerateMinimap, AssetDataList), FCanExecuteAction());
		MenuBuilder.AddMenuEntry(LOCTEXT("GenMiniMapMeshTitle", "Generate Level Minimap Mesh"),
			LOCTEXT("GenMiniMapMeshTooltip", "Create a simplified mesh representing the level, so it can be used in snap based dungeon minimaps"), IconGeneric, Action);
	}
};

//////////////////////////////////////// Hook Registration ////////////////////////////////////////
void FDungeonArchitectContentBrowserExtensions::InstallHooks() {
	FDungeonArchitectContentBrowserExtensions_Impl::InstallHooks();
	DAContentBrowserMenuActions::Actions.Reset();
	
	// Add actions here
	//DAContentBrowserMenuActions::Actions.Add(MakeShareable(new FDAContentBrowserMenuAction_GenerateLevelMinimap));
}

void FDungeonArchitectContentBrowserExtensions::RemoveHooks() {
	FDungeonArchitectContentBrowserExtensions_Impl::RemoveHooks();
	DAContentBrowserMenuActions::Actions.Reset();
}

#undef LOCTEXT_NAMESPACE

