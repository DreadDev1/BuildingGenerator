//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditorMode.h"

#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditorCommands.h"
#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditorModeSettings.h"
#include "Core/LevelEditor/EditorMode/ToolsMode/DungeonToolsEditorModeToolkit.h"
#include "Core/LevelEditor/EditorMode/ToolsMode/Tools/DungeonEdTool.h"
#include "Core/LevelEditor/EditorMode/ToolsMode/Tools/LayoutDesigner/DungeonLayoutDesignerEdTool.h"

#include "Application/ThrottleManager.h"
#include "BaseGizmos/TransformGizmoUtil.h"
#include "Components/StaticMeshComponent.h"
#include "Editor/UnrealEdEngine.h"
#include "EditorModeManager.h"
#include "EditorModelingObjectsCreationAPI.h"
#include "Engine/Selection.h"
#include "Engine/World.h"
#include "EngineAnalytics.h"
#include "Features/IModularFeatures.h"
#include "IAnalyticsProviderET.h"
#include "ILevelEditor.h"
#include "InteractiveToolQueryInterfaces.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "Scene/LevelObjectsObserver.h"
#include "Selection/GeometrySelectionManager.h"
#include "Selection/StaticMeshSelector.h"
#include "Selection/VolumeSelector.h"
#include "ToolSceneQueriesUtil.h"
#include "ToolTargetManager.h"
#include "ToolTargets/DynamicMeshComponentToolTarget.h"
#include "ToolTargets/SkeletalMeshComponentToolTarget.h"
#include "ToolTargets/StaticMeshComponentToolTarget.h"
#include "ToolTargets/VolumeComponentToolTarget.h"
#include "Tools/EdModeInteractiveToolsContext.h"
#include "UnrealEdGlobals.h" // GUnrealEd
#include "UnrealWidget.h"

#define LOCTEXT_NAMESPACE "UDungeonToolsEditorMode"


class UDungeonLayoutDesignerEdToolBuilder;
const FEditorModeID UDungeonToolsEditorMode::EM_DungeonToolsEditorModeId = TEXT("EM_DungeonToolsEditorMode");

FDateTime UDungeonToolsEditorMode::LastModeStartTimestamp;
FDateTime UDungeonToolsEditorMode::LastToolStartTimestamp;

FDelegateHandle UDungeonToolsEditorMode::GlobalDungeonToolWorldTeardownEventHandle;

namespace
{
FString GetToolName(const UInteractiveTool& Tool)
{
	const FString* ToolName = FTextInspector::GetSourceString(Tool.GetToolInfo().ToolDisplayName);
	return ToolName ? *ToolName : FString(TEXT("<Invalid ToolName>"));
}
}

UDungeonToolsEditorMode::UDungeonToolsEditorMode()
{
	Info = FEditorModeInfo(
		EM_DungeonToolsEditorModeId,
		LOCTEXT("DungeonToolsEditorModeName", "Dungeon Tools"),
		FSlateIcon("ModelingToolsStyle", "LevelEditor.ModelingToolsMode", "LevelEditor.ModelingToolsMode.Small"),
		false,		// Hide by default
		5000);
}

UDungeonToolsEditorMode::UDungeonToolsEditorMode(FVTableHelper& Helper)
	: UBaseLegacyWidgetEdMode(Helper)
{
}

UDungeonToolsEditorMode::~UDungeonToolsEditorMode()
{
}

bool UDungeonToolsEditorMode::ProcessEditDelete()
{
	if (UEdMode::ProcessEditDelete())
	{
		return true;
	}

	// for now we disable deleting in an Accept-style tool because it can result in crashes if we are deleting target object
	if ( GetToolManager()->HasAnyActiveTool() && GetToolManager()->GetActiveTool(EToolSide::Mouse)->HasAccept() )
	{
		GetToolManager()->DisplayMessage(
			LOCTEXT("CannotDeleteWarning", "Cannot delete objects while this Tool is active"), EToolMessageLevel::UserWarning);
		return true;
	}

	return false;
}


