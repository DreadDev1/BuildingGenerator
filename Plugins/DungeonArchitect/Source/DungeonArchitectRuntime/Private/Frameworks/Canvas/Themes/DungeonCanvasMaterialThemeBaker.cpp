//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialThemeBaker.h"

#include "Core/Dungeon.h"
#include "Frameworks/Canvas/Cameras/DungeonCanvasCamera.h"
#include "Frameworks/Canvas/DungeonCanvas.h"
#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialTheme.h"

#include "Components/BillboardComponent.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(DungeonCanvasMaterialThemeBaker, Log, All);

void UDungeonCanvasMaterialThemeBaker::ReleaseResources() {
	if (CanvasRenderTexture) {
		CanvasRenderTexture->ReleaseResource();
		CanvasRenderTexture = nullptr;
	}

	CanvasRenderResources = nullptr;
}

void UDungeonCanvasMaterialThemeBaker::Tick(const FIntPoint& InDesiredTextureSize) {
	if (!CanvasRenderTexture) {
		CreateRenderResources(InDesiredTextureSize);
	}
	ResizeTextureIfNeeded(InDesiredTextureSize);
	
	if (CanvasRenderTexture) {
		CanvasRenderTexture->UpdateResource();
	}
}

UCanvasRenderTarget2D* UDungeonCanvasMaterialThemeBaker::GetRenderedTexture() const {
	return CanvasRenderTexture;
}

void UDungeonCanvasMaterialThemeBaker::CanvasDrawCallback(UCanvas* Canvas, int32 Width, int32 Height) {
	if (UDungeonCanvasComponent* DungeonCanvasComponent = DungeonCanvas.Get()) {
		FDungeonCanvasDrawContext DrawContext = CreateDrawContext(Canvas, Width, Height);
		CachedViewTransform = DrawContext.ViewTransform;
		DungeonCanvasComponent->Draw(DrawContext);
	}
}

void UDungeonCanvasMaterialThemeBaker::CreateRenderResources(const FVector2D& InSize) {
	if (!CanvasRenderTexture) {
		CanvasRenderTexture = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(GetWorld(), UCanvasRenderTarget2D::StaticClass(), InSize.X, InSize.Y);
		CanvasRenderTexture->ClearColor = FLinearColor::Black;
		CanvasRenderTexture->SetShouldClearRenderTargetOnReceiveUpdate(true);
		CanvasRenderTexture->OnCanvasRenderTargetUpdate.RemoveAll(this);
		CanvasRenderTexture->OnCanvasRenderTargetUpdate.AddDynamic(this, &UDungeonCanvasMaterialThemeBaker::CanvasDrawCallback);
	}
	if (!CanvasRenderResources) {
		CanvasRenderResources = FDungeonCanvasRenderingLibrary::CreateDungeonCanvasRenderSettings(this, GetTheme());
	}
}

void UDungeonCanvasMaterialThemeBaker::ResizeTextureIfNeeded(const FIntPoint& InDesiredTextureSize) const {
	if (InDesiredTextureSize.X != CanvasRenderTexture->SizeX || InDesiredTextureSize.Y != CanvasRenderTexture->SizeY) {
		CanvasRenderTexture->ResizeTarget(InDesiredTextureSize.X, InDesiredTextureSize.Y);
		UE_LOG(DungeonCanvasMaterialThemeBaker, Log, TEXT("Resizing texture to: (%d, %d)"), InDesiredTextureSize.X, InDesiredTextureSize.Y);
	}
}

FDungeonCanvasDrawContext UDungeonCanvasMaterialThemeBaker::CreateDrawContext(UCanvas* Canvas, int32 Width, int32 Height) const {
	UDungeonCanvasComponent* DungeonCanvasComponent = DungeonCanvas.Get();
	UDungeonCanvasCamera* DungeonCameraPtr = DungeonCamera.Get();
	
	if (!DungeonCanvasComponent) {
		return {};
	}
	
	FDungeonCanvasViewportTransform CameraTransform{};
	UWorld* World = GetWorld();
	int32 FloorIndex = 0;
	if (DungeonCameraPtr && World) {
		const float DeltaSeconds = GetWorld()->DeltaTimeSeconds;
		DungeonCameraPtr->Update(DungeonCanvasComponent, DeltaSeconds);
		CameraTransform = DungeonCameraPtr->GetCameraTransform(FVector2D(Width, Height));
		FloorIndex = DungeonCameraPtr->FloorIndex;
	}
	else {
		// Fallback to focus on the entire dungeon
		CameraTransform = DungeonCanvasComponent->GetFullDungeonViewTransform();
	}

	FDungeonCanvasDrawContext DrawContext;
	DrawContext.CanvasLayout = Canvas;
	DrawContext.ViewTransform = CameraTransform;
	DrawContext.FloorIndex = FloorIndex;
	DrawContext.DrawSettings = DrawSettings.Get();
	DrawContext.ThemeRenderResources = CanvasRenderResources;
	DrawContext.Theme = GetTheme();

	return DrawContext;
}

