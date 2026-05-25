//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ThemeMode/DungeonThemeEdModeToolbar.h"

#include "Core/Dungeon.h"
#include "Core/Settings/DungeonArchitectProjectUserSettings.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"

#define LOCTEXT_NAMESPACE "SDungeonThemeEdModeToolbar"

void SDungeonThemeEdModeToolbar::Construct(const FArguments& InArgs) {
    OnDungeonSelected = InArgs._OnDungeonSelected;
    OnThemeSelected = InArgs._OnThemeSelected;
    OnDungeonBuildClicked = InArgs._OnDungeonBuildClicked;
    OnAutoLayoutStateChange = InArgs._OnAutoLayoutStateChange;
    OnPerformLayoutClicked = InArgs._OnPerformLayoutClicked;
    
    RefreshDungeonList();

    ChildSlot
    [
        SNew(SBorder)
        .Padding(FMargin(8))
        .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
        [
            SNew(SHorizontalBox)
            
            // Dungeon Section
            + SHorizontalBox::Slot()
            .Padding(FMargin(0, 0, 16, 0))
            .AutoWidth()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(FMargin(0, 0, 4, 0))
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Dungeon: ")))
                    //.TextStyle(FAppStyle::Get(), "EditorModesToolbar.Label")
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                //.MinWidth(200.0f)
                .VAlign(VAlign_Center)
                [
                    SAssignNew(DungeonComboBox, SComboBox<TWeakObjectPtr<ADungeon>>)
                    .OptionsSource(&DungeonOptions)
                    .OnGenerateWidget(this, &SDungeonThemeEdModeToolbar::GenerateDungeonComboItem)
                    .OnSelectionChanged(this, &SDungeonThemeEdModeToolbar::OnDungeonSelectionChanged)
                    .OnComboBoxOpening(this, &SDungeonThemeEdModeToolbar::OnDungeonComboBoxOpening)
                    .ContentPadding(FMargin(4, 2))
                    [
                        SNew(STextBlock)
                        .Text(this, &SDungeonThemeEdModeToolbar::GetDungeonComboText)
                    ]
                ]
            ]

            // Theme Section
            + SHorizontalBox::Slot()
            .Padding(FMargin(0, 0, 16, 0)) 
            .AutoWidth()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(FMargin(0, 0, 4, 0))
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Theme: ")))
                    //.TextStyle(FAppStyle::Get(), "EditorModesToolbar.Label") 
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                //.MinWidth(200.0f)
                .VAlign(VAlign_Center)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()
                    [
                        SAssignNew(ThemeComboBox, SComboBox<TWeakObjectPtr<UDungeonThemeAsset>>)
                        .OptionsSource(&ThemeOptions)
                        .OnGenerateWidget(this, &SDungeonThemeEdModeToolbar::GenerateThemeComboItem)
                        .OnSelectionChanged(this, &SDungeonThemeEdModeToolbar::OnThemeSelectionChanged)
                        .OnComboBoxOpening(this, &SDungeonThemeEdModeToolbar::OnThemeComboBoxOpening)
                        .Visibility(this, &SDungeonThemeEdModeToolbar::GetThemeComboVisibility)
                        .ContentPadding(FMargin(4, 2))
                        [
                            SNew(STextBlock)
                            .Text(this, &SDungeonThemeEdModeToolbar::GetThemeComboText)
                        ]
                    ]
                    + SOverlay::Slot()
                    .HAlign(HAlign_Center)
                    .VAlign(VAlign_Center)
                    [
                        SAssignNew(ThemeHintText, STextBlock)
                        .Text(FText::FromString(TEXT("Select a dungeon first")))
                        .Visibility_Lambda([this]() {
                            return CurrentDungeon.IsValid() ? EVisibility::Collapsed : EVisibility::Visible;
                        })
                        .ColorAndOpacity(this, &SDungeonThemeEdModeToolbar::GetThemeHintTextColor)
                    ]
                ]
            ]

            +SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SButton)
                .Text(LOCTEXT("BuildDungeonButtonLabel", "Build Dungeon"))
                //.ButtonStyle(FAppStyle::Get(), "FlatButton.Primary")
                .ContentPadding(FMargin(8, 4)) 
                .OnClicked_Raw(this, &SDungeonThemeEdModeToolbar::OnBuildDungeonClicked)
            ]

            
            +SHorizontalBox::Slot()
            .FillWidth(1)
            [
                SNullWidget::NullWidget
            ]
            
            +SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SHorizontalBox)
                +SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SButton)
                    .Text(LOCTEXT("PerformLayoutLabel", "Perform Layout"))
                    .OnClicked(this, &SDungeonThemeEdModeToolbar::HandlePerformLayoutClicked)
                ]
                
                +SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SCheckBox)
                    .IsChecked(this, &SDungeonThemeEdModeToolbar::GetAutoArrangeCheckboxState)
                    .OnCheckStateChanged(this, &SDungeonThemeEdModeToolbar::OnAutoArrangeCheckboxStateChanged)
                    [
                        SNew(SHorizontalBox)
                        +SHorizontalBox::Slot()
                        .AutoWidth()
                        [
                            SNew(STextBlock)
                            .Text(LOCTEXT("AutoLayoutLabel", "Auto Layout"))
                            .ToolTipText(LOCTEXT("AutoLayoutTooltip", "Automatically arranges nodes as they are added or removed."))
                        ]
                    ]
                ]
            ]
        ]
    ];

    if (DungeonOptions.Num() > 0) {
        // Select the first dungeon
        TWeakObjectPtr<ADungeon> Dungeon = DungeonOptions[0];
        if (Dungeon.IsValid()) {
            DungeonComboBox->SetSelectedItem(Dungeon);
            if (Dungeon->Themes.Num() > 0) {
                if (UDungeonThemeAsset* Theme = Dungeon->Themes[0]) {
                    RefreshThemeList();
                    ThemeComboBox->SetSelectedItem(Theme);
                    if (OnThemeSelected.IsBound()) {
                        OnThemeSelected.Execute(Dungeon.Get(), Theme);
                    }
                }
            }
        }
    }
    
}