bool UDungeonToolsEditorMode::ProcessEditCut()
{
	// for now we disable deleting in an Accept-style tool because it can result in crashes if we are deleting target object
	if (GetToolManager()->HasAnyActiveTool() && GetToolManager()->GetActiveTool(EToolSide::Mouse)->HasAccept())
	{
		GetToolManager()->DisplayMessage(
			LOCTEXT("CannotCutWarning", "Cannot cut objects while this Tool is active"), EToolMessageLevel::UserWarning);
		return true;
	}

	return false;
}


void UDungeonToolsEditorMode::ActorSelectionChangeNotify()
{
	// would like to clear selection here, but this is called multiple times, including after a transaction when
	// we cannot identify that the selection should not be cleared
}


bool UDungeonToolsEditorMode::CanAutoSave() const
{
	// prevent autosave if any tool is active
	return GetToolManager()->HasAnyActiveTool() == false;
}

bool UDungeonToolsEditorMode::ShouldDrawWidget() const
{ 
	// hide standard xform gizmo if we have an active tool, unless it explicitly opts in via the IInteractiveToolEditorGizmoAPI
	if (GetInteractiveToolsContext() != nullptr && GetToolManager()->HasAnyActiveTool())
	{
		IInteractiveToolEditorGizmoAPI* GizmoAPI = Cast<IInteractiveToolEditorGizmoAPI>(GetToolManager()->GetActiveTool(EToolSide::Left));
		if (!GizmoAPI || !GizmoAPI->GetAllowStandardEditorGizmos())
		{
			return false;
		}
	}

	return UBaseLegacyWidgetEdMode::ShouldDrawWidget(); 
}

void UDungeonToolsEditorMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	Super::Tick(ViewportClient, DeltaTime);

	if (Toolkit.IsValid())
	{
		FDungeonToolsEditorModeToolkit* ModelingToolkit = (FDungeonToolsEditorModeToolkit*)Toolkit.Get();
		ModelingToolkit->ShowRealtimeAndModeWarnings(ViewportClient->IsRealtime() == false);
	}
}

