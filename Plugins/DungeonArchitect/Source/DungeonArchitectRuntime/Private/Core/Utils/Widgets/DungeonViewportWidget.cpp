//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Utils/Widgets/DungeonViewportWidget.h"

#include "CanvasTypes.h"
#include "Components/LineBatchComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/LocalPlayer.h"
#include "EngineModule.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "LegacyScreenPercentageDriver.h"
#include "Misc/App.h"
#include "PreviewScene.h"
#include "SceneInterface.h"
#include "Slate/SceneViewport.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SViewport.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogDAViewport, Log, All);

#define LOCTEXT_NAMESPACE "DungeonViewportWidget"

namespace FocusConstants
{
	const float TransitionTime = 0.25f;
}

void UDungeonPreviewScene::CreateScene() {
	FPreviewScene::ConstructionValues CSV;
	CSV.bEditor = false;
	CSV.bShouldSimulatePhysics = false;
	CSV.bAllowAudioPlayback = false;
	
	PreviewScene = MakeShareable(new FPreviewScene(CSV));
}

UDungeonPreviewScene* UDungeonPreviewScene::CreateDungeonPreviewScene() {
	UDungeonPreviewScene* Scene = NewObject<UDungeonPreviewScene>();
	Scene->CreateScene();
	return Scene;
}

UWorld* UDungeonPreviewScene::GetSceneWorld() const {
	return PreviewScene.IsValid() ? PreviewScene->GetWorld() : nullptr;
}

void UDungeonPreviewScene::SetupSky(UTextureCube* Cubemap) const {
	if (PreviewScene.IsValid()) {
		ASkyAtmosphere* SkyAtmosphere = PreviewScene->GetWorld()->SpawnActor<ASkyAtmosphere>();
		PreviewScene->SkyLight->bRealTimeCapture = true;		// TODO: Check if this is needed
		PreviewScene->SkyLight->SourceType = SLS_SpecifiedCubemap;
		PreviewScene->SkyLight->SetCubemap(Cubemap);
		PreviewScene->SkyLight->RecaptureSky();
	}
}

AActor* UDungeonPreviewScene::Spawn(TSubclassOf<AActor> ActorClass)
{
	if (PreviewScene.IsValid()) {
		UWorld* World = PreviewScene->GetWorld();
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		return World->SpawnActor<AActor>(ActorClass, FVector(0, 0, 0), FRotator(), SpawnParameters);
	}

	// TODO UMG Report spawning actor error before the world is ready.

	return nullptr;
}

FDAViewportCameraTransform::FDAViewportCameraTransform()
	: TransitionStartTime(0)
	, ViewLocation(FVector::ZeroVector)
	, ViewRotation(FRotator::ZeroRotator)
	, DesiredLocation(FVector::ZeroVector)
	, LookAt(FVector::ZeroVector)
	, StartLocation(FVector::ZeroVector)
	, OrthoZoom(DEFAULT_ORTHOZOOM)
{}

void FDAViewportCameraTransform::SetLocation(const FVector& Position)
{
	ViewLocation = Position;
	DesiredLocation = ViewLocation;
}

void FDAViewportCameraTransform::TransitionToLocation(const FVector& InDesiredLocation, bool bInstant)
{
	if ( bInstant )
	{
		SetLocation(InDesiredLocation);
		TransitionStartTime = FSlateApplication::Get().GetCurrentTime() - FocusConstants::TransitionTime;
	}
	else
	{
		DesiredLocation = InDesiredLocation;
		StartLocation = ViewLocation;

		TransitionStartTime = FSlateApplication::Get().GetCurrentTime();
	}
}

bool FDAViewportCameraTransform::UpdateTransition()
{
	bool bIsAnimating = false;
	double TransitionProgress = FMath::Clamp(( FSlateApplication::Get().GetCurrentTime() - TransitionStartTime ) / FocusConstants::TransitionTime, 0.0, 1.0);
	if (TransitionProgress < 1.0 || ViewLocation != DesiredLocation)
	{
		const float Offset = (float)TransitionProgress - 1.0f;
		float LerpWeight = Offset * Offset * Offset + 1.0f;

		if (LerpWeight == 1.0f)
		{
			// Failsafe for the value not being exact on lerps
			ViewLocation = DesiredLocation;
		}
		else
		{
			ViewLocation = FMath::Lerp(StartLocation, DesiredLocation, LerpWeight);
		}


		bIsAnimating = true;
	}

	return bIsAnimating;
}