UDungeonCanvasMaterialTheme* UDungeonCanvasMaterialThemeBaker::GetTheme() const {
	if (UDungeonCanvasMaterialTheme* CanvasThemeOverridePtr = ThemeOverride.Get()) {
		return CanvasThemeOverridePtr;
	}

	UDungeonCanvasComponent* DungeonCanvasComponent = DungeonCanvas.Get();
	return DungeonCanvasComponent ? DungeonCanvasComponent->DefaultMaterialTheme : nullptr;
}

///////////////// UDungeonCanvasMaterialThemeBakerComponent /////////////////
UDungeonCanvasMaterialThemeBakerComponent::UDungeonCanvasMaterialThemeBakerComponent() {
	ThemeBaker = CreateDefaultSubobject<UDungeonCanvasMaterialThemeBaker>("Baker");
	
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	
	DisplayMaterialTemplate = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, TEXT("/DungeonArchitect/Core/Runtime/Features/DungeonCanvas/Materials/HUD/M_DungeonCanvas_Borderless.M_DungeonCanvas_Borderless")));
}

void UDungeonCanvasMaterialThemeBakerComponent::BeginPlay() {
	Super::BeginPlay();

	if (!Dungeon) {
		Dungeon = Cast<ADungeon>(UGameplayStatics::GetActorOfClass(this, ADungeon::StaticClass()));
	}
	UDungeonCanvasComponent* DungeonCanvas{};
	if (Dungeon) {
		DungeonCanvas = Dungeon->GetComponentByClass<UDungeonCanvasComponent>();
	}

	// Setup the camera
	if (Camera) {
		APlayerController* PlayerController = nullptr;
		if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(GetWorld())) {
			const TArray<ULocalPlayer*>& LocalPlayers = GameInstance->GetLocalPlayers();
			for (ULocalPlayer* LocalPlayer : LocalPlayers) {
				if (APlayerController* PC = LocalPlayer->PlayerController) {
					if (PC->IsLocalController()) {
						PlayerController = PC;
						break;
					}
				}
			}
		}
		Camera->PlayerController = PlayerController;
	}
	
	ThemeBaker->ThemeOverride.Set(CanvasThemeOverride);
	ThemeBaker->DrawSettings.Set(DrawSettings);
	ThemeBaker->DungeonCanvas.Set(DungeonCanvas);
	ThemeBaker->DungeonCamera.Set(Camera);

	if (DisplayMaterialTemplate) {
		DisplayMaterialInstance = UMaterialInstanceDynamic::Create(DisplayMaterialTemplate, this);
	}
}

void UDungeonCanvasMaterialThemeBakerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);
	
	if (ThemeBaker) {
		ThemeBaker->ReleaseResources();
		ThemeBaker = nullptr;
	}
}

void UDungeonCanvasMaterialThemeBakerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (TextureSize.X > 0 && TextureSize.Y > 0) {
		if (ThemeBaker) {
			ThemeBaker->Tick(TextureSize);
			if (DisplayMaterialInstance) {
				DisplayMaterialInstance->SetTextureParameterValue(TEXT("RenderTexture"), ThemeBaker->GetRenderedTexture());
			}
		}
	}
}

UCanvasRenderTarget2D* UDungeonCanvasMaterialThemeBakerComponent::GetRenderedTexture() const {
	return ThemeBaker->GetRenderedTexture();
}

UMaterialInstanceDynamic* UDungeonCanvasMaterialThemeBakerComponent::GetRenderedMaterial() const {
	return DisplayMaterialInstance;
}


///////////////// ADungeonCanvasMaterialThemeBakerActor /////////////////
ADungeonCanvasMaterialThemeBakerActor::ADungeonCanvasMaterialThemeBakerActor() {
	bRelevantForLevelBounds = false;
	ThemeBakerComponent = CreateDefaultSubobject<UDungeonCanvasMaterialThemeBakerComponent>("ThemeBaker");
	SetCanBeDamaged(false);
	
#if WITH_EDITORONLY_DATA
	Billboard = CreateDefaultSubobject<UBillboardComponent>("Billboard");
	Billboard->SetupAttachment(RootComponent);
#endif
}



