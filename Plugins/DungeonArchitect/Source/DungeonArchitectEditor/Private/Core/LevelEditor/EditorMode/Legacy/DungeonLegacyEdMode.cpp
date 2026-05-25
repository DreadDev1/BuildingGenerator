//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/Legacy/DungeonLegacyEdMode.h"

#include "Core/Common/Utils/DungeonBuildingNotification.h"
#include "Core/Common/Utils/DungeonEditorUtils.h"
#include "Core/Dungeon.h"
#include "Core/LevelEditor/EditorMode/Legacy/DungeonEdMode/DungeonEdModeHandler.h"
#include "Core/LevelEditor/EditorMode/Legacy/DungeonEdMode/DungeonEdModeHandlerFactory.h"
#include "Core/LevelEditor/EditorMode/Legacy/DungeonLegacyEdModeToolkit.h"

#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "Editor/LevelEditor/Public/SLevelViewport.h"
#include "EditorViewportClient.h"
#include "Engine/Selection.h"
#include "IAssetViewport.h"
#include "Modules/ModuleManager.h"
#include "Toolkits/ToolkitManager.h"

#define LOCTEXT_NAMESPACE "DungeonDrawEdMode"
#define DungeonDraw_SNAP_TRACE (10000.f)
DEFINE_LOG_CATEGORY(DungeonDrawMode);

FEditorModeID FEdModeDungeonLegacy::EM_Dungeon(TEXT("EM_Dungeon"));


const float ROTATION_SPEED = 10;


/** Constructor */
FEdModeDungeonLegacy::FEdModeDungeonLegacy()
    : FEdMode()
      , ModeHandler(nullptr)
      , bToolActive(false) {
}


/** Destructor */
FEdModeDungeonLegacy::~FEdModeDungeonLegacy() {
    // Save UI settings to config file
    FEditorDelegates::MapChange.RemoveAll(this);
}


/** FGCObject interface */
void FEdModeDungeonLegacy::AddReferencedObjects(FReferenceCollector& Collector) {
    // Call parent implementation
    FEdMode::AddReferencedObjects(Collector);
    Collector.AddReferencedObject(ModeHandler);
}

FString FEdModeDungeonLegacy::GetReferencerName() const {
    static const FString NameString = TEXT("FEdModeDungeon");
    return NameString;
}

/** FEdMode: Called when the mode is entered */
void FEdModeDungeonLegacy::Enter() {
    FEdMode::Enter();

    DungeonBuildingNotification = MakeShareable(new FDungeonBuildingNotification);


    // Clear any selection 
    GEditor->SelectNone(false, true);

    // Bind to editor callbacks
    FEditorDelegates::NewCurrentLevel.AddSP(this, &FEdModeDungeonLegacy::NotifyNewCurrentLevel);

    // Force real-time viewports.  We'll back up the current viewport state so we can restore it when the
    // user exits this mode.
    SetRealtimeOverride();

    ADungeon* SelectedDungeon = FDungeonEditorUtils::GetDungeonActorFromLevelViewport();
    RecreateHandler(SelectedDungeon);

    RecreateToolkit();

    GEditor->RegisterForUndo(this);
}

void FEdModeDungeonLegacy::RecreateToolkit() {

    if (Toolkit.IsValid()) {
        FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
        Toolkit.Reset();
    }

    // Recreate the UI widget
    TSharedPtr<SWidget> ToolkitWidget;
    if (ModeHandler) {
        ToolkitWidget = ModeHandler->CreateToolkitWidget();
    }
    else {
        ToolkitWidget = SNullWidget::NullWidget;
    }

    TSharedPtr<FDungeonLegacyEdModeToolkit> DungeonToolkit = MakeShareable(new FDungeonLegacyEdModeToolkit);
    DungeonToolkit->SetInlineContent(ToolkitWidget);

    Toolkit = DungeonToolkit;
    Toolkit->Init(Owner->GetToolkitHost());
}

/** FEdMode: Called when the mode is exited */
void FEdModeDungeonLegacy::Exit() {
    if (ModeHandler) {
        ModeHandler->Exit();
    }

    if (Toolkit.IsValid()) {
        FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
        Toolkit.Reset();
    }
    //
    FEditorDelegates::NewCurrentLevel.RemoveAll(this);

    // Restore real-time viewport state if we changed it
    RestoreRealtimeOverride();

    DungeonBuildingNotification = nullptr;

    // Call base Exit method to ensure proper cleanup
    FEdMode::Exit();

    GEditor->UnregisterForUndo(this);
}