FMatrix FDAViewportCameraTransform::ComputeOrbitMatrix() const
{
	FTransform Transform =
		FTransform(-LookAt) *
		FTransform(FRotator(0, ViewRotation.Yaw, 0)) *
		FTransform(FRotator(0, 0, ViewRotation.Pitch)) *
		FTransform(FVector(0, ( ViewLocation - LookAt ).Size(), 0));

	return Transform.ToMatrixNoScale() * FInverseRotationMatrix(FRotator(0, 90.f, 0));
}

FDAViewportClient::FDAViewportClient(TWeakObjectPtr<UDungeonPreviewScene> InDungeonPreviewScene, TWeakObjectPtr<APlayerController> InPlayerController)
	: DungeonPreviewScene(InDungeonPreviewScene)
	, Viewport(nullptr)
	, PlayerController(InPlayerController)
	, EngineShowFlags(ESFIM_Game)
{
	FSceneInterface* Scene = GetScene();
	ViewState.Allocate(Scene ? Scene->GetFeatureLevel() : GMaxRHIFeatureLevel);

	BackgroundColor = FColor(55, 55, 55);
}

FDAViewportClient::~FDAViewportClient()
{
}

void FDAViewportClient::SetPreviewScene(TWeakObjectPtr<UDungeonPreviewScene> InDungeonPreviewScene) {
	DungeonPreviewScene = InDungeonPreviewScene;
}

void FDAViewportClient::Tick(float InDeltaTime)
{
	if ( !GIntraFrameDebuggingGameThread )
	{
		// Begin Play
		const TSharedPtr<FPreviewScene, ESPMode::ThreadSafe> PreviewScene = DungeonPreviewScene.IsValid() ? DungeonPreviewScene->GetPreviewScene() : nullptr;
		if (PreviewScene.IsValid()) {
			UWorld* PreviewWorld = PreviewScene->GetWorld();
			if ( !PreviewWorld->HasBegunPlay())
			{
				for ( FActorIterator It(PreviewWorld); It; ++It )
				{
					It->DispatchBeginPlay();
				}
				PreviewWorld->SetBegunPlay(true);
			}

			// Tick
			PreviewWorld->Tick(LEVELTICK_All, InDeltaTime);
		
			CameraHandler->Tick(PreviewWorld, InDeltaTime);
		}
	}

}

