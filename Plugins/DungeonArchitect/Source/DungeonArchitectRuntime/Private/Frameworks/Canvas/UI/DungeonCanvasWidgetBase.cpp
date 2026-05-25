//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/UI/DungeonCanvasWidgetBase.h"

#include "Core/Dungeon.h"
#include "Frameworks/Canvas/DungeonCanvas.h"
#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialThemeBaker.h"

#include "Engine/CanvasRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogDungeonCanvasWidget, Log, All);

void UDungeonCanvasWidgetBase::NativeConstruct() {
	Super::NativeConstruct();

	if (ThemeBaker) {
		ThemeBaker->ReleaseResources();
		ThemeBaker = nullptr;
	}

	if (DungeonActorTag.IsNone()) {
		if (ADungeon* Dungeon = Cast<ADungeon>(UGameplayStatics::GetActorOfClass(this, ADungeon::StaticClass()))) {
			DungeonCanvasComponent = Dungeon->GetComponentByClass<UDungeonCanvasComponent>();
		}
	}
	else {
		TArray<AActor*> DungeonActors;
		UGameplayStatics::GetAllActorsOfClassWithTag(this, ADungeon::StaticClass(), DungeonActorTag, DungeonActors);
		for (AActor* DungeonActor : DungeonActors) {
			if (ADungeon* Dungeon = Cast<ADungeon>(DungeonActor)) {
				DungeonCanvasComponent = Dungeon->GetComponentByClass<UDungeonCanvasComponent>();
				if (DungeonCanvasComponent.IsValid()) {
					break;
				}
			}
		}
	}
	
	BindToCanvas();

	// Setup the camera
	if (Camera) {
		Camera->PlayerController = GetOwningPlayer();
		Camera->Initialize(DungeonCanvasComponent.Get());
	}

	ThemeBaker = NewObject<UDungeonCanvasMaterialThemeBaker>(this);
	ThemeBaker->ThemeOverride.Set(CanvasThemeOverride);
	ThemeBaker->DrawSettings.Set(DrawSettings);
	ThemeBaker->DungeonCanvas.Set(DungeonCanvasComponent.Get());
	ThemeBaker->DungeonCamera.Set(Camera);

	if (MinimapMaterialTemplate) {
		MinimapMaterialInstance = UMaterialInstanceDynamic::Create(MinimapMaterialTemplate, this);
		if (CanvasImageWidget) {
			FSlateBrush SlateBrush = CanvasImageWidget->GetBrush();
			SlateBrush.DrawAs = ESlateBrushDrawType::Image;
			SlateBrush.Tiling = ESlateBrushTileType::NoTile;
			SlateBrush.SetResourceObject(MinimapMaterialInstance);
			CanvasImageWidget->SetBrush(SlateBrush);
		}
	}
	
}

void UDungeonCanvasWidgetBase::NativeDestruct() {
	Super::NativeDestruct();

	UnbindFromCanvas();

	if (ThemeBaker) {
		ThemeBaker->ReleaseResources();
		ThemeBaker = nullptr;
	}
}

void UDungeonCanvasWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime) {
	Super::NativeTick(MyGeometry, InDeltaTime);

	FIntPoint CurrentWidgetSize = (MyGeometry.GetDrawSize() * CanvasTextureScaleMultiplier).IntPoint();
	CurrentWidgetSize.X = FMath::Max(CurrentWidgetSize.X, 1);
	CurrentWidgetSize.Y = FMath::Max(CurrentWidgetSize.Y, 1);

	if (ThemeBaker) {
		ThemeBaker->Tick(CurrentWidgetSize);
		if (MinimapMaterialInstance) {
			MinimapMaterialInstance->SetTextureParameterValue(TEXT("RenderTexture"), ThemeBaker->GetRenderedTexture());
			if (DungeonCanvasComponent.IsValid() && Camera) {
				DungeonCanvasComponent->SetupMaterialParameters(MinimapMaterialInstance, ThemeBaker->GetCachedViewTransform(), Camera->FloorIndex);
			}
		}
	}

}

FReply UDungeonCanvasWidgetBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	if (Camera) {
		if (Camera->OnMouseButtonDown(InGeometry, InMouseEvent)) {
			return FReply::Handled();
		}
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UDungeonCanvasWidgetBase::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	if (Camera) {
		if (Camera->OnMouseButtonUp(InGeometry, InMouseEvent)) {
			return FReply::Handled();
		}
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UDungeonCanvasWidgetBase::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	if (Camera) {
		if (Camera->OnMouseMove(InGeometry, InMouseEvent)) {
			return FReply::Handled();
		}
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UDungeonCanvasWidgetBase::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) {
	if (Camera) {
		if (Camera->OnMouseWheel(InGeometry, InMouseEvent)) {
			return FReply::Handled();
		}
	}
	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

void UDungeonCanvasWidgetBase::NativeOnMouseLeave(const FPointerEvent& InMouseEvent) {
	Super::NativeOnMouseLeave(InMouseEvent);
	if (Camera) {
		Camera->OnMouseLeave(InMouseEvent);
	}
}

void UDungeonCanvasWidgetBase::SetCanvasComponent(UDungeonCanvasComponent* InCanvasComponent) {
	UnbindFromCanvas();
	DungeonCanvasComponent = InCanvasComponent;
	BindToCanvas();
	if (ThemeBaker) {
		ThemeBaker->DungeonCanvas.Set(InCanvasComponent);
		ThemeBaker->ReleaseResources();
	}
}

void UDungeonCanvasWidgetBase::OnCanvasRebuildRequested(UDungeonCanvasComponent* Canvas) {
	if (Camera) {
		Camera->Initialize(Canvas);
	}
}

void UDungeonCanvasWidgetBase::BindToCanvas() {
	if (DungeonCanvasComponent.IsValid()) {
		DungeonCanvasComponent->OnCanvasRebuildRequested.AddDynamic(this, &UDungeonCanvasWidgetBase::OnCanvasRebuildRequested);
	}
}

void UDungeonCanvasWidgetBase::UnbindFromCanvas() {
	if (DungeonCanvasComponent.IsValid()) {
		DungeonCanvasComponent->OnCanvasRebuildRequested.RemoveDynamic(this, &UDungeonCanvasWidgetBase::OnCanvasRebuildRequested);
	}
}

#if WITH_EDITOR
void UDungeonCanvasWidgetBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (FProperty* Property = PropertyChangedEvent.Property) {
		const FName PropertyName = Property->GetFName();
		
	}
}

#endif // WITH_EDITOR