void UDungeonToolsEditorMode::Enter()
{
	UEdMode::Enter();

	const UDungeonToolsEditorModeSettings* DungeonModeSettings = GetDefault<UDungeonToolsEditorModeSettings>();
	const UDungeonToolsModeCustomizationSettings* ModelingEditorSettings = GetDefault<UDungeonToolsModeCustomizationSettings>();

	// Register builders for tool targets that the mode uses.
	GetInteractiveToolsContext()->TargetManager->AddTargetFactory(NewObject<UStaticMeshComponentToolTargetFactory>(GetToolManager()));
	GetInteractiveToolsContext()->TargetManager->AddTargetFactory(NewObject<UVolumeComponentToolTargetFactory>(GetToolManager()));
	GetInteractiveToolsContext()->TargetManager->AddTargetFactory(NewObject<UDynamicMeshComponentToolTargetFactory>(GetToolManager()));

	// Register read-only skeletal mesh tool targets. Currently tools that write to meshes risk breaking
	// skin weights.
	GetInteractiveToolsContext()->TargetManager->AddTargetFactory(NewObject<USkeletalMeshComponentReadOnlyToolTargetFactory>(GetToolManager()));

	// listen to post-build
	GetToolManager()->OnToolPostBuild.AddUObject(this, &UDungeonToolsEditorMode::OnToolPostBuild);

	// forward shutdown requests
	GetToolManager()->OnToolShutdownRequest.BindLambda([this](UInteractiveToolManager*, UInteractiveTool* Tool, EToolShutdownType ShutdownType)
	{
		GetInteractiveToolsContext()->EndTool(ShutdownType); 
		return true;
	});

	// register for OnRender and OnDrawHUD extensions
	GetInteractiveToolsContext()->OnRender.AddUObject(this, &UDungeonToolsEditorMode::OnToolsContextRender);
	GetInteractiveToolsContext()->OnDrawHUD.AddUObject(this, &UDungeonToolsEditorMode::OnToolsContextDrawHUD);

	// register gizmo helper
	UE::TransformGizmoUtil::RegisterTransformGizmoContextObject(GetInteractiveToolsContext());
	// configure mode-level Gizmo options
	if (DungeonModeSettings)
	{
		GetInteractiveToolsContext()->SetForceCombinedGizmoMode(DungeonModeSettings->bRespectLevelEditorGizmoMode == false);
		//GetInteractiveToolsContext()->SetAbsoluteWorldSnappingEnabled(DungeonModeSettings->bEnableAbsoluteWorldSnapping);
	}

	// Now that we have the gizmo helper, bind the numerical UI.
	if (ensure(Toolkit.IsValid()))
	{
		((FDungeonToolsEditorModeToolkit*)Toolkit.Get())->BindGizmoNumericalUI();
	}
	
	// rebuild tool palette on any selection changes. This is expensive and ideally will be
	// optimized in the future.
	//SelectionManager_SelectionModifiedHandle = SelectionManager->OnSelectionModified.AddLambda([this]()
	//{
	//	((FDungeonToolsEditorModeToolkit*)Toolkit.Get())->ForceToolPaletteRebuild();
	//});


	// register level objects observer that will update the snapping manager as the scene changes
	LevelObjectsObserver = MakeShared<FLevelObjectsObserver>();

	// tracker will auto-populate w/ the current level, but must have registered the handlers first!
	LevelObjectsObserver->Initialize(GetWorld());

	// disable HitProxy rendering, it is not used in Modeling Mode and adds overhead to Render() calls
	GetInteractiveToolsContext()->SetEnableRenderingDuringHitProxyPass(false);

	const FDungeonToolsEditorCommands& ToolManagerCommands = FDungeonToolsEditorCommands::Get();

	// register tool set

	//
	// builder tools
	//
	auto RegisterDungeonToolFunc  =
		[this](TSharedPtr<FUICommandInfo> UICommand, FString&& ToolIdentifier, TSubclassOf<UDungeonEdToolBase> ToolClass)
	{
		UDungeonEdToolBuilder* DungeonToolBuilder = NewObject<UDungeonEdToolBuilder>();
		DungeonToolBuilder->ToolClass = ToolClass;
		RegisterTool(UICommand, ToolIdentifier, DungeonToolBuilder);
	};
	RegisterDungeonToolFunc(ToolManagerCommands.BeginGridPaintTool,
							  TEXT("BeginGridPaintTool"),
							  UDungeonGridPaintEdTool::StaticClass());
	
	RegisterDungeonToolFunc(ToolManagerCommands.BeginSnapStitchModuleTool,
							  TEXT("BeginSnapStitchModuleTool"),
							  UDungeonSnapStitchEdTool::StaticClass());
	
	RegisterDungeonToolFunc(ToolManagerCommands.BeginSnapConnectionTool,
							  TEXT("BeginSnapConnectionTool"),
							  UDungeonSnapConnectionEdTool::StaticClass());
	
	RegisterDungeonToolFunc(ToolManagerCommands.BeginFlowNodeTool,
							  TEXT("BeginFlowNodeTool"),
							  UDungeonFlowNodeEdTool::StaticClass());
	

	auto RegisterLayoutToolFunc  = [this](TSharedPtr<FUICommandInfo> UICommand, FString&& ToolIdentifier, TSubclassOf<UDungeonLayoutDesignerEdToolBase> ToolClass) {
			UDungeonLayoutDesignerEdToolBuilder* ToolBuilder = NewObject<UDungeonLayoutDesignerEdToolBuilder>();
			ToolBuilder->ToolClass = ToolClass;
			RegisterTool(UICommand, ToolIdentifier, ToolBuilder);
		};
	
	RegisterLayoutToolFunc(ToolManagerCommands.BeginLayoutSegmentManageTool,
							  TEXT("LayoutSegmentManageTool"),
							  UDungeonLayoutDesignerEdTool::StaticClass());
	
	// enable realtime viewport override
	ConfigureRealTimeViewportsOverride(true);

	//
	// Engine Analytics
	//

	// Log mode starting
	if (FEngineAnalytics::IsAvailable())
	{
		LastModeStartTimestamp = FDateTime::UtcNow();

		TArray<FAnalyticsEventAttribute> Attributes;
		Attributes.Add(FAnalyticsEventAttribute(TEXT("Timestamp"), LastModeStartTimestamp.ToString()));

		FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.MeshModelingMode.Enter"), Attributes);
	}

	// Log tool starting
	GetToolManager()->OnToolStarted.AddLambda([this](UInteractiveToolManager* Manager, UInteractiveTool* Tool)
	{
		if (FEngineAnalytics::IsAvailable())
		{
			LastToolStartTimestamp = FDateTime::UtcNow();

			TArray<FAnalyticsEventAttribute> Attributes;
			Attributes.Add(FAnalyticsEventAttribute(TEXT("ToolName"), GetToolName(*Tool)));
			Attributes.Add(FAnalyticsEventAttribute(TEXT("Timestamp"), LastToolStartTimestamp.ToString()));

			FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.MeshModelingMode.ToolStarted"), Attributes);
		}
	});

	// Log tool ending
	GetToolManager()->OnToolEnded.AddLambda([this](UInteractiveToolManager* Manager, UInteractiveTool* Tool)
	{
		if (FEngineAnalytics::IsAvailable())
		{
			const FDateTime Now = FDateTime::UtcNow();
			const FTimespan ToolUsageDuration = Now - LastToolStartTimestamp;

			TArray<FAnalyticsEventAttribute> Attributes;
			Attributes.Add(FAnalyticsEventAttribute(TEXT("ToolName"), GetToolName(*Tool)));
			Attributes.Add(FAnalyticsEventAttribute(TEXT("Timestamp"), Now.ToString()));
			Attributes.Add(FAnalyticsEventAttribute(TEXT("Duration.Seconds"), static_cast<float>(ToolUsageDuration.GetTotalSeconds())));

			FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.MeshModelingMode.ToolEnded"), Attributes);
		}
	});

	// do any toolkit UI initialization that depends on the mode setup above
	if (Toolkit.IsValid())
	{
		FDungeonToolsEditorModeToolkit* ModelingToolkit = (FDungeonToolsEditorModeToolkit*)Toolkit.Get();
		ModelingToolkit->InitializeAfterModeSetup();
	}

	EditorClosedEventHandle = GEditor->OnEditorClose().AddUObject(this, &UDungeonToolsEditorMode::OnEditorClosed);

	BlueprintPreCompileHandle = GEditor->OnBlueprintPreCompile().AddUObject(this, &UDungeonToolsEditorMode::OnBlueprintPreCompile);

	// Removing levels from the world can happen either by entering/exiting level instance edit mode, or
	// by using the Levels panel. The problem is that any temporary actors we may have spawned in the 
	// level for visualization, gizmos, etc. will be garbage collected. While EdModeInteractiveToolsContext
	// should end the tools for us, we still have to take care of mode-level temporary actors.
	FWorldDelegates::PreLevelRemovedFromWorld.AddWeakLambda(this, [this](ULevel*, UWorld*) {
		// The ideal solution would be to just exit the mode, but we don't have a way to do that- we
		// can only request a mode switch on next tick.Since this is too late to prevent a crash, we
		// hand-clean up temporary actors here.
		//if (SelectionInteraction) {
		//	SelectionInteraction->Shutdown();
		//}

		// Since we're doing this hand-cleanup above, we could actually register to OnCurrentLevelChanged and
		// reinstate the temporary actors to stay in the mode. That seems a bit brittle, though, and there
		// is still some hope that we can someday exit the mode instead of having to keep track of what is
		// in danger of being garbage collected, so we might as well keep the workflow the same (i.e. exit
		// mode).
		GetModeManager()->ActivateDefaultMode();
	});
}