void FDAViewportClient::Draw(FViewport* InViewport, FCanvas* Canvas)
{
	FViewport* ViewportBackup = Viewport;
	Viewport = InViewport ? InViewport : Viewport;

	const bool bIsRealTime = true;

	UWorld* World = GetWorld();
	FGameTime Time;
	if ( bIsRealTime || GetScene() != World->Scene )
	{
		Time = FGameTime::GetTimeSinceAppStart();
	}
	else
	{
		Time = World->GetTime();
	}

	// Setup a FSceneViewFamily/FSceneView for the viewport.
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		Canvas->GetRenderTarget(),
		GetScene(),
		EngineShowFlags)
		.SetTime(Time)
		.SetRealtimeUpdate(bIsRealTime));

	// Get DPI derived view fraction.
	float GlobalResolutionFraction = GetDPIDerivedResolutionFraction();

	// Force screen percentage show flag for High DPI.
	ViewFamily.EngineShowFlags.ScreenPercentage = true;

	//UpdateLightingShowFlags(ViewFamily.EngineShowFlags);

	//ViewFamily.ExposureSettings = ExposureSettings;

	//ViewFamily.LandscapeLODOverride = LandscapeLODOverride;

	FSceneView* View = CalcSceneView(&ViewFamily);

	ViewFamily.SetScreenPercentageInterface(new FLegacyScreenPercentageDriver(
		ViewFamily, GlobalResolutionFraction));

	//SetupViewForRendering(ViewFamily, *View);

	FSlateRect SafeFrame;
	View->CameraConstrainedViewRect = View->UnscaledViewRect;
	//if ( CalculateEditorConstrainedViewRect(SafeFrame, Viewport) )
	//{
	//	View->CameraConstrainedViewRect = FIntRect(SafeFrame.Left, SafeFrame.Top, SafeFrame.Right, SafeFrame.Bottom);
	//}

	if ( IsAspectRatioConstrained() )
	{
		// Clear the background to black if the aspect ratio is constrained, as the scene view won't write to all pixels.
		Canvas->Clear(FLinearColor::Black);
	}

	Canvas->Clear(BackgroundColor);

	// workaround for hacky renderer code that uses GFrameNumber to decide whether to resize render targets
	--GFrameNumber;
	GetRendererModule().BeginRenderingViewFamily(Canvas, &ViewFamily);

	// Remove temporary debug lines.
	// Possibly a hack. Lines may get added without the scene being rendered etc.
	ULineBatchComponent* LineBatcher = World->GetLineBatcher(UWorld::ELineBatcherType::World);
	if ( LineBatcher != NULL && ( LineBatcher->BatchedLines.Num() || LineBatcher->BatchedPoints.Num() ) )
	{
		LineBatcher->Flush();
	}

	ULineBatchComponent* ForegroundLineBatcher = World->GetLineBatcher(UWorld::ELineBatcherType::Foreground);
	if ( ForegroundLineBatcher != NULL && ( ForegroundLineBatcher->BatchedLines.Num() || ForegroundLineBatcher->BatchedPoints.Num() ) )
	{
		ForegroundLineBatcher->Flush();
	}
	
	Viewport = ViewportBackup;
}

bool FDAViewportClient::InputKey(const FInputKeyEventArgs& EventArgs) {
	const TSharedPtr<IDAViewportClientInputListener> InputListenerPtr = InputListener.Pin();
	if (InputListenerPtr.IsValid()) {
		if (InputListenerPtr->InputKey(EventArgs)) {
			return true;
		}
	}
	
	return FCommonViewportClient::InputKey(EventArgs);
}

FSceneInterface* FDAViewportClient::GetScene() const
{
	UWorld* World = GetWorld();
	if ( World )
	{
		return World->Scene;
	}

	return NULL;
}

UWorld* FDAViewportClient::GetWorld() const
{
	UWorld* OutWorldPtr = nullptr;
	// If we have a valid scene get its world
	const TSharedPtr<FPreviewScene, ESPMode::ThreadSafe> PreviewScene = DungeonPreviewScene.IsValid() ? DungeonPreviewScene->GetPreviewScene() : nullptr;
	if (PreviewScene.IsValid())
	{
		OutWorldPtr = PreviewScene->GetWorld();
	}
	if ( OutWorldPtr == nullptr )
	{
		OutWorldPtr = GWorld;
	}
	return OutWorldPtr;
}

bool FDAViewportClient::IsAspectRatioConstrained() const
{
	return ViewInfo.bConstrainAspectRatio;
}

void FDAViewportClient::SetBackgroundColor(FLinearColor InBackgroundColor)
{
	BackgroundColor = InBackgroundColor;
}

FLinearColor FDAViewportClient::GetBackgroundColor() const
{
	return BackgroundColor;
}

float FDAViewportClient::GetOrthoUnitsPerPixel(const FViewport* InViewport) const
{
	const float SizeX = InViewport->GetSizeXY().X;

	// 15.0f was coming from the CAMERA_ZOOM_DIV marco, seems it was chosen arbitrarily
	return ( GetOrthoZoom() / ( SizeX * 15.f ) )/* * ComputeOrthoZoomFactor(SizeX)*/;
}