void FEdModeDungeonLegacy::RecreateHandler(ADungeon* SelectedDungeon) {

    if (SelectedDungeon) {
        if (ModeHandler) {
            ModeHandler->Exit();
            ModeHandler = nullptr;
        }

        UDungeonBuilder* Builder = SelectedDungeon->GetBuilder();
        if (Builder) {
            ModeHandler = FDungeonEdModeHandlerFactory::Get()->CreateHandler(Builder->GetClass());
            if (ModeHandler) {
                ModeHandler->Enter();
            }
        }
    }
}

void FEdModeDungeonLegacy::PostUndo(bool bSuccess) {
    if (ModeHandler) {
        ModeHandler->Undo();
    }
}

void FEdModeDungeonLegacy::PostRedo(bool bSuccess) {
    if (ModeHandler) {
        ModeHandler->Redo();
    }
}

/** When the user changes the active streaming level with the level browser */
void FEdModeDungeonLegacy::NotifyNewCurrentLevel() {

}

/** When the user changes the current tool in the UI */
void FEdModeDungeonLegacy::NotifyToolChanged() {

}

bool FEdModeDungeonLegacy::DisallowMouseDeltaTracking() const {
    // We never want to use the mouse delta tracker while painting
    return bToolActive;
}

/** FEdMode: Called once per frame */
void FEdModeDungeonLegacy::Tick(FEditorViewportClient* ViewportClient, float DeltaTime) {
    if (bToolActive) {
        ApplyBrush(ViewportClient);
    }

    FEdMode::Tick(ViewportClient, DeltaTime);
}

FVector PerformLocationSnapMap(const FVector& Location) {
    //ULevelEditorViewportSettings* ViewportSettings = GetMutableDefault<ULevelEditorViewportSettings>();
    //int32 SnapMapWidth = ViewportSettings->GridEnabled;
    int32 SnapMapWidth = GEditor->GetGridSize();
    if (SnapMapWidth <= 0) {
        return Location;
    }
    float X = Location.X / SnapMapWidth;
    float Y = Location.Y / SnapMapWidth;
    float Z = Location.Z / SnapMapWidth;

    X = FMath::RoundToInt(X) * SnapMapWidth;
    Y = FMath::RoundToInt(Y) * SnapMapWidth;
    Z = FMath::RoundToInt(Z) * SnapMapWidth;
    return FVector(X, Y, Z);
}

float SnapMapRotation(float Value, float SnapMapWidth) {
    return FMath::RoundToInt(Value / SnapMapWidth) * SnapMapWidth;
}

/**
 * Called when the mouse is moved over the viewport
 *
 * @param	InViewportClient	Level editor viewport client that captured the mouse input
 * @param	InViewport			Viewport that captured the mouse input
 * @param	InMouseX			New mouse cursor X coordinate
 * @param	InMouseY			New mouse cursor Y coordinate
 *
 * @return	true if input was handled
 */
bool FEdModeDungeonLegacy::MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 MouseX, int32 MouseY) {
    return false;
}

/**
 * Called when the mouse is moved while a window input capture is in effect
 *
 * @param	InViewportClient	Level editor viewport client that captured the mouse input
 * @param	InViewport			Viewport that captured the mouse input
 * @param	InMouseX			New mouse cursor X coordinate
 * @param	InMouseY			New mouse cursor Y coordinate
 *
 * @return	true if input was handled
 */
bool FEdModeDungeonLegacy::CapturedMouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 MouseX,
                                       int32 MouseY) {
    return false;
}

void FEdModeDungeonLegacy::ApplyBrush(FEditorViewportClient* ViewportClient) {
    if (ModeHandler) {
        ModeHandler->ApplyBrush(ViewportClient);
    }
}

TObjectPtr<UDungeonEdModeHandler> FEdModeDungeonLegacy::GetHandler() const {
    return ModeHandler;
}

/** FEdMode: Called when a key is pressed */
bool FEdModeDungeonLegacy::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) {
    if (Key == EKeys::LeftMouseButton) {
        bToolActive = (Event == IE_Pressed);
    }

    bool bKeyProcessed = false;
    if (ModeHandler) {
        bKeyProcessed = ModeHandler->InputKey(ViewportClient, Viewport, Key, Event);
    }
    return bKeyProcessed;
}

/** FEdMode: Render the DungeonDraw edit mode */
void FEdModeDungeonLegacy::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) {
    /** Call parent implementation */
    FEdMode::Render(View, Viewport, PDI);

    if (!IsRunningCommandlet() && ModeHandler) {
        ModeHandler->UpdateFrame(View, Viewport, PDI);
    }
}


