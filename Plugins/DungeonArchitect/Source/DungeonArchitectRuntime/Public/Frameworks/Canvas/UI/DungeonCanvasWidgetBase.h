//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Canvas/Cameras/DungeonCanvasCamera.h"
#include "Frameworks/Canvas/DungeonCanvas.h"
#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialTheme.h"

#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Input/Events.h"
#include "Layout/Geometry.h"
#include "DungeonCanvasWidgetBase.generated.h"

class UDungeonCanvasThemeRenderResources;
class UDungeonCanvasMaterialThemeBaker;
class UMaterialInterface;

UCLASS(Abstract, HideDropdown, Blueprintable)
class DUNGEONARCHITECTRUNTIME_API UDungeonCanvasWidgetBase : public UUserWidget {
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

	UFUNCTION(BlueprintCallable, Category = "Dungeon Canvas")
	void SetCanvasComponent(UDungeonCanvasComponent* CanvasComponent);

private:
	UFUNCTION()
	void OnCanvasRebuildRequested(UDungeonCanvasComponent* Canvas);

	void BindToCanvas();
	void UnbindFromCanvas();

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon Canvas", meta=(UIMin=0.05, UIMax=1.0))
	float CanvasTextureScaleMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon Canvas", meta=(UIMin=0.05, UIMax=1.0))
	FName DungeonActorTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Canvas")
	FDungeonCanvasDrawSettings DrawSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon Canvas")
	TObjectPtr<UMaterialInterface> MinimapMaterialTemplate;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon Canvas")
	TObjectPtr<UDungeonCanvasMaterialTheme> CanvasThemeOverride;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget), Category="Dungeon Canvas")
	TObjectPtr<UImage> CanvasImageWidget;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category="Dungeon Canvas")
	TObjectPtr<UDungeonCanvasCamera> Camera = nullptr;;

protected:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MinimapMaterialInstance;

	UPROPERTY(Transient, BlueprintReadWrite, Category="Dungeon Canvas")
	TWeakObjectPtr<UDungeonCanvasComponent> DungeonCanvasComponent = nullptr;

	UPROPERTY()
	TObjectPtr<UDungeonCanvasMaterialThemeBaker> ThemeBaker = nullptr;;
};