class FDungeonViewportCameraHandlerPlayerCharacter : public IDungeonViewportCameraHandler {
public:
	explicit FDungeonViewportCameraHandlerPlayerCharacter(FDAViewportCameraTransform& InViewTransform, const FVector& InViewOffset, TWeakObjectPtr<APlayerController> InPlayerController)
		: IDungeonViewportCameraHandler(InViewTransform)
		, ViewOffset(InViewOffset)
		, PlayerController(InPlayerController)
	{
	}

	virtual void Tick(UWorld* InWorld, float InDeltaTime) override {
		const APlayerController* PC = PlayerController.Get();
		if (const APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr) {
			const FVector TargetLoc = PlayerPawn->GetActorLocation();
			const FVector CameraLoc = TargetLoc + ViewOffset;

			ViewTransform.SetLocation(CameraLoc);
			ViewTransform.SetRotation(UKismetMathLibrary::FindLookAtRotation(CameraLoc, TargetLoc));
		}
	}
	
private:
	FVector ViewOffset;
	TWeakObjectPtr<APlayerController> PlayerController;
};

void FDAViewportClient::SetPlayerCameraTracking(const FVector& InDefaultOffset) {
	CameraHandler = MakeShareable(new FDungeonViewportCameraHandlerPlayerCharacter(ViewTransform, InDefaultOffset, PlayerController));
	SetInputListener(nullptr);
}

class FDungeonViewportCameraHandlerFreeForm : public IDungeonViewportCameraHandler {
public:
	explicit FDungeonViewportCameraHandlerFreeForm(FDAViewportCameraTransform& InViewTransform, const FVector& InViewOffset,
			const FVector& InLookAt, float InMovementSpeed, const FDungeonViewportKeyBindings& InKeyBindings,
			const TFunction<void()>& InFnGoBack)
		: IDungeonViewportCameraHandler(InViewTransform)
		, ViewOffset(InViewOffset)
		, LookAt(InLookAt)
		, KeyBindings(InKeyBindings)
		, MovementSpeed(InMovementSpeed)
		, FnGoBack(InFnGoBack)
	{
	}

	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override {
		bool bHandled{};
		if (KeyBindings.MovementLeft.Contains(EventArgs.Key)) {
			bKeyPressedLeft = (EventArgs.Event != IE_Released);
			bHandled = true;
		}
		if (KeyBindings.MovementRight.Contains(EventArgs.Key)) {
			bKeyPressedRight = (EventArgs.Event != IE_Released);
			bHandled = true;
		}
		if (KeyBindings.MovementUp.Contains(EventArgs.Key)) {
			bKeyPressedUp = (EventArgs.Event != IE_Released);
			bHandled = true;
		}
		if (KeyBindings.MovementDown.Contains(EventArgs.Key)) {
			bKeyPressedDown = (EventArgs.Event != IE_Released);
			bHandled = true;
		}
		if (KeyBindings.ZoomIn.Contains(EventArgs.Key)) {
			ZoomIn();
			bHandled = true;
		}
		if (KeyBindings.ZoomOut.Contains(EventArgs.Key)) {
			ZoomOut();
			bHandled = true;
		}
		if (KeyBindings.BackButton.Contains(EventArgs.Key)) {
			GoBack();
			bHandled = true;
		}
		return bHandled;
	}

	virtual void Tick(UWorld* InWorld, float InDeltaTime) override {
		FVector Offset = FVector::ZeroVector;
		if (bKeyPressedLeft) {
			Offset.X -= 1;
		}
		if (bKeyPressedRight) {
			Offset.X += 1;
		}

		if (bKeyPressedUp) {
			Offset.Y += 1;
		}
		if (bKeyPressedDown) {
			Offset.Y -= 1;
		}

		FVector ForwardVector = ViewTransform.GetRotation().Vector();
		ForwardVector.Z = 0;
		ForwardVector.Normalize();

		Offset = FQuat::FindBetweenNormals(FVector(0, 1, 0), ForwardVector).RotateVector(Offset);
		const float Distance = InDeltaTime * MovementSpeed;
		LookAt += Offset * Distance;

		const FVector CameraLocation = LookAt + ViewOffset;
		ViewTransform.SetLocation(CameraLocation);
		ViewTransform.SetRotation(UKismetMathLibrary::FindLookAtRotation(CameraLocation, LookAt));
	}
	
