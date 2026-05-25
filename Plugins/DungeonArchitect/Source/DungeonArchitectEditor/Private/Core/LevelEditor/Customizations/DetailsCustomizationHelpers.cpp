//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/Customizations/DetailsCustomizationHelpers.h"

#include "Core/Common/Utils/DungeonEditorUtils.h"
#include "Core/DungeonConfig.h"

#include "DetailCategoryBuilder.h"

ADungeon* DADetailsCustomizationHelpers::GetDungeon(IDetailLayoutBuilder& DetailBuilder) {
	return GetEditedObject<ADungeon>(DetailBuilder);
}

void DADetailsCustomizationHelpers::ShowDungeonConfigProperties(IDetailLayoutBuilder& DetailBuilder, UDungeonConfig* Config) {
	if (!Config) return;

	TArray<FName> ImportantAttributes;
	ImportantAttributes.Add("Seed");

	TArray<UObject*> Configs;
	Configs.Add(Config);
    
	const FText ConfigName = Config->GetClass()->GetDisplayNameText();
	IDetailCategoryBuilder& ConfigCategory = DetailBuilder.EditCategory(*ConfigName.ToString(), FText::GetEmpty(), ECategoryPriority::Important);

	// First add the important attributes, so they show up on top of the config properties list
	for (const FName& Attrib : ImportantAttributes) {
		ConfigCategory.AddExternalObjectProperty(Configs, Attrib);
	}

	TArray<FProperty*> ExperimentalProperties;

	for (TFieldIterator<FProperty> PropIt(Config->GetClass()); PropIt; ++PropIt) {
		FProperty* Property = *PropIt;

		if (Property->HasMetaData("Experimental")) {
			ExperimentalProperties.Add(Property);
			continue;
		}

		FName PropertyName(*(Property->GetName()));

		// Make sure we skip the important attrib, since we have already added them above
		if (ImportantAttributes.Contains(PropertyName)) {
			// Already added
			continue;
		}

		ConfigCategory.AddExternalObjectProperty(Configs, PropertyName);
	}

	IDetailCategoryBuilder& ExperimentalCategory = DetailBuilder.EditCategory("Experimental");
	for (FProperty* Property : ExperimentalProperties) {
		const FName PropertyName(*(Property->GetName()));
		ExperimentalCategory.AddExternalObjectProperty(Configs, PropertyName);
	}
}

void DADetailsCustomizationHelpers::NotifyPropertyChanged(UObject* InObject, const FName& InFieldName) {
	if (InObject) {
		if (FProperty* BuilderClassProperty = FindFProperty<FProperty>(InObject->GetClass(), InFieldName)) {
			InObject->PreEditChange(BuilderClassProperty);

			FPropertyChangedEvent PropertyEvent(BuilderClassProperty);
			InObject->PostEditChangeProperty(PropertyEvent);
		}
	}
}

void DADetailsCustomizationHelpers::ShowLevelStreamingProperties(IDetailLayoutBuilder& DetailBuilder, FDungeonLevelStreamingConfig* StreamingConfig) {
	const FString ConfigName = "Level Streaming";
	IDetailCategoryBuilder& ConfigCategory = DetailBuilder.EditCategory(*ConfigName);

	const TSharedRef<FStructOnScope> ConfigStructRef = MakeShared<FStructOnScope>(
		FDungeonLevelStreamingConfig::StaticStruct(), reinterpret_cast<uint8*>(StreamingConfig));
	ConfigCategory.AddAllExternalStructureProperties(ConfigStructRef);
}

