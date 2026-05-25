//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Camera/CameraTypes.h"
#include "Components/ContentWidget.h"
#include "GameFramework/Actor.h"
#include "SceneManagement.h"
#include "SceneTypes.h"
#include "ShowFlags.h"
#include "Templates/SubclassOf.h"
#include "UObject/ObjectMacros.h"
#include "UnrealClient.h"
#include "ViewportClient.h"
#include "Widgets/SWidget.h"
#include "DungeonViewportWidget.generated.h"

class FCanvas;
class FPreviewScene;
class SAutoRefreshViewport;


UCLASS(Blueprintable)
class UDungeonPreviewScene : public UObject {
	GENERATED_BODY()
public:

	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	static UDungeonPreviewScene* CreateDungeonPreviewScene();
	
	UFUNCTION(BlueprintPure, Category="Dungeon Architect")
	UWorld* GetSceneWorld() const;

	UFUNCTION(BlueprintCallable, Category="Viewport")
	void SetupSky(UTextureCube* Cubemap) const;

	UFUNCTION(BlueprintCallable, Category="Viewport")
	AActor* Spawn(TSubclassOf<AActor> ActorClass);

	TSharedPtr<FPreviewScene, ESPMode::ThreadSafe> GetPreviewScene() const { return PreviewScene; }

private:
	void CreateScene();
	
private:
	TSharedPtr<FPreviewScene, ESPMode::ThreadSafe> PreviewScene;
};


/**
* Stores the transformation data for the viewport camera
*/
struct FDAViewportCameraTransform
{
public:
	FDAViewportCameraTransform();

	/** Sets the transform's location */
	void SetLocation(const FVector& Position);

	/** Sets the transform's rotation */
	void SetRotation(const FRotator& Rotation)
	{
		ViewRotation = Rotation;
	}

	/** Sets the location to look at during orbit */
	void SetLookAt(const FVector& InLookAt)
	{
		LookAt = InLookAt;
	}

	/** Set the ortho zoom amount */
	void SetOrthoZoom(float InOrthoZoom)
	{
		OrthoZoom = InOrthoZoom;
	}


	/** @return The transform's location */
	FORCEINLINE const FVector& GetLocation() const { return ViewLocation; }

	/** @return The transform's rotation */
	FORCEINLINE const FRotator& GetRotation() const { return ViewRotation; }

	/** @return The look at point for orbiting */
	FORCEINLINE const FVector& GetLookAt() const { return LookAt; }

	/** @return The ortho zoom amount */
	FORCEINLINE float GetOrthoZoom() const { return OrthoZoom; }

	/**
	* Animates from the current location to the desired location
	*
	* @param InDesiredLocation	The location to transition to
	* @param bInstant			If the desired location should be set instantly rather than transitioned to over time
	*/
	void TransitionToLocation(const FVector& InDesiredLocation, bool bInstant);

	/**
	* Updates any current location transitions
	*
	* @return true if there is currently a transition
	*/
	bool UpdateTransition();

	/**
	* Computes a matrix to use for viewport location and rotation when orbiting
	*/
	FMatrix ComputeOrbitMatrix() const;
private:
	/** The time when a transition to the desired location began */
	double TransitionStartTime;
	/** Current viewport Position. */
	FVector	ViewLocation;
	/** Current Viewport orientation; valid only for perspective projections. */
	FRotator ViewRotation;
	/** Desired viewport location when animating between two locations */
	FVector	DesiredLocation;
	/** When orbiting, the point we are looking at */
	FVector LookAt;
	/** Viewport start location when animating to another location */
	FVector StartLocation;
	/** Ortho zoom amount */
	float OrthoZoom;
};


class IDAViewportClientInputListener {
public:
	virtual ~IDAViewportClientInputListener() = default;
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) = 0;
};

class IDungeonViewportCameraHandler : public IDAViewportClientInputListener {
public:
	explicit IDungeonViewportCameraHandler(FDAViewportCameraTransform& InViewTransform)
		: ViewTransform(InViewTransform)
	{
	}
	virtual ~IDungeonViewportCameraHandler() override = default;
	virtual void Tick(UWorld* InWorld, float InDeltaTime) {}
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override { return false; }

protected:
	FDAViewportCameraTransform& ViewTransform;
};
typedef TSharedPtr<IDungeonViewportCameraHandler> IDungeonViewportCameraHandlerPtr;