void UDungeonToolsEditorMode::Exit()
{
	FWorldDelegates::PreLevelRemovedFromWorld.RemoveAll(this);
	if (BlueprintPreCompileHandle.IsValid())
	{
		GEditor->OnBlueprintPreCompile().Remove(BlueprintPreCompileHandle);
	}

	// exit any exclusive active tools w/ cancel
	if (UInteractiveTool* ActiveTool = GetToolManager()->GetActiveTool(EToolSide::Left))
	{
		if (Cast<IInteractiveToolExclusiveToolAPI>(ActiveTool))
		{
			GetToolManager()->DeactivateTool(EToolSide::Left, EToolShutdownType::Cancel);
		}
	}

	//
	// Engine Analytics
	//
	// Log mode ending
	if (FEngineAnalytics::IsAvailable())
	{
		const FTimespan ModeUsageDuration = FDateTime::UtcNow() - LastModeStartTimestamp;

		TArray<FAnalyticsEventAttribute> Attributes;
		Attributes.Add(FAnalyticsEventAttribute(TEXT("Timestamp"), FDateTime::UtcNow().ToString()));
		Attributes.Add(FAnalyticsEventAttribute(TEXT("Duration.Seconds"), static_cast<float>(ModeUsageDuration.GetTotalSeconds())));

		FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.MeshModelingMode.Exit"), Attributes);
	}

	// TODO: cannot deregister currently because if another mode is also registering, its Enter()
	// will be called before our Exit()
	//UE::TransformGizmoUtil::DeregisterTransformGizmoContextObject(ToolsContext.Get());
	
	// deregister snapping manager and shut down level objects tracker
	LevelObjectsObserver->Shutdown();		// do this first because it is going to fire events on the snapping manager
	LevelObjectsObserver.Reset();

	// TODO: cannot deregister currently because if another mode is also registering, its Enter()
	// will be called before our Exit()
	UEditorModelingObjectsCreationAPI* ObjectCreationAPI = UEditorModelingObjectsCreationAPI::Find(GetInteractiveToolsContext());
	if (ObjectCreationAPI)
	{
		ObjectCreationAPI->GetNewAssetPathNameCallback.Unbind();
		//UEditorModelingObjectsCreationAPI::Deregister(ToolsContext.Get());		// cannot do currently because of shared ToolsContext, revisit in future
	}

	// clear realtime viewport override
	ConfigureRealTimeViewportsOverride(false);

	// re-enable HitProxy rendering
	GetInteractiveToolsContext()->SetEnableRenderingDuringHitProxyPass(true);

	// Call base Exit method to ensure proper cleanup
	UEdMode::Exit();
}