	void GoBack() const {
		FnGoBack();
	}

	void ZoomIn() {
		
	}

	void ZoomOut() {
		
	}

private:
	FVector ViewOffset;
	FVector LookAt;
	FDungeonViewportKeyBindings KeyBindings;
	float MovementSpeed;
	TFunction<void()> FnGoBack;

	bool bKeyPressedLeft{};
	bool bKeyPressedRight{};
	bool bKeyPressedUp{};
	bool bKeyPressedDown{};
};

void FDAViewportClient::SetFreeFormCameraTracking(const FVector& InLocation, const FVector& InLookAt, float InMovementSpeed,
		const FDungeonViewportKeyBindings& KeyBindings, const TFunction<void()>& InFnGoBack) {
	CameraHandler = MakeShareable(new FDungeonViewportCameraHandlerFreeForm(ViewTransform, InLocation, InLookAt, InMovementSpeed, KeyBindings, InFnGoBack));
	SetInputListener(CameraHandler);
}


FSceneView* FDAViewportClient::CalcSceneView(FSceneViewFamily* ViewFamily)
{
	FSceneViewInitOptions ViewInitOptions;

	const FVector& ViewLocation = GetViewLocation();
	const FRotator& ViewRotation = GetViewRotation();

	const FIntPoint ViewportSizeXY = Viewport->GetSizeXY();

	FIntRect ViewRect = FIntRect(0, 0, ViewportSizeXY.X, ViewportSizeXY.Y);
	ViewInitOptions.SetViewRectangle(ViewRect);

	ViewInitOptions.ViewOrigin = ViewLocation;

	ViewInitOptions.ViewRotationMatrix = FInverseRotationMatrix(ViewRotation);
	ViewInitOptions.ViewRotationMatrix = ViewInitOptions.ViewRotationMatrix * FMatrix(
		FPlane(0, 0, 1, 0),
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, 0, 1));

	//@TODO: Should probably be locally configurable (or just made into a FMinimalViewInfo property)
	const EAspectRatioAxisConstraint AspectRatioAxisConstraint = GetDefault<ULocalPlayer>()->AspectRatioAxisConstraint;

	FMinimalViewInfo::CalculateProjectionMatrixGivenView(ViewInfo, AspectRatioAxisConstraint, Viewport, /*inout*/ ViewInitOptions);

	ViewInitOptions.ViewFamily = ViewFamily;
	ViewInitOptions.SceneViewStateInterface = ViewState.GetReference();
	ViewInitOptions.ViewElementDrawer = this;

	ViewInitOptions.BackgroundColor = GetBackgroundColor();

	//ViewInitOptions.EditorViewBitflag = 0, // send the bit for this view - each actor will check it's visibility bits against this

	// for ortho views to steal perspective view origin
	//ViewInitOptions.OverrideLODViewOrigin = FVector::ZeroVector;
	//ViewInitOptions.bUseFauxOrthoViewPos = true;

	//ViewInitOptions.CursorPos = CurrentMousePos;

	FSceneView* View = new FSceneView(ViewInitOptions);

	ViewFamily->Views.Add(View);

	View->StartFinalPostprocessSettings(ViewLocation);

	//OverridePostProcessSettings(*View);

	View->EndFinalPostprocessSettings(ViewInitOptions);

	return View;
}

/////////////////////////////////////////////////////
// SDAAutoRefreshViewport