class FDAViewportClient : public FCommonViewportClient, public FViewElementDrawer
{
public:
	FDAViewportClient(TWeakObjectPtr<UDungeonPreviewScene> InDungeonPreviewScene, TWeakObjectPtr<APlayerController> InPlayerController);
	virtual ~FDAViewportClient();
	
	void SetPreviewScene(TWeakObjectPtr<UDungeonPreviewScene> InDungeonPreviewScene);

	using FViewElementDrawer::Draw;

	// FViewportClient interface
	virtual UWorld* GetWorld() const override;
	virtual void Draw(FViewport* InViewport, FCanvas* Canvas) override;
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;

	// FDAViewportClient

	void SetInputListener(const TWeakPtr<IDAViewportClientInputListener>& InInputListener) { InputListener = InInputListener; }
	
	virtual void Tick(float InDeltaTime);
	virtual FSceneView* CalcSceneView(FSceneViewFamily* ViewFamily);
	
	/**
	 * @return The scene being rendered in this viewport
	 */
	virtual FSceneInterface* GetScene() const;


	bool IsAspectRatioConstrained() const;

	void SetBackgroundColor(FLinearColor InBackgroundColor);
	FLinearColor GetBackgroundColor() const;

	/** Sets the location of the viewport's camera */
	void SetViewLocation(const FVector& NewLocation)
	{
		ViewTransform.SetLocation(NewLocation);
	}

	/** Sets the location of the viewport's camera */
	void SetViewRotation(const FRotator& NewRotation)
	{
		ViewTransform.SetRotation(NewRotation);
	}

	/**
	* Sets the look at location of the viewports camera for orbit *
	*
	* @param LookAt The new look at location
	* @param bRecalulateView	If true, will recalculate view location and rotation to look at the new point immediatley
	*/
	void SetLookAtLocation(const FVector& LookAt, bool bRecalculateView = false)
	{
		ViewTransform.SetLookAt(LookAt);

		if ( bRecalculateView )
		{
			FMatrix OrbitMatrix = ViewTransform.ComputeOrbitMatrix();
			OrbitMatrix = OrbitMatrix.InverseFast();

			ViewTransform.SetRotation(OrbitMatrix.Rotator());
			ViewTransform.SetLocation(OrbitMatrix.GetOrigin());
		}
	}

	/** Sets ortho zoom amount */
	void SetOrthoZoom(float InOrthoZoom)
	{
		// A zero ortho zoom is not supported and causes NaN/div0 errors
		check(InOrthoZoom != 0);
		ViewTransform.SetOrthoZoom(InOrthoZoom);
	}

	/** @return the current viewport camera location */
	const FVector& GetViewLocation() const
	{
		return ViewTransform.GetLocation();
	}

	/** @return the current viewport camera rotation */
	const FRotator& GetViewRotation() const
	{
		return ViewTransform.GetRotation();
	}

	/** @return the current look at location */
	const FVector& GetLookAtLocation() const
	{
		return ViewTransform.GetLookAt();
	}

	/** @return the current ortho zoom amount */
	float GetOrthoZoom() const
	{
		return ViewTransform.GetOrthoZoom();
	}

	/** @return The number of units per pixel displayed in this viewport */
	float GetOrthoUnitsPerPixel(const FViewport* Viewport) const;

	void SetEngineShowFlags(FEngineShowFlags InEngineShowFlags)
	{
		EngineShowFlags = InEngineShowFlags;
	}

	void SetPlayerCameraTracking(const FVector& InDefaultOffset);
	void SetFreeFormCameraTracking(const FVector& InLocation, const FVector& InLookAt, float InMovementSpeed,
			const FDungeonViewportKeyBindings& KeyBindings, const TFunction<void()>& InFnGoBack);

protected:

	/** The scene used for the viewport. Owned externally */
	TWeakObjectPtr<UDungeonPreviewScene> DungeonPreviewScene;

	FMinimalViewInfo ViewInfo;

	FLinearColor BackgroundColor;

	/** Viewport camera transform data */
	FDAViewportCameraTransform ViewTransform;

	IDungeonViewportCameraHandlerPtr CameraHandler;
	
	FViewport* Viewport;
	TWeakObjectPtr<APlayerController> PlayerController;

	/** The viewport's scene view state. */
	FSceneViewStateReference ViewState;

	/** A set of flags that determines visibility for various scene elements. */
	FEngineShowFlags EngineShowFlags;