void UDungeonToolsEditorMode::OnEditorClosed()
{
	// On editor close, Exit() should run to clean up, but this happens very late.
	// Close out any active Tools or Selections to mitigate any late-destruction issues.
	if (GetModeManager() != nullptr 
		&& GetInteractiveToolsContext() != nullptr 
		&& GetToolManager() != nullptr 
		&& GetToolManager()->HasAnyActiveTool())
	{
		GetToolManager()->DeactivateTool(EToolSide::Mouse, EToolShutdownType::Cancel);
	}

	if (EditorClosedEventHandle.IsValid() && GEditor)
	{
		GEditor->OnEditorClose().Remove(EditorClosedEventHandle);
	}
}


void UDungeonToolsEditorMode::OnBlueprintPreCompile(UBlueprint* Blueprint)
{
}


void UDungeonToolsEditorMode::OnToolsContextRender(IToolsContextRenderAPI* RenderAPI)
{
}

void UDungeonToolsEditorMode::OnToolsContextDrawHUD(FCanvas* Canvas, IToolsContextRenderAPI* RenderAPI)
{
}

bool UDungeonToolsEditorMode::ShouldToolStartBeAllowed(const FString& ToolIdentifier) const
{
	if (UInteractiveToolManager* Manager = GetToolManager())
	{
		if (UInteractiveTool* Tool = Manager->GetActiveTool(EToolSide::Left))
		{
			IInteractiveToolExclusiveToolAPI* ExclusiveAPI = Cast<IInteractiveToolExclusiveToolAPI>(Tool);
			if (ExclusiveAPI)
			{
				return false;
			}
		}
	}
	return Super::ShouldToolStartBeAllowed(ToolIdentifier);
}



