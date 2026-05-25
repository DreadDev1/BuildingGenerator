//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/CanvasEditor/DungeonCanvasDetailCustomization.h"

#include "Core/Common/Utils/DungeonEditorUtils.h"
#include "Frameworks/Canvas/DungeonCanvasMaterialLayer.h"
#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialTheme.h"

#include "Editor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Internationalization/Regex.h"
#include "Materials/MaterialFunctionMaterialLayer.h"
#include "Materials/MaterialInstanceConstant.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Input/SNumericEntryBox.h"

#define LOCTEXT_NAMESPACE "DungeonCanvasDetailCustomization"

///////////////////////////////////// FDungeonCanvasMaterialLayerCustomization /////////////////////////////////////


class FDungeonCanvasCustomizationLib {
public:
    static void UpdateMIC(UMaterialInstanceConstant* MIC) {
        MIC->PostEditChange();
        //UMaterialEditingLibrary::UpdateMaterialInstance(MIC);
        //FMaterialUpdateContext UpdateContext(FMaterialUpdateContext::EOptions::Default, GMaxRHIShaderPlatform);
        //UpdateContext.AddMaterialInstance(MIC);
        //MIC->MarkPackageDirty();
    }
};

class UDungeonCanvasMaterialTheme;
class UDungeonCanvasMaterialLayer;

class FDungeonCanvasColorPicker {
public:
    static FDungeonCanvasColorPicker& Get() { return Singleton; }

    struct FSettings {
        FLinearColor InitialColor;
        FMaterialParameterInfo ParamInfo;
        TWeakObjectPtr<UMaterialInstanceConstant> MaterialInstanceWeakPtr;
        TSharedPtr<SWidget> ParentDetailsWidget;
        TWeakObjectPtr<UDungeonCanvasMaterialLayer> CanvasMatLayerPtr;
        TWeakObjectPtr<UDungeonCanvasMaterialTheme> CanvasThemePtr;
    };

    void ShowColorPicker(const FSettings& InSettings) {
        Settings = InSettings;
        
        GEditor->BeginTransaction(FText::Format(LOCTEXT("SetColorProperty", "Edit Color: {0}"), FText::FromName(Settings.ParamInfo.Name)));
        
        FColorPickerArgs PickerArgs;
        PickerArgs.bUseAlpha = true;
        PickerArgs.bOnlyRefreshOnMouseUp = false;
        PickerArgs.bOnlyRefreshOnOk = false;
        PickerArgs.sRGBOverride = false;
        PickerArgs.DisplayGamma = TAttribute<float>::Create(TAttribute<float>::FGetter::CreateUObject(GEngine, &UEngine::GetDisplayGamma));
        PickerArgs.OnColorCommitted = FOnLinearColorValueChanged::CreateRaw(this, &FDungeonCanvasColorPicker::OnSetColorFromColorPicker);
        PickerArgs.OnColorPickerCancelled = FOnColorPickerCancelled::CreateRaw(this, &FDungeonCanvasColorPicker::OnColorPickerCancelled);
        PickerArgs.OnColorPickerWindowClosed = FOnWindowClosed::CreateRaw(this, &FDungeonCanvasColorPicker::OnColorPickerWindowClosed);
        PickerArgs.InitialColor = Settings.InitialColor;
        PickerArgs.ParentWidget = Settings.ParentDetailsWidget;
        PickerArgs.OptionalOwningDetailsView = Settings.ParentDetailsWidget;
        FWidgetPath ParentWidgetPath;
        if (FSlateApplication::Get().FindPathToWidget(Settings.ParentDetailsWidget.ToSharedRef(), ParentWidgetPath))
        {
            PickerArgs.bOpenAsMenu = FSlateApplication::Get().FindMenuInWidgetPath(ParentWidgetPath).IsValid();
        }
        
	    OpenColorPicker(PickerArgs);
    }
private:
    void OnSetColorFromColorPicker(FLinearColor NewColor) {
        SetMaterialColor(NewColor);
    }

