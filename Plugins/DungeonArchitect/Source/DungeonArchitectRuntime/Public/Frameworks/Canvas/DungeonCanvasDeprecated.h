//$ Copyright 2015-24, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Canvas/DungeonCanvasStructs.h"

#include "UObject/Package.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "GameFramework/Actor.h"
#include "DungeonCanvasDeprecated.generated.h"

class UDungeonCanvasMaterialThemeEditorProperties;
class UDungeonEditorViewportProperties;
class UDungeonCanvasMaterialLayer;
class UMaterialInstanceConstant;


UCLASS(Blueprintable, BlueprintType, hidecategories = (Rendering,Input,Actor,Misc,Replication,Collision,LOD,Cooking,HLOD,Physics,Networking))
class DUNGEONARCHITECTRUNTIME_API ADungeonCanvas : public AActor {
	GENERATED_BODY()
public:
	ADungeonCanvas();

	// AActor Interface
	//~ End AActor Interface

	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot = nullptr;;	

public:
	// Deprecated Theme Properties.   They'll be removed in the future updates, For now they're used for extracting the theme, on to a separate asset (right click on the canvas blueprint asset in the content browser)
	
	UPROPERTY()
	TArray<FDungeonCanvasOverlayIcon> OverlayIcons_DEPRECATED;
	
	UPROPERTY()
	float OverlayIconOpacity_DEPRECATED = 0.85f;
	
	UPROPERTY()
	FDungeonCanvasOverlayInternalIcon StairIcon_DEPRECATED;
	
	UPROPERTY()
	TArray<TObjectPtr<UDungeonCanvasEffectBase>> Effects_DEPRECATED;

	UPROPERTY()
	TSoftObjectPtr<UMaterialInterface> MaterialTemplateCanvas_DEPRECATED;
	
	UPROPERTY()
	TSoftObjectPtr<UMaterialInterface> FogOfWarMaterialTemplate_DEPRECATED;

};


UCLASS(BlueprintType, Blueprintable, Meta=(IgnoreClassThumbnail, DontUseGenericSpawnObject="true"))
class DUNGEONARCHITECTRUNTIME_API UDungeonCanvasBlueprint : public UBlueprint {
	GENERATED_BODY()
public:
	UDungeonCanvasBlueprint();

	//UFUNCTION(BlueprintCallable, Category="DungeonCanvas")
	//UDungeonCanvasThemeRenderResources* CreateThemeRenderResources(UObject* OuterOwner);

	
	//~ Begin UBlueprint Interface
#if WITH_EDITOR
	virtual bool AlwaysCompileOnLoad() const override { return true; }
	virtual UClass* GetBlueprintClass() const override;
#endif // WITH_EDITOR
	//~ End UBlueprint Interface

public:
	UPROPERTY()
	TObjectPtr<UDungeonCanvasMaterialThemeEditorProperties> PreviewDungeonProperties_DEPRECATED;

	UPROPERTY()
	TArray<TObjectPtr<UDungeonCanvasMaterialLayer>> MaterialLayers_DEPRECATED;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceConstant> MaterialInstance_DEPRECATED = nullptr;;
};



UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonCanvasBlueprintGeneratedClass : public UBlueprintGeneratedClass
{
	GENERATED_UCLASS_BODY()
public:
	//~ Begin UBlueprintGeneratedClass interface
	virtual uint8* GetPersistentUberGraphFrame(UObject* Obj, UFunction* FuncToCheck) const override;
	//~ End UBlueprintGeneratedClass interface
	
	/**
	* Gets UDungeonCanvasBlueprintGeneratedClass from class hierarchy
	* @return UDungeonCanvasBlueprintGeneratedClass or NULL
	*/
	FORCEINLINE static UDungeonCanvasBlueprintGeneratedClass* GetDungeonCanvasGeneratedClass(UClass* InClass)
	{
		UDungeonCanvasBlueprintGeneratedClass* ScriptClass = nullptr;
		for (UClass* MyClass = InClass; MyClass && !ScriptClass; MyClass = MyClass->GetSuperClass()) {
			ScriptClass = Cast<UDungeonCanvasBlueprintGeneratedClass>(MyClass);
		}
		return ScriptClass;
	}

	UPROPERTY()
	TObjectPtr<UMaterialInstanceConstant> MaterialInstance_DEPRECATED;
	
	UPROPERTY()
	TArray<TObjectPtr<UDungeonCanvasMaterialLayer>> MaterialLayers_DEPRECATED;
};