bool UDungeonToolsEditorMode::TestForEditorGizmoHit(const FInputDeviceRay& ClickPos) const
{
	// Because the editor gizmo does not participate in InputRouter behavior system, in some input behaviors
	// we need to filter out clicks on the gizmo. This function can do this check.
	if ( ShouldDrawWidget() )
	{
		FViewport* FocusedViewport = GetToolManager()->GetContextQueriesAPI()->GetFocusedViewport();
		HHitProxy* HitResult = FocusedViewport->GetHitProxy(ClickPos.ScreenPosition.X, ClickPos.ScreenPosition.Y);
		if (HitResult && HitResult->IsA(HWidgetAxis::StaticGetType()))
		{
			return true;
		}
	}
	return false;
}

bool UDungeonToolsEditorMode::BoxSelect(FBox& InBox, bool InSelect) 
{
	// not handling yet
	return false;
}

bool UDungeonToolsEditorMode::FrustumSelect(const FConvexVolume& InFrustum, FEditorViewportClient* InViewportClient, bool InSelect)
{
	// not handling yet
	return false;
}



void UDungeonToolsEditorMode::CreateToolkit()
{
	Toolkit = MakeShareable(new FDungeonToolsEditorModeToolkit);
}

void UDungeonToolsEditorMode::OnToolPostBuild(
	UInteractiveToolManager* InToolManager, EToolSide InSide, 
	UInteractiveTool* InBuiltTool, UInteractiveToolBuilder* InToolBuilder, const FToolBuilderState& ToolState)
{
}

void UDungeonToolsEditorMode::OnToolStarted(UInteractiveToolManager* Manager, UInteractiveTool* Tool)
{
	// disable slate throttling so that Tool background computes responding to sliders can properly be processed
	// on Tool Tick. Otherwise, when a Tool kicks off a background update in a background thread, the computed
	// result will be ignored until the user moves the slider, ie you cannot hold down the mouse and wait to see
	// the result. This apparently broken behavior is currently by-design.
	FSlateThrottleManager::Get().DisableThrottle(true);

	if( FEngineAnalytics::IsAvailable() )
	{
		FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.MeshModelingMode.ToolStarted"),
		                                            TEXT("ToolName"),
		                                            GetToolName(*Tool));
	}
}

void UDungeonToolsEditorMode::OnToolEnded(UInteractiveToolManager* Manager, UInteractiveTool* Tool)
{
	// re-enable slate throttling (see OnToolStarted)
	FSlateThrottleManager::Get().DisableThrottle(false);

	// We may require a gizmo location update despite not having changed the selection (transform tool,
	// edit pivot, etc).
	GUnrealEd->UpdatePivotLocationForSelection();

	if( FEngineAnalytics::IsAvailable() )
	{
		FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.MeshModelingMode.ToolEnded"),
		                                            TEXT("ToolName"),
		                                            GetToolName(*Tool));
	}
}