class SDAAutoRefreshViewport : public SViewport
{
	SLATE_BEGIN_ARGS(SDAAutoRefreshViewport) {}
	SLATE_ARGUMENT(TWeakObjectPtr<UDungeonPreviewScene>, DungeonPreviewScene)
	SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, PlayerController)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		DungeonPreviewScene = InArgs._DungeonPreviewScene;
		PlayerController = InArgs._PlayerController;
		SViewport::FArguments ParentArgs;
		ParentArgs.IgnoreTextureAlpha(false);
		ParentArgs.EnableBlending(true);
		//ParentArgs.RenderDirectlyToWindow(true);
		SViewport::Construct(ParentArgs);

		ViewportClient = MakeShareable(new FDAViewportClient(DungeonPreviewScene, PlayerController));
		Viewport = MakeShareable(new FSceneViewport(ViewportClient.Get(), SharedThis(this)));

		// The viewport widget needs an interface so it knows what should render
		SetViewportInterface(Viewport.ToSharedRef());
		SetCanTick(true);
	}

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override
	{
		Viewport->Invalidate();
		//Viewport->InvalidateDisplay();

		Viewport->Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
		ViewportClient->Tick(InDeltaTime);
	}

	void SetPreviewScene(TWeakObjectPtr<UDungeonPreviewScene> InDungeonPreviewScene) {
		DungeonPreviewScene = InDungeonPreviewScene;
		if (ViewportClient.IsValid()) {
			ViewportClient->SetPreviewScene(DungeonPreviewScene);
		}
	}
	
public:
	TSharedPtr<FDAViewportClient> ViewportClient;
	
	TSharedPtr<FSceneViewport> Viewport;

	/** preview scene */
	TWeakObjectPtr<UDungeonPreviewScene> DungeonPreviewScene;
	TWeakObjectPtr<APlayerController> PlayerController;
};

FReply SDAAutoRefreshViewport::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) {
	UE_LOG(LogDAViewport, Log, TEXT("DA Viewport: Key Down"));
	return SViewport::OnKeyDown(MyGeometry, InKeyEvent);
}

FReply SDAAutoRefreshViewport::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) {
	UE_LOG(LogDAViewport, Log, TEXT("DA Viewport: Key Up"));
	return SViewport::OnKeyUp(MyGeometry, InKeyEvent);
}


/////////////////////////////////////////////////////
// UViewport

UDungeonViewportWidget::UDungeonViewportWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ShowFlags(ESFIM_Game)
{
	bIsVariable = true;
PRAGMA_DISABLE_DEPRECATION_WARNINGS
	BackgroundColor = FLinearColor::Black;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	ShowFlags.DisableAdvancedFeatures();
	//ParentArgs.IgnoreTextureAlpha(false);
	//ParentArgs.EnableBlending(true);
	////ParentArgs.RenderDirectlyToWindow(true);
}

void UDungeonViewportWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	ViewportWidget.Reset();
}

TSharedRef<SWidget> UDungeonViewportWidget::RebuildWidget()
{
	if ( IsDesignTime() )
	{
		return SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Viewport", "Viewport"))
			];
	}
	else
	{
		ViewportWidget = SNew(SDAAutoRefreshViewport)
			.DungeonPreviewScene(DungeonPreviewScene)
			.PlayerController(GetOwningPlayer());
		if (CameraTrackMode == EDungeonViewportCameraTrackMode::FreeMovement) {
			ViewportWidget->ViewportClient->SetFreeFormCameraTracking(DefaultLookAt + DefaultOffset, DefaultLookAt, FreeFormCameraMovementSpeed,
					FreeFormCameraInputBindings, [this]() {
						if (OnFreeModeCameraBackPressed.IsBound()) {
							OnFreeModeCameraBackPressed.Broadcast();
						}
					});
		}
		else if (CameraTrackMode == EDungeonViewportCameraTrackMode::TrackPlayerCharacter) {
			ViewportWidget->ViewportClient->SetPlayerCameraTracking(DefaultOffset);
		}

		if ( GetChildrenCount() > 0 )
		{
			ViewportWidget->SetContent(GetContentSlot()->Content ? GetContentSlot()->Content->TakeWidget() : SNullWidget::NullWidget);
		}

		// Set focus to the search box on creation
		FSlateApplication::Get().SetKeyboardFocus(ViewportWidget);
		FSlateApplication::Get().SetUserFocus(0, ViewportWidget);
		
		return ViewportWidget.ToSharedRef();
	}
}

