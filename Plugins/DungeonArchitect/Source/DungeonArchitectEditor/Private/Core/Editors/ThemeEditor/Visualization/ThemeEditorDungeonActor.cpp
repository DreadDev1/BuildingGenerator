//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/Visualization/ThemeEditorDungeonActor.h"

#include "Core/DungeonConfig.h"
#include "Core/Utils/DungeonUtils.h"
#include "Frameworks/ThemeEngine/Markers/ProceduralMarkers/ProceduralMarkerEmitter.h"

#include "Engine/Engine.h"

#define LOCTEXT_NAMESPACE "ThemeEditorDungeonActor"

void AThemeEditorDungeonActor::HandlePropertyChanged(const FName& InPropertyName) {
	// Perform a full layout rebuild whenever the config changes in any way
	bPerformFullRebuild = true;
	if (PreviewViewportProperties.IsValid()) {
		if (IsValid(Config) && IsValid(PreviewViewportProperties->DungeonConfig) && Config->GetClass() == PreviewViewportProperties->DungeonConfig->GetClass()) {
			UEngine::FCopyPropertiesForUnrelatedObjectsParams CopyParams;
			CopyParams.bPerformDuplication = true;
			CopyParams.bDoDelta = false;
			UEngine::CopyPropertiesForUnrelatedObjects(Config, PreviewViewportProperties->DungeonConfig, CopyParams);
		}
		else {
			PreviewViewportProperties->DungeonConfig = NewObject<UDungeonConfig>(PreviewViewportProperties.Get(), Config->GetClass(), NAME_None, RF_NoFlags, Config); 
		}
		
		if (UDungeonBuilder* DungeonBuilder = GetBuilder()) {
			PreviewViewportProperties->BuilderClass = DungeonBuilder->GetClass();
		}
		PreviewViewportProperties->PCGGraphs = PCGGraphs;

		if (InPropertyName == GET_MEMBER_NAME_CHECKED(ADungeon, MarkerEmitters)) {
			FDungeonUtils::CloneUObjectArray(PreviewViewportProperties.Get(), MarkerEmitters, PreviewViewportProperties->MarkerEmitters);
		}
		
		if (InPropertyName == GET_MEMBER_NAME_CHECKED(ADungeon, ProceduralMarkerEmitters)) {
			FDungeonUtils::CloneUObjectArray(PreviewViewportProperties.Get(), ProceduralMarkerEmitters, PreviewViewportProperties->ProceduralMarkerEmitters);
		}
		
		PreviewViewportProperties->Modify();
	}	
}

void AThemeEditorDungeonActor::InitFromPreviewViewportSettings(UDungeonEditorViewportProperties* InPreviewViewportProperties) {
	check(InPreviewViewportProperties);
	
	PreviewViewportProperties = InPreviewViewportProperties;
	PCGGraphs = InPreviewViewportProperties->PCGGraphs;

	if (PreviewViewportProperties->MarkerEmitters.Num() > 0) {
		TArray<TObjectPtr<UDungeonMarkerEmitter>> ClonedArray;
		FDungeonUtils::CloneUObjectArray(this, PreviewViewportProperties->MarkerEmitters, ClonedArray);
		MarkerEmitters = ClonedArray;
	}
		
	if (PreviewViewportProperties->ProceduralMarkerEmitters.Num() > 0) {
		TArray<TObjectPtr<UProceduralMarkerEmitter>> ClonedArray;
		FDungeonUtils::CloneUObjectArray(this, PreviewViewportProperties->ProceduralMarkerEmitters, ClonedArray);
		ProceduralMarkerEmitters = ClonedArray;
	}
	
	bPerformFullRebuild = true;

	if (PreviewViewportProperties.IsValid()) {
		if (BuilderClass != PreviewViewportProperties->BuilderClass) {
			SetBuilderClass(PreviewViewportProperties->BuilderClass);
		}
		if (PreviewViewportProperties->DungeonConfig) {
			UEngine::FCopyPropertiesForUnrelatedObjectsParams CopyParams;
			CopyParams.bPerformDuplication = true;
			CopyParams.bDoDelta = false;
			UEngine::CopyPropertiesForUnrelatedObjects(PreviewViewportProperties->DungeonConfig, GetConfig(), CopyParams);
		}
		else {
			PreviewViewportProperties->DungeonConfig = NewObject<UDungeonConfig>(PreviewViewportProperties.Get(), Config->GetClass(), NAME_None, RF_NoFlags, Config); 
		}
	}
}

#undef LOCTEXT_NAMESPACE