void UDungeonToolsEditorMode::BindCommands()
{
	const FDungeonToolsEditorCommands& ToolManagerCommands = FDungeonToolsEditorCommands::Get();
	const TSharedRef<FUICommandList>& CommandList = Toolkit->GetToolkitCommands();

	CommandList->MapAction(
		ToolManagerCommands.AcceptActiveTool,
		FExecuteAction::CreateLambda([this]() { 
			GetInteractiveToolsContext()->EndTool(EToolShutdownType::Accept); 
		}),
		FCanExecuteAction::CreateLambda([this]() { return GetInteractiveToolsContext()->CanAcceptActiveTool(); }),
		FGetActionCheckState(),
		FIsActionButtonVisible::CreateLambda([this]() {return GetInteractiveToolsContext()->ActiveToolHasAccept(); }),
		EUIActionRepeatMode::RepeatDisabled
		);

	CommandList->MapAction(
		ToolManagerCommands.CancelActiveTool,
		FExecuteAction::CreateLambda([this]() { GetInteractiveToolsContext()->EndTool(EToolShutdownType::Cancel); }),
		FCanExecuteAction::CreateLambda([this]() { return GetInteractiveToolsContext()->CanCancelActiveTool(); }),
		FGetActionCheckState(),
		FIsActionButtonVisible::CreateLambda([this]() {return GetInteractiveToolsContext()->ActiveToolHasAccept(); }),
		EUIActionRepeatMode::RepeatDisabled
		);

	CommandList->MapAction(
		ToolManagerCommands.CompleteActiveTool,
		FExecuteAction::CreateLambda([this]() { GetInteractiveToolsContext()->EndTool(EToolShutdownType::Completed); }),
		FCanExecuteAction::CreateLambda([this]() { return GetInteractiveToolsContext()->CanCompleteActiveTool(); }),
		FGetActionCheckState(),
		FIsActionButtonVisible::CreateLambda([this]() {return GetInteractiveToolsContext()->CanCompleteActiveTool(); }),
		EUIActionRepeatMode::RepeatDisabled
		);

	// These aren't activated by buttons but have default chords that bind the keypresses to the action.
	CommandList->MapAction(
		ToolManagerCommands.AcceptOrCompleteActiveTool,
		FExecuteAction::CreateLambda([this]() { AcceptActiveToolActionOrTool(); }),
		FCanExecuteAction::CreateLambda([this]() {
				return GetInteractiveToolsContext()->CanAcceptActiveTool() || GetInteractiveToolsContext()->CanCompleteActiveTool();
			}),
		FGetActionCheckState(),
		FIsActionButtonVisible(),
		EUIActionRepeatMode::RepeatDisabled);

	CommandList->MapAction(
		ToolManagerCommands.CancelOrCompleteActiveTool,
		FExecuteAction::CreateLambda([this]() { CancelActiveToolActionOrTool(); }),
		FCanExecuteAction::CreateLambda([this]() {
				return GetInteractiveToolsContext()->CanCompleteActiveTool() || GetInteractiveToolsContext()->CanCancelActiveTool();
			}),
		FGetActionCheckState(),
		FIsActionButtonVisible(),
		EUIActionRepeatMode::RepeatDisabled);
}


void UDungeonToolsEditorMode::AcceptActiveToolActionOrTool()
{
	// if we have an active Tool that implements 
	if (GetToolManager()->HasAnyActiveTool())
	{
		UInteractiveTool* Tool = GetToolManager()->GetActiveTool(EToolSide::Mouse);
		IInteractiveToolNestedAcceptCancelAPI* CancelAPI = Cast<IInteractiveToolNestedAcceptCancelAPI>(Tool);
		if (CancelAPI && CancelAPI->SupportsNestedAcceptCommand() && CancelAPI->CanCurrentlyNestedAccept())
		{
			bool bAccepted = CancelAPI->ExecuteNestedAcceptCommand();
			if (bAccepted)
			{
				return;
			}
		}
	}

	const EToolShutdownType ShutdownType = GetInteractiveToolsContext()->CanAcceptActiveTool() ? EToolShutdownType::Accept : EToolShutdownType::Completed;
	GetInteractiveToolsContext()->EndTool(ShutdownType);
}


void UDungeonToolsEditorMode::CancelActiveToolActionOrTool()
{
	// if we have an active Tool that implements 
	if (GetToolManager()->HasAnyActiveTool())
	{
		UInteractiveTool* Tool = GetToolManager()->GetActiveTool(EToolSide::Mouse);
		IInteractiveToolNestedAcceptCancelAPI* CancelAPI = Cast<IInteractiveToolNestedAcceptCancelAPI>(Tool);
		if (CancelAPI && CancelAPI->SupportsNestedCancelCommand() && CancelAPI->CanCurrentlyNestedCancel())
		{
			bool bCancelled = CancelAPI->ExecuteNestedCancelCommand();
			if (bCancelled)
			{
				return;
			}
		}
	}

	const EToolShutdownType ShutdownType = GetInteractiveToolsContext()->CanCancelActiveTool() ? EToolShutdownType::Cancel : EToolShutdownType::Completed;
	GetInteractiveToolsContext()->EndTool(ShutdownType);
}