void SDungeonThemeEdModeToolbar::OnDungeonComboBoxOpening() {
    RefreshDungeonList();
}

void SDungeonThemeEdModeToolbar::RefreshDungeonList() {
    DungeonOptions.Empty();
    
    if (GEditor && GEditor->GetEditorWorldContext().World()) {
        for (TActorIterator<ADungeon> It(GEditor->GetEditorWorldContext().World()); It; ++It) {
            if (ADungeon* DungeonActor = *It) {
                DungeonOptions.Add(DungeonActor);
            }
        }
    }

    if (DungeonComboBox.IsValid()) {
        DungeonComboBox->RefreshOptions();
    }
}

void SDungeonThemeEdModeToolbar::RefreshThemeList() {
    ThemeOptions.Empty();
    
    if (CurrentDungeon.IsValid()) {
        for (const auto& Theme : CurrentDungeon->Themes) {
            if (Theme.Get()) {
                ThemeOptions.Add(Theme.Get());
            }
        }
    }

    if (ThemeComboBox.IsValid()) {
        ThemeComboBox->RefreshOptions();
    }
}

FReply SDungeonThemeEdModeToolbar::OnBuildDungeonClicked() const {
    if (OnDungeonBuildClicked.IsBound()) {
        OnDungeonBuildClicked.Execute();
    }
    return FReply::Handled();
}

ECheckBoxState SDungeonThemeEdModeToolbar::GetAutoArrangeCheckboxState() const {
    return bAutoLayoutGraph ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SDungeonThemeEdModeToolbar::OnAutoArrangeCheckboxStateChanged(ECheckBoxState CheckBoxState) {
    bAutoLayoutGraph = (CheckBoxState == ECheckBoxState::Checked);
	GetMutableDefault<UDungeonArchitectProjectUserSettings>()->SetThemeEdModeAutoLayoutGraph(bAutoLayoutGraph);
    if (OnAutoLayoutStateChange.IsBound()) {
        OnAutoLayoutStateChange.Execute(bAutoLayoutGraph);
    }
}

FReply SDungeonThemeEdModeToolbar::HandlePerformLayoutClicked() const {
    if (OnPerformLayoutClicked.IsBound()) {
        OnPerformLayoutClicked.Execute();
    }
    return FReply::Handled();
}

void SDungeonThemeEdModeToolbar::OnDungeonSelectionChanged(TWeakObjectPtr<ADungeon> NewSelection, ESelectInfo::Type SelectInfo) {
    CurrentDungeon = NewSelection;
    RefreshThemeList();
    
    if (OnDungeonSelected.IsBound() && SelectInfo != ESelectInfo::Direct) {
        OnDungeonSelected.Execute(NewSelection.Get());
    }
}

void SDungeonThemeEdModeToolbar::OnThemeComboBoxOpening() {
    RefreshThemeList();
}

void SDungeonThemeEdModeToolbar::OnThemeSelectionChanged(TWeakObjectPtr<UDungeonThemeAsset> NewSelection, ESelectInfo::Type SelectInfo) {
    if (OnThemeSelected.IsBound() && SelectInfo != ESelectInfo::Direct) {
        OnThemeSelected.Execute(CurrentDungeon.Get(), NewSelection.Get());
    }
}

EVisibility SDungeonThemeEdModeToolbar::GetThemeComboVisibility() const {
    return CurrentDungeon.IsValid() ? EVisibility::Visible : EVisibility::Hidden;
}

FSlateColor SDungeonThemeEdModeToolbar::GetThemeHintTextColor() const {
    return FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 0.8f));  // Slightly grayed out text
}

FText SDungeonThemeEdModeToolbar::GetDungeonComboText() const {
    TWeakObjectPtr<ADungeon> SelectedDungeon = DungeonComboBox->GetSelectedItem();
    return FText::FromString(SelectedDungeon.IsValid() ? SelectedDungeon->GetActorLabel() : TEXT("Select Dungeon..."));
}

FText SDungeonThemeEdModeToolbar::GetThemeComboText() const {
    TWeakObjectPtr<UDungeonThemeAsset> SelectedTheme = ThemeComboBox->GetSelectedItem();
    return FText::FromString(SelectedTheme.IsValid() ? SelectedTheme->GetName() : TEXT("Select Theme..."));
}

TSharedRef<SWidget> SDungeonThemeEdModeToolbar::GenerateDungeonComboItem(TWeakObjectPtr<ADungeon> InDungeon) {
    return SNew(STextBlock)
        .Text(FText::FromString(InDungeon.IsValid() ? InDungeon->GetActorLabel() : TEXT("None")));
}

TSharedRef<SWidget> SDungeonThemeEdModeToolbar::GenerateThemeComboItem(TWeakObjectPtr<UDungeonThemeAsset> InTheme) {
    return SNew(STextBlock)
        .Text(FText::FromString(InTheme.IsValid() ? InTheme->GetName() : TEXT("None")));
}


#undef LOCTEXT_NAMESPACE