void UDungeonViewportWidget::SwitchCameraMode(EDungeonViewportCameraTrackMode NewCameraMode) {
	CameraTrackMode = NewCameraMode;

	if (ViewportWidget.IsValid() && ViewportWidget->ViewportClient.IsValid()) {
		const FVector ViewLocation = ViewportWidget->ViewportClient->GetViewLocation();
		const FRotator ViewRotation = ViewportWidget->ViewportClient->GetViewRotation();
	
		if (CameraTrackMode == EDungeonViewportCameraTrackMode::FreeMovement) {
			const FVector NewLookAt = ViewLocation - ViewRotation.Vector() * DefaultOffset.Size(); 
			ViewportWidget->ViewportClient->SetFreeFormCameraTracking(ViewLocation, DefaultLookAt, FreeFormCameraMovementSpeed,
					FreeFormCameraInputBindings, [this]() {
						if (OnFreeModeCameraBackPressed.IsBound()) {
							OnFreeModeCameraBackPressed.Broadcast();
						}
					});
		}
		else if (CameraTrackMode == EDungeonViewportCameraTrackMode::TrackPlayerCharacter) {
			ViewportWidget->ViewportClient->SetPlayerCameraTracking(DefaultOffset);
		}
	}
}

void UDungeonViewportWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if ( ViewportWidget.IsValid() )
	{
		check(ViewportWidget->ViewportClient.IsValid());
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		ViewportWidget->ViewportClient->SetBackgroundColor(BackgroundColor);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
		ViewportWidget->ViewportClient->SetEngineShowFlags(ShowFlags);
	}
}

void UDungeonViewportWidget::OnSlotAdded(UPanelSlot* InSlot)
{
	// Add the child to the live canvas if it already exists
	if ( ViewportWidget.IsValid() )
	{
		ViewportWidget->SetContent(InSlot->Content ? InSlot->Content->TakeWidget() : SNullWidget::NullWidget);
	}
}

void UDungeonViewportWidget::OnSlotRemoved(UPanelSlot* InSlot)
{
	// Remove the widget from the live slot if it exists.
	if ( ViewportWidget.IsValid() )
	{
		ViewportWidget->SetContent(SNullWidget::NullWidget);
	}
}

void UDungeonViewportWidget::SetPreviewScene(UDungeonPreviewScene* PreviewScene) {
	DungeonPreviewScene = PreviewScene;
	if (ViewportWidget.IsValid()) {
		ViewportWidget->SetPreviewScene(DungeonPreviewScene);
	}
}

FVector UDungeonViewportWidget::GetViewLocation() const
{
	if ( ViewportWidget.IsValid() )
	{
		return ViewportWidget->ViewportClient->GetViewLocation();
	}

	return FVector();
}

void UDungeonViewportWidget::SetViewLocation(FVector Vector)
{
	if ( ViewportWidget.IsValid() )
	{
		ViewportWidget->ViewportClient->SetViewLocation(Vector);
	}
}

FRotator UDungeonViewportWidget::GetViewRotation() const
{
	if ( ViewportWidget.IsValid() )
	{
		return ViewportWidget->ViewportClient->GetViewRotation();
	}

	return FRotator();
}

void UDungeonViewportWidget::SetViewRotation(FRotator Rotator)
{
	if ( ViewportWidget.IsValid() )
	{
		ViewportWidget->ViewportClient->SetViewRotation(Rotator);
	}
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS

const FLinearColor& UDungeonViewportWidget::GetBackgroundColor() const
{
	return BackgroundColor;
}

void UDungeonViewportWidget::SetBackgroundColor(const FLinearColor& InColor)
{
	BackgroundColor = InColor;
	if (ViewportWidget.IsValid())
	{
		check(ViewportWidget->ViewportClient.IsValid());
		ViewportWidget->ViewportClient->SetBackgroundColor(BackgroundColor);
	}
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

#if WITH_EDITOR

const FText UDungeonViewportWidget::GetPaletteCategory()
{
	return LOCTEXT("Primitive", "Primitive");
}

#endif

/////////////////////////////////////////////////////


#undef LOCTEXT_NAMESPACE