void UDungeonToolsEditorMode::FocusCameraAtCursorHotkey()
{
	FRay Ray = GetInteractiveToolsContext()->GetLastWorldRay();

	double NearestHitDist = (double)HALF_WORLD_MAX;
	FVector HitPoint = FVector::ZeroVector;

	// cast ray against visible objects
	FHitResult WorldHitResult;
	if (ToolSceneQueriesUtil::FindNearestVisibleObjectHit(GetWorld(), WorldHitResult, Ray) )
	{
		HitPoint = WorldHitResult.ImpactPoint;
		NearestHitDist = (double)Ray.GetParameter(HitPoint);
	}

	// cast ray against tool
	if (GetToolManager()->HasAnyActiveTool())
	{
		UInteractiveTool* Tool = GetToolManager()->GetActiveTool(EToolSide::Mouse);
		IInteractiveToolCameraFocusAPI* FocusAPI = Cast<IInteractiveToolCameraFocusAPI>(Tool);
		if (FocusAPI && FocusAPI->SupportsWorldSpaceFocusPoint())
		{
			FVector ToolHitPoint;
			if (FocusAPI->GetWorldSpaceFocusPoint(Ray, ToolHitPoint))
			{
				double HitDepth = (double)Ray.GetParameter(ToolHitPoint);
				if (HitDepth < NearestHitDist)
				{
					NearestHitDist = HitDepth;
					HitPoint = ToolHitPoint;
				}
			}
		}
	}


	if (NearestHitDist < (double)HALF_WORLD_MAX && GCurrentLevelEditingViewportClient)
	{
		GCurrentLevelEditingViewportClient->CenterViewportAtPoint(HitPoint, false);
	}
}


bool UDungeonToolsEditorMode::ComputeBoundingBoxForViewportFocus(AActor* Actor, UPrimitiveComponent* PrimitiveComponent, FBox& InOutBox) const
{
	auto ProcessFocusBoxFunc = [](FBox& FocusBoxInOut)
	{
		double MaxDimension = FocusBoxInOut.GetExtent().GetMax();
		double ExpandAmount = (MaxDimension > SMALL_NUMBER) ? (MaxDimension * 0.2) : 25;		// 25 is a bit arbitrary here...
		FocusBoxInOut = FocusBoxInOut.ExpandBy(MaxDimension * 0.2);
	};

	// if Tool supports custom Focus box, use that
	if (GetToolManager()->HasAnyActiveTool())
	{
		UInteractiveTool* Tool = GetToolManager()->GetActiveTool(EToolSide::Mouse);
		IInteractiveToolCameraFocusAPI* FocusAPI = Cast<IInteractiveToolCameraFocusAPI>(Tool);
		if (FocusAPI && FocusAPI->SupportsWorldSpaceFocusBox() )
		{
			InOutBox = FocusAPI->GetWorldSpaceFocusBox();
			if (InOutBox.IsValid)
			{
				ProcessFocusBoxFunc(InOutBox);
				return true;
			}
		}
	}

	// fallback to base focus behavior
	return false;
}


bool UDungeonToolsEditorMode::GetPivotForOrbit(FVector& OutPivot) const
{
	if (GCurrentLevelEditingViewportClient)
	{
		OutPivot = GCurrentLevelEditingViewportClient->GetViewTransform().GetLookAt();
		return true;
	}
	return false;
}



void UDungeonToolsEditorMode::ConfigureRealTimeViewportsOverride(bool bEnable)
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule.GetFirstLevelEditor();
	if (LevelEditor.IsValid())
	{
		TArray<TSharedPtr<SLevelViewport>> Viewports = LevelEditor->GetViewports();
		for (const TSharedPtr<SLevelViewport>& ViewportWindow : Viewports)
		{
			if (ViewportWindow.IsValid())
			{
				FEditorViewportClient& Viewport = ViewportWindow->GetAssetViewportClient();
				const FText SystemDisplayName = LOCTEXT("RealtimeOverrideMessage_ModelingMode", "Modeling Mode");
				if (bEnable)
				{
					Viewport.AddRealtimeOverride(bEnable, SystemDisplayName);
				}
				else
				{
					Viewport.RemoveRealtimeOverride(SystemDisplayName, false);
				}
			}
		}
	}
}



#undef LOCTEXT_NAMESPACE