	TWeakPtr<IDAViewportClientInputListener> InputListener;
};

UENUM()
enum class EDungeonViewportCameraTrackMode : uint8 {
	TrackPlayerCharacter,
	FreeMovement
};

USTRUCT(BlueprintType)
struct FDungeonViewportKeyBindings {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="DungeonArchitect")
	TArray<FKey> MovementLeft = { EKeys::Left, EKeys::A };
	
	UPROPERTY(EditAnywhere, Category="DungeonArchitect")
	TArray<FKey> MovementRight = { EKeys::Right, EKeys::D };
	
	UPROPERTY(EditAnywhere, Category="DungeonArchitect")
	TArray<FKey> MovementUp = { EKeys::Up, EKeys::W };
	
	UPROPERTY(EditAnywhere, Category="DungeonArchitect")
	TArray<FKey> MovementDown = { EKeys::Down, EKeys::S };
	
	UPROPERTY(EditAnywhere, Category="DungeonArchitect")
	TArray<FKey> ZoomIn = { EKeys::MouseScrollUp };
	
	UPROPERTY(EditAnywhere, Category="DungeonArchitect")
	TArray<FKey> ZoomOut = { EKeys::MouseScrollDown };

	UPROPERTY(EditAnywhere, Category="DungeonArchitect")
	TArray<FKey> BackButton = { EKeys::Escape };
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDungeonViewportFreeCameraBack);

UCLASS()
class UDungeonViewportWidget : public UContentWidget {
	GENERATED_UCLASS_BODY()
	
public:
	
	UE_DEPRECATED(5.2, "Direct access to BackgroundColor is deprecated. Please use the getter or setter.")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Getter, Setter, Category = Appearance)
	FLinearColor BackgroundColor;

	UPROPERTY(EditAnywhere, Category="DungeonArchitect")
	FVector DefaultOffset = FVector(1000, 1000, 2000);

	UPROPERTY(EditAnywhere, Category="DungeonArchitect")
	FVector DefaultLookAt = FVector(0, 0, 0);

	UPROPERTY(EditAnywhere, Category="DungeonArchitect")
	EDungeonViewportCameraTrackMode CameraTrackMode = EDungeonViewportCameraTrackMode::TrackPlayerCharacter;

	UPROPERTY(EditAnywhere, Category="DungeonArchitect", meta=(EditCondition="CameraTrackMode == EDungeonViewportCameraTrackMode::FreeMovement"))
	FDungeonViewportKeyBindings FreeFormCameraInputBindings;

	UPROPERTY(EditAnywhere, Category="DungeonArchitect")
	float FreeFormCameraMovementSpeed = 1000;
	
	UPROPERTY(BlueprintAssignable, Category = Dungeon)
	FDungeonViewportFreeCameraBack OnFreeModeCameraBackPressed;
	
	UPROPERTY(EditAnywhere, Category="DungeonArchitect")
	FDungeonViewportKeyBindings FreeFormCameraInput;
	
	UFUNCTION(BlueprintCallable, Category="Camera")
	void SetPreviewScene(UDungeonPreviewScene* PreviewScene);
	
	UFUNCTION(BlueprintCallable, Category="Camera")
	FVector GetViewLocation() const;

	UFUNCTION(BlueprintCallable, Category="Camera")
	void SetViewLocation(FVector Location);

	UFUNCTION(BlueprintCallable, Category="Camera")
	FRotator GetViewRotation() const;

	UFUNCTION(BlueprintCallable, Category="Camera")
	void SetViewRotation(FRotator Rotation);


	
	UFUNCTION(BlueprintCallable, Category="Viewport")
	void SwitchCameraMode(EDungeonViewportCameraTrackMode NewCameraMode);
	
	void SetBackgroundColor(const FLinearColor& InColor);

	const FLinearColor& GetBackgroundColor() const;

	// UWidget interface
	virtual void SynchronizeProperties() override;
	// End of UWidget interface

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:

	// UPanelWidget
	virtual void OnSlotAdded(UPanelSlot* Slot) override;
	virtual void OnSlotRemoved(UPanelSlot* Slot) override;
	// End UPanelWidget

protected:
	TSharedPtr<class SDAAutoRefreshViewport> ViewportWidget;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

	/** Show flags for the engine for this viewport */
	FEngineShowFlags ShowFlags;

	UPROPERTY()
	TObjectPtr<UDungeonPreviewScene> DungeonPreviewScene = {};;
};