/** FEdMode: Render HUD elements for this tool */
void FEdModeDungeonLegacy::DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View,
                             FCanvas* Canvas) {
}

/** FEdMode: Check to see if an actor can be selected in this mode - no side effects */
bool FEdModeDungeonLegacy::IsSelectionAllowed(AActor* InActor, bool bInSelection) const {
    if (InActor->IsA(ADungeon::StaticClass())) {
        return true;
    }
    return false;
}

/** FEdMode: Handling SelectActor */
bool FEdModeDungeonLegacy::Select(AActor* InActor, bool bInSelected) {
    return false;
}

/** FEdMode: Called when the currently selected actor has changed */
void FEdModeDungeonLegacy::ActorSelectionChangeNotify() {
    bool bRequiresUpdate = false;
    ADungeon* SelectedDungeon = nullptr;

    for (FSelectionIterator SelectionIt(*Owner->GetSelectedActors()); SelectionIt; ++SelectionIt) {
        AActor* CurActor = CastChecked<AActor>(*SelectionIt);

        if (CurActor->IsHidden() || !CurActor->IsSelected()) {
            continue;
        }

        if (CurActor->IsA(ADungeon::StaticClass()) && ModeHandler) {
            SelectedDungeon = Cast<ADungeon>(CurActor);
            if (ModeHandler->GetActiveDungeon() != SelectedDungeon) {
                bRequiresUpdate = true;
                break;
            }
        }
    }

    if (bRequiresUpdate) {
        ModeHandler->OnDungeonSelectionChanged(SelectedDungeon);

        RecreateHandler(SelectedDungeon);
        RecreateToolkit();
    }
}

const FText FEdModeDungeonLegacy::RealtimeDisplayId = LOCTEXT("DARealtimeOverrideText", "Dungeon EdMode");
void FEdModeDungeonLegacy::SetRealtimeOverride() {
    FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
    TSharedPtr<IAssetViewport> ViewportWindow = LevelEditorModule.GetFirstActiveViewport();
    if (ViewportWindow.IsValid()) {
        FEditorViewportClient& Viewport = ViewportWindow->GetAssetViewportClient();
        Viewport.AddRealtimeOverride(true, RealtimeDisplayId);
    }
}

void FEdModeDungeonLegacy::RestoreRealtimeOverride() {
    FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
    TSharedPtr<IAssetViewport> ViewportWindow = LevelEditorModule.GetFirstActiveViewport();
    if (ViewportWindow.IsValid()) {
        FEditorViewportClient& Viewport = ViewportWindow->GetAssetViewportClient();
        Viewport.RemoveRealtimeOverride(RealtimeDisplayId);
    }
}


bool FEdModeDungeonLegacy::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy,
                                 const FViewportClick& Click) {
    if (Click.GetKey() == EKeys::LeftMouseButton) {
        ApplyBrush(InViewportClient);
    }

    return FEdMode::HandleClick(InViewportClient, HitProxy, Click);
}

FVector FEdModeDungeonLegacy::GetWidgetLocation() const {
    return FEdMode::GetWidgetLocation();
}

/** FEdMode: Called when a mouse button is pressed */
bool FEdModeDungeonLegacy::StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) {
    return FEdMode::StartTracking(InViewportClient, InViewport);
}

/** FEdMode: Called when the a mouse button is released */
bool FEdModeDungeonLegacy::EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) {
    return FEdMode::EndTracking(InViewportClient, InViewport);
}

/** FEdMode: Called when mouse drag input it applied */
bool FEdModeDungeonLegacy::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag,
                                FRotator& InRot, FVector& InScale) {
    if (bToolActive) {
        return true;
    }

    return FEdMode::InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale);
}

bool FEdModeDungeonLegacy::AllowWidgetMove() {
    return ShouldDrawWidget();
}

bool FEdModeDungeonLegacy::UsesTransformWidget() const {
    return ShouldDrawWidget();
}

bool FEdModeDungeonLegacy::ShouldDrawWidget() const {
    return true;
}

EAxisList::Type FEdModeDungeonLegacy::GetWidgetAxisToDraw(UE::Widget::EWidgetMode InWidgetMode) const {
    switch (InWidgetMode) {
    case UE::Widget::WM_Translate:
    case UE::Widget::WM_Rotate:
    case UE::Widget::WM_Scale:
        return EAxisList::XYZ;
    default:
        return EAxisList::None;
    }
}


#undef LOCTEXT_NAMESPACE