    void OnColorPickerCancelled(FLinearColor OriginalColor) {
        SetMaterialColor(OriginalColor);
	    GEditor->CancelTransaction(0);
    }

    void OnColorPickerWindowClosed(const TSharedRef<SWindow>& Window) {
        GEditor->EndTransaction();
    }
    
    void SetMaterialColor(const FLinearColor& InColor) const {
        if (UMaterialInstanceConstant* Mat = Settings.MaterialInstanceWeakPtr.Get()) {
            Mat->SetVectorParameterValueEditorOnly(Settings.ParamInfo, InColor);
            FDungeonCanvasCustomizationLib::UpdateMIC(Mat);

            if (Settings.CanvasMatLayerPtr.IsValid()) {
                Settings.CanvasMatLayerPtr->Modify();
            }
            if (Settings.CanvasThemePtr.IsValid()) {
                Settings.CanvasThemePtr->Modify();
            }
        }
    }
    
private:
    FSettings Settings{};
    static FDungeonCanvasColorPicker Singleton;
};
FDungeonCanvasColorPicker FDungeonCanvasColorPicker::Singleton;

void FDungeonCanvasMaterialLayerCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) {
    UDungeonCanvasMaterialLayer* CanvasMatLayer = FDungeonEditorUtils::GetBuilderObject<UDungeonCanvasMaterialLayer>(&DetailBuilder);
    if (!CanvasMatLayer || !CanvasMatLayer->MaterialLayer) {
        return;
    }

    int32 LayerIndex = CanvasMatLayer->GetLayerIndex();
    UDungeonCanvasMaterialTheme* Theme = Cast<UDungeonCanvasMaterialTheme>(CanvasMatLayer->GetOuter());
    UMaterialInstanceConstant* MaterialInstance = Theme ? Theme->CompiledThemeMaterial : nullptr;
    if (!MaterialInstance) {
        return;
    }

    TArray<FGuid> TempGuids;
    TWeakObjectPtr<UMaterialInstanceConstant> MaterialInstanceWeakPtr = MaterialInstance;

    auto MarkDirty = [CanvasMatLayer, Theme]() {
        CanvasMatLayer->Modify();
    };

    auto MakeResetToDefaultWidget = [](const TFunction<bool()>& FnIsResetToDefaultVisible, const TFunction<void()>& FnHandleResetToDefaultClicked) {
        static FSlateIcon EnabledResetToDefaultIcon(FAppStyle::Get().GetStyleSetName(), "PropertyWindow.DiffersFromDefault");
        static FSlateIcon DisabledResetToDefaultIcon(FAppStyle::Get().GetStyleSetName(), "NoBrush");
        FSlimHorizontalToolBarBuilder ToolbarBuilder(TSharedPtr<FUICommandList>(), FMultiBoxCustomization::None);
        ToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);
        ToolbarBuilder.SetStyle(&FAppStyle::Get(), "DetailsView.ExtensionToolBar");
        ToolbarBuilder.SetIsFocusable(false);
        ToolbarBuilder.AddToolBarButton(
            FUIAction(
                FExecuteAction::CreateLambda([FnHandleResetToDefaultClicked]() {
                    FnHandleResetToDefaultClicked();
                }),
                FCanExecuteAction::CreateLambda([FnIsResetToDefaultVisible]()
                {
                    return FnIsResetToDefaultVisible();
                })
            ),
            NAME_None,
            NSLOCTEXT("PropertyEditor", "ResetToDefault", "Reset to Default"),
            TAttribute<FText>::Create([FnIsResetToDefaultVisible]() {
                return FnIsResetToDefaultVisible() ?
                    NSLOCTEXT("PropertyEditor", "ResetToDefaultPropertyValueToolTip", "Reset this property to its default value.") :
                    FText::GetEmpty();
            }),
            TAttribute<FSlateIcon>::Create([FnIsResetToDefaultVisible]()
            {
                return FnIsResetToDefaultVisible() ?
                    EnabledResetToDefaultIcon :
                    DisabledResetToDefaultIcon;
            }));
        return ToolbarBuilder.MakeWidget();
    };

    struct FPropertyEntry {
        FName ParamName;
        FName Category = NAME_None;
        int32 SortPriority{};
        TSharedPtr<SWidget> Content;
    };
    TArray<FPropertyEntry> PropertyEntries;
    
    // Scalar Parameters
    {
        TArray<FMaterialParameterInfo> ScalarMaterialParams;
        MaterialInstance->GetAllScalarParameterInfo(ScalarMaterialParams, TempGuids);
        for (const FMaterialParameterInfo& ParamInfo : ScalarMaterialParams) {
            if (ParamInfo.Association == LayerParameter && ParamInfo.Index == LayerIndex) {
                float SliderMin{}, SliderMax{};
                MaterialInstance->GetScalarParameterSliderMinMax(ParamInfo, SliderMin, SliderMax);
                TSharedPtr<SNumericEntryBox<float>> EntryBox;
                auto FnGetValue = [ParamInfo, MaterialInstanceWeakPtr]() {
                    if (const UMaterialInstanceConstant* Mat = MaterialInstanceWeakPtr.Get()) {
                        float Value{};
                        Mat->GetScalarParameterValue(ParamInfo, Value);
                        return TOptional<float>(Value);
                    }
                    else {
                        return TOptional<float>(0);
                    }
                };

                auto FnSetValue = [ParamInfo, MaterialInstanceWeakPtr, MarkDirty](float NewValue) {
                    if (UMaterialInstanceConstant* Mat = MaterialInstanceWeakPtr.Get()) {
                        Mat->SetScalarParameterValueEditorOnly(ParamInfo, NewValue);
                        FDungeonCanvasCustomizationLib::UpdateMIC(Mat);
                        MarkDirty();
                    }
                };

                if (FMath::IsNearlyEqual(SliderMin, SliderMax)) {
                    EntryBox = SNew(SNumericEntryBox<float>)
                        .Value_Lambda(FnGetValue)
                        .OnValueChanged_Lambda(FnSetValue);
                }
                else {
                    EntryBox = SNew(SNumericEntryBox<float>)
                        .AllowSpin(true)
                        .MinSliderValue(SliderMin)
                        .MaxSliderValue(SliderMax)
                        .Value_Lambda(FnGetValue)
                        .OnValueChanged_Lambda(FnSetValue);
                }

                ////////////// Reset to default handlers //////////////
                TFunction<bool()> FnIsResetToDefaultVisible = [MaterialInstance, ParamInfo]() {
                    float DefaultValue{}, CurrentValue{};
                    MaterialInstance->GetScalarParameterDefaultValue(ParamInfo, DefaultValue);
                    MaterialInstance->GetScalarParameterValue(ParamInfo, CurrentValue);
                    return !FMath::IsNearlyEqual(DefaultValue, CurrentValue);
                };

                TFunction<void()> FnHandleResetToDefaultClicked = [MaterialInstance, MarkDirty, ParamInfo]() {
                    float DefaultValue{};
                    MaterialInstance->GetScalarParameterDefaultValue(ParamInfo, DefaultValue);
                    MaterialInstance->SetScalarParameterValueEditorOnly(ParamInfo, DefaultValue);

                    FDungeonCanvasCustomizationLib::UpdateMIC(MaterialInstance);
                    MarkDirty();
                };
                ///////////////////////////////////////////////////////

                FPropertyEntry& PropertyEntry = PropertyEntries.AddDefaulted_GetRef();
                MaterialInstance->GetParameterSortPriority(ParamInfo, PropertyEntry.SortPriority);
                MaterialInstance->GetGroupName(ParamInfo, PropertyEntry.Category);
                PropertyEntry.ParamName = ParamInfo.Name;
                PropertyEntry.Content = SNew(SHorizontalBox)
                    +SHorizontalBox::Slot()
                    .FillWidth(1.0)
                    [
                        EntryBox.ToSharedRef()
                    ]
                    +SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        MakeResetToDefaultWidget(FnIsResetToDefaultVisible, FnHandleResetToDefaultClicked)
                    ];
                
            }
        }
    }

    // Vector Color Parameters
    {
        TArray<FMaterialParameterInfo> VectorMaterialParams;
        MaterialInstance->GetAllVectorParameterInfo(VectorMaterialParams, TempGuids);
        for (const FMaterialParameterInfo& ParamInfo : VectorMaterialParams) {
            if (ParamInfo.Association == LayerParameter && ParamInfo.Index == LayerIndex) {
                TSharedPtr<SBox> ParentBlockParent = SNew(SBox);
                TSharedPtr<SColorBlock> ColorBlockWidget = SNew(SColorBlock)
                    .AlphaBackgroundBrush(FAppStyle::Get().GetBrush("ColorPicker.RoundedAlphaBackground"))
                    .IsEnabled(true)
                    .ShowBackgroundForAlpha(true)
                    .AlphaDisplayMode(EColorBlockAlphaDisplayMode::Separate)
                    .Size(FVector2D(70.0f, 20.0f))
                    .CornerRadius(FVector4(4.0f,4.0f,4.0f,4.0f))
                    .Color_Lambda([MaterialInstanceWeakPtr, ParamInfo]() {
                        if (const UMaterialInstanceConstant* Mat = MaterialInstanceWeakPtr.Get()) {
                            FLinearColor Value{};
                            Mat->GetVectorParameterValue(ParamInfo, Value);
                            return Value;
                        }
                        else {
                            return FLinearColor::Black;
                        }
                    })
                    .OnMouseButtonDown_Lambda([MaterialInstanceWeakPtr, ParamInfo, ParentBlockParent, Theme, CanvasMatLayer](const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) {
                        FLinearColor InitialColor = FLinearColor::Black;
                        if (const UMaterialInstanceConstant* Mat = MaterialInstanceWeakPtr.Get()) {
                            Mat->GetVectorParameterValue(ParamInfo, InitialColor);
                        }

                        FDungeonCanvasColorPicker::FSettings PickerSettings;
                        PickerSettings.InitialColor = InitialColor;
                        PickerSettings.ParamInfo = ParamInfo;
                        PickerSettings.MaterialInstanceWeakPtr = MaterialInstanceWeakPtr;
                        PickerSettings.ParentDetailsWidget = ParentBlockParent;
                        PickerSettings.CanvasThemePtr = Theme;
                        PickerSettings.CanvasMatLayerPtr = CanvasMatLayer;

                        FDungeonCanvasColorPicker::Get().ShowColorPicker(PickerSettings);
                        return FReply::Handled();
                    });
                
                ParentBlockParent->SetContent(ColorBlockWidget.ToSharedRef());

                
                ////////////// Reset to default handlers //////////////
                TFunction<bool()> FnIsResetToDefaultVisible = [MaterialInstance, ParamInfo]() {
                    FLinearColor DefaultValue{}, CurrentValue{};
                    MaterialInstance->GetVectorParameterDefaultValue(ParamInfo, DefaultValue);
                    MaterialInstance->GetVectorParameterValue(ParamInfo, CurrentValue);
                    return DefaultValue != CurrentValue;
                };

                TFunction<void()> FnHandleResetToDefaultClicked = [MaterialInstance, ParamInfo, MarkDirty]() {
                    FLinearColor DefaultValue{};
                    MaterialInstance->GetVectorParameterDefaultValue(ParamInfo, DefaultValue);
                    MaterialInstance->SetVectorParameterValueEditorOnly(ParamInfo, DefaultValue);

                    FDungeonCanvasCustomizationLib::UpdateMIC(MaterialInstance);
                    MarkDirty();
                };
                ///////////////////////////////////////////////////////

                FPropertyEntry& PropertyEntry = PropertyEntries.AddDefaulted_GetRef();
                MaterialInstance->GetParameterSortPriority(ParamInfo, PropertyEntry.SortPriority);
                MaterialInstance->GetGroupName(ParamInfo, PropertyEntry.Category);
                PropertyEntry.ParamName = ParamInfo.Name;
                PropertyEntry.Content = SNew(SHorizontalBox)
                    +SHorizontalBox::Slot()
                    .FillWidth(1.0)
                    [
                        ParentBlockParent.ToSharedRef()
                    ]
                    +SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        MakeResetToDefaultWidget(FnIsResetToDefaultVisible, FnHandleResetToDefaultClicked)
                    ];
            }
        }
    }

    // Texture Parameter
    {
        TArray<FMaterialParameterInfo> TextureMaterialParams;
        MaterialInstance->GetAllTextureParameterInfo(TextureMaterialParams, TempGuids);
        for (const FMaterialParameterInfo& ParamInfo : TextureMaterialParams) {
            if (ParamInfo.Association == LayerParameter && ParamInfo.Index == LayerIndex) {
                
                ////////////// Reset to default handlers //////////////
                TFunction<bool()> FnIsResetToDefaultVisible = [MaterialInstance, ParamInfo]() {
                    UTexture* DefaultValue{};
                    UTexture* CurrentValue{};
                    MaterialInstance->GetTextureParameterDefaultValue(ParamInfo, DefaultValue);
                    MaterialInstance->GetTextureParameterValue(ParamInfo, CurrentValue);
                    return DefaultValue != CurrentValue;
                };

                TFunction<void()> FnHandleResetToDefaultClicked = [MaterialInstance, ParamInfo, MarkDirty]() {
                    UTexture* DefaultValue{};
                    MaterialInstance->GetTextureParameterDefaultValue(ParamInfo, DefaultValue);
                    MaterialInstance->SetTextureParameterValueEditorOnly(ParamInfo, DefaultValue);

                    FDungeonCanvasCustomizationLib::UpdateMIC(MaterialInstance);
                    MarkDirty();
                };
                ///////////////////////////////////////////////////////

                FPropertyEntry& PropertyEntry = PropertyEntries.AddDefaulted_GetRef();
                MaterialInstance->GetParameterSortPriority(ParamInfo, PropertyEntry.SortPriority);
                MaterialInstance->GetGroupName(ParamInfo, PropertyEntry.Category);
                PropertyEntry.ParamName = ParamInfo.Name;
                PropertyEntry.Content = SNew(SHorizontalBox)
                    +SHorizontalBox::Slot()
                    .FillWidth(1.0)
                    [
                        SNew(SObjectPropertyEntryBox)
                        .AllowedClass(UTexture::StaticClass())
                        .AllowClear(true)
                        .DisplayUseSelected(true)
                        .DisplayBrowse(true)
                        .DisplayThumbnail(true)
			            .ThumbnailPool(DetailBuilder.GetThumbnailPool())
                        .ObjectPath_Lambda([ParamInfo, MaterialInstanceWeakPtr]() {
                            UTexture* Texture{};
                            if (const UMaterialInstanceConstant* Mat = MaterialInstanceWeakPtr.Get()) {
                                Mat->GetTextureParameterValue(ParamInfo, Texture);
                            }
                            return Texture ? Texture->GetPathName() : "";
                        })
                        .OnObjectChanged_Lambda([ParamInfo, MaterialInstanceWeakPtr, MarkDirty](const FAssetData& InAssetData) {
                            if (UMaterialInstanceConstant* Mat = MaterialInstanceWeakPtr.Get()) {
                                UTexture* NewTexture = Cast<UTexture>(InAssetData.GetAsset());
                                Mat->SetTextureParameterValueEditorOnly(ParamInfo, NewTexture);
                                FDungeonCanvasCustomizationLib::UpdateMIC(Mat);
                                MarkDirty();
                            }
                        })
                    ]
                    +SHorizontalBox::Slot()
                    .AutoWidth()
                    [
                        MakeResetToDefaultWidget(FnIsResetToDefaultVisible, FnHandleResetToDefaultClicked)
                    ];
            }
        }
    }

    PropertyEntries.Sort([](const FPropertyEntry& A, const FPropertyEntry& B) {
        return A.SortPriority < B.SortPriority;
    });

    TMap<FName, IDetailCategoryBuilder*> CategoriesByName;
    const FName InternalCategoryName = "Internal";
    const FName DefaultCategoryName = "Material Layer Inputs";
    IDetailCategoryBuilder& DefaultFallbackCategory = DetailBuilder.EditCategory("Material Layer Inputs", FText::GetEmpty(), ECategoryPriority::Uncommon);
    for (const FPropertyEntry& Entry : PropertyEntries) {
        FName CategoryName = Entry.Category.IsNone() ? DefaultCategoryName : Entry.Category;
        if (CategoryName == InternalCategoryName) {
            // Do no display internal parameters
            continue;
        }
        IDetailCategoryBuilder* Category{};
        if (IDetailCategoryBuilder** SearchResult = CategoriesByName.Find(CategoryName)) {
            Category = *SearchResult;
        }
        else {
            FString UnparsedCategoryName = CategoryName.ToString();
            if (UnparsedCategoryName.IsEmpty()) {
                Category = &DefaultFallbackCategory;
            }
            else {
                FString DisplayCategoryName = UnparsedCategoryName;
                int32 SortOrder = INDEX_NONE;
            
                FRegexPattern Pattern(TEXT("^(.+?)\\s*(?:\\[(\\d+)\\])?$"));
                FRegexMatcher Matcher(Pattern, DisplayCategoryName);
                if (Matcher.FindNext()) {
                    DisplayCategoryName = UnparsedCategoryName.Mid(Matcher.GetCaptureGroupBeginning(1), Matcher.GetCaptureGroupEnding(1) - Matcher.GetCaptureGroupBeginning(1));
                    if (Matcher.GetCaptureGroupBeginning(2) != INDEX_NONE) {
                        FString SortOrderString = UnparsedCategoryName.Mid(Matcher.GetCaptureGroupBeginning(2), Matcher.GetCaptureGroupEnding(2) - Matcher.GetCaptureGroupBeginning(2));
                        SortOrder = FCString::Atoi(*SortOrderString);
                    }
                }
            
                Category = &DetailBuilder.EditCategory(FName(DisplayCategoryName), FText::GetEmpty(), ECategoryPriority::Uncommon);
                Category->SetSortOrder(5000 + SortOrder);
                CategoriesByName.Add(Entry.Category, Category);
            }
        }
        if (!Category) {
            continue;
        }

        FString DisplayString = FName::NameToDisplayString(Entry.ParamName.ToString(), false);
        Category->AddCustomRow(FText::FromName(Entry.ParamName))
            .NameContent()
            [
                SNew(STextBlock).Text(FText::FromString(DisplayString))
            ]
            .ValueContent()
            .MinDesiredWidth(125.f)
            [
                Entry.Content.ToSharedRef()
            ];
    }

    IDetailCategoryBuilder& MaterialFunctionsCategory = DetailBuilder.EditCategory("Material Functions", FText::GetEmpty(), ECategoryPriority::Uncommon);
    MaterialFunctionsCategory.SetSortOrder(6000);

}

TSharedRef<IDetailCustomization> FDungeonCanvasMaterialLayerCustomization::MakeInstance() {
    return MakeShareable(new FDungeonCanvasMaterialLayerCustomization);
}

#undef LOCTEXT_NAMESPACE

