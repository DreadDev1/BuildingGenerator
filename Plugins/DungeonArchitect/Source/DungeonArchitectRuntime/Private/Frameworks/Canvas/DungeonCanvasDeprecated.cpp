//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/DungeonCanvasDeprecated.h"

#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialThemeEditorProperties.h"

//////////////////////////////// ADungeonCanvas ////////////////////////////////
ADungeonCanvas::ADungeonCanvas()
{
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>("SceneRoot");
	RootComponent = SceneRoot;
	
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_DefaultSubObject))
	{
		ClearFlags(RF_Transactional);
	}

	if (HasAnyFlags(RF_ArchetypeObject|RF_ClassDefaultObject)) {
		SetFlags(RF_Public);
	}
	else if (HasAnyFlags(RF_Public)) {
		ClearFlags(RF_Public);
	}
}

////////////////////////////////// UDungeonCanvasBlueprint //////////////////////////////////
UDungeonCanvasBlueprint::UDungeonCanvasBlueprint() {
	PreviewDungeonProperties_DEPRECATED = CreateDefaultSubobject<UDungeonCanvasMaterialThemeEditorProperties>("PreviewDungeonProperties");
	PreviewDungeonProperties_DEPRECATED->SetFlags(RF_DefaultSubObject | RF_Transactional);
}

/*
UDungeonCanvasThemeRenderResources* UDungeonCanvasBlueprint::CreateThemeRenderResources(UObject* OuterOwner) {
	UDungeonCanvasThemeRenderResources* RenderResources = NewObject<UDungeonCanvasThemeRenderResources>(OuterOwner);
	if (UDungeonCanvasBlueprintGeneratedClass* CanvasGeneratedClass = Cast<UDungeonCanvasBlueprintGeneratedClass>(GeneratedClass)) {
		if (CanvasGeneratedClass->MaterialInstance) {
			RenderResources->MasterMaterialInstanceDynamic = UMaterialInstanceDynamic::Create(CanvasGeneratedClass->MaterialInstance, RenderResources);
		}
	}
	return RenderResources;
}
*/

#if WITH_EDITOR
UClass* UDungeonCanvasBlueprint::GetBlueprintClass() const {
	return UDungeonCanvasBlueprintGeneratedClass::StaticClass();
}
#endif // WITH_EDITOR


////////////////////////////////// UDungeonCanvasBlueprintGeneratedClass //////////////////////////////////
UDungeonCanvasBlueprintGeneratedClass::UDungeonCanvasBlueprintGeneratedClass(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MaterialInstance_DEPRECATED(nullptr)
{
}

uint8* UDungeonCanvasBlueprintGeneratedClass::GetPersistentUberGraphFrame(UObject* Obj, UFunction* FuncToCheck) const {
	if (!IsInGameThread()) {
		// we cant use the persistent frame if we are executing in parallel (as we could potentially thunk to BP)
		return nullptr;
	}
	return Super::GetPersistentUberGraphFrame(Obj, FuncToCheck);
}


