//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SCompoundWidget.h"

class ADungeon;
class UDungeonThemeAsset;

class SDungeonThemeEdModeToolbar : public SCompoundWidget {
public:
	DECLARE_DELEGATE_OneParam(FOnDungeonSelected, ADungeon*);
	DECLARE_DELEGATE_TwoParams(FOnThemeSelected, ADungeon*, UDungeonThemeAsset*);
	DECLARE_DELEGATE_OneParam(FOnAutoLayoutStateChange, bool);
	
	SLATE_BEGIN_ARGS(SDungeonThemeEdModeToolbar) {}
		SLATE_EVENT(FOnDungeonSelected, OnDungeonSelected)
		SLATE_EVENT(FOnThemeSelected, OnThemeSelected)
		SLATE_EVENT(FSimpleDelegate, OnDungeonBuildClicked)
		SLATE_EVENT(FSimpleDelegate, OnPerformLayoutClicked)
		SLATE_EVENT(FOnAutoLayoutStateChange, OnAutoLayoutStateChange)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	
	void SetOnDungeonSelected(const FOnDungeonSelected& InDelegate) { OnDungeonSelected = InDelegate; }
	void SetOnThemeSelected(const FOnThemeSelected& InDelegate) { OnThemeSelected = InDelegate; }

private:
	TSharedPtr<SComboBox<TWeakObjectPtr<ADungeon>>> DungeonComboBox;
	TSharedPtr<SComboBox<TWeakObjectPtr<UDungeonThemeAsset>>> ThemeComboBox;
	TSharedPtr<STextBlock> ThemeHintText;

	TArray<TWeakObjectPtr<ADungeon>> DungeonOptions;
	TArray<TWeakObjectPtr<UDungeonThemeAsset>> ThemeOptions;

	TWeakObjectPtr<ADungeon> CurrentDungeon;

	FOnDungeonSelected OnDungeonSelected;
	FOnThemeSelected OnThemeSelected;
	FSimpleDelegate OnDungeonBuildClicked;
	FOnAutoLayoutStateChange OnAutoLayoutStateChange;
	FSimpleDelegate OnPerformLayoutClicked;

private:
	void RefreshDungeonList();
	void RefreshThemeList();
	FReply OnBuildDungeonClicked() const;
	ECheckBoxState GetAutoArrangeCheckboxState() const;
	void OnAutoArrangeCheckboxStateChanged(ECheckBoxState CheckBoxState);
	FReply HandlePerformLayoutClicked() const;


	TSharedRef<SWidget> GenerateDungeonComboItem(TWeakObjectPtr<ADungeon> InDungeon);
	TSharedRef<SWidget> GenerateThemeComboItem(TWeakObjectPtr<UDungeonThemeAsset> InTheme);
	void OnDungeonComboBoxOpening();

	void OnDungeonSelectionChanged(TWeakObjectPtr<ADungeon> NewSelection, ESelectInfo::Type SelectInfo);
	void OnThemeSelectionChanged(TWeakObjectPtr<UDungeonThemeAsset> NewSelection, ESelectInfo::Type SelectInfo);
	void OnThemeComboBoxOpening();

	FText GetDungeonComboText() const;
	FText GetThemeComboText() const;
	EVisibility GetThemeComboVisibility() const;
	FSlateColor GetThemeHintTextColor() const;
	
	bool bAutoLayoutGraph = true;
};

