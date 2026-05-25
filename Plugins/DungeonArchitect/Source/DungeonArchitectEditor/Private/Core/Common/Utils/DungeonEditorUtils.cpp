//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Common/Utils/DungeonEditorUtils.h"

#include "Core/Dungeon.h"
#include "Core/LevelEditor/Notifications/DungeonArchitectNotificationSubsystem.h"
#include "Core/Utils/DungeonConstants.h"
#include "Frameworks/Snap/Lib/SnapDungeonModelBase.h"
#include "Frameworks/ThemeEngine/Common/DungeonThemeEngineUtils.h"
#include "Frameworks/ThemeEngine/DungeonThemeEngine.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Editor/LevelEditor/Public/LevelEditor.h"
#include "EditorActorFolders.h"
#include "EditorViewportClient.h"
#include "Engine/Selection.h"
#include "EngineUtils.h"
#include "IAssetViewport.h"
#include "IContentBrowserSingleton.h"
#include "ImageUtils.h"
#include "LevelEditorViewport.h"
#include "ObjectTools.h"
#include "SceneView.h"
#include "ScopedTransaction.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "DungeonEditorUtils"

DEFINE_LOG_CATEGORY(LogDungeonEditorUtils);

TObjectPtr<ADungeon> FDungeonEditorUtils::GetDungeonActorFromLevelViewport() {
    FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
    TSharedPtr<IAssetViewport> ViewportWindow = LevelEditorModule.GetFirstActiveViewport();
    TObjectPtr<ADungeon> DungeonCandidate = nullptr;
    if (ViewportWindow.IsValid()) {
        FEditorViewportClient& Viewport = ViewportWindow->GetAssetViewportClient();
        UWorld* World = Viewport.GetWorld();
        for (TActorIterator<ADungeon> DungeonIt(World); DungeonIt; ++DungeonIt) {
            ADungeon* Dungeon = *DungeonIt;
            if (Dungeon->IsSelected()) {
                return Dungeon;
            }
            DungeonCandidate = Dungeon;
        }
    }
    return DungeonCandidate;
}

void FDungeonEditorUtils::ShowNotification(FText Text, SNotificationItem::ECompletionState State /*= SNotificationItem::CS_Fail*/) {
    FNotificationInfo Info(Text);
    Info.bFireAndForget = true;
    Info.FadeOutDuration = 1.0f;
    Info.ExpireDuration = 2.0f;

	TSharedPtr<SNotificationItem> Notification = UDungeonArchitectNotificationSubsystem::Get().AddSimpleNotification(Info);
    if (Notification.IsValid()) {
        Notification->SetCompletionState(State);
    }
}

void FDungeonEditorUtils::SwitchToRealtimeMode() {
    FEditorViewportClient* ViewportClient = nullptr;
    if (GEditor) {
        if (const FViewport* ActiveViewport = GEditor->GetActiveViewport()) {
            ViewportClient = static_cast<FEditorViewportClient*>(ActiveViewport->GetClient());
        }
    }
    if (ViewportClient) {
        const bool bRealtime = ViewportClient->IsRealtime();
        if (!bRealtime) {
            ShowNotification(
                NSLOCTEXT("DungeonRealtimeMode", "DungeonRealtimeMode", "Switched viewport to Realtime mode"),
                SNotificationItem::CS_None);
            ViewportClient->SetRealtime(true);
        }
    }
    else {
        ShowNotification(NSLOCTEXT("ClientNotFound", "ClientNotFound", "Warning: Cannot find active viewport"));
    }
}

void FDungeonEditorUtils::CreateDungeonItemFolder(ADungeon* Dungeon) {
    if (Dungeon) {
        UWorld& World = *Dungeon->GetWorld();
        const FScopedTransaction Transaction(LOCTEXT("UndoAction_CreateFolder", "Create Folder"));

        auto& Folders = FActorFolders::Get();

        if (Dungeon->bUseCustomItemFolderName && !Dungeon->CustomItemFolderName.IsEmpty()) {
            Dungeon->ItemFolderPath = FName(Dungeon->CustomItemFolderName);
        }
        else {
            const FString FullPath = Dungeon->GetName() + "_Items";
            const FName Path(*FullPath);
            Dungeon->ItemFolderPath = Path;
        }

        const FFolder::FRootObject& RootObject(Dungeon);
        Folders.CreateFolder(World, FFolder(RootObject, Dungeon->ItemFolderPath));

        if (USnapDungeonModelBase* SnapModel = Cast<USnapDungeonModelBase>(Dungeon->GetModel())) {
            Folders.CreateFolder(World, FFolder(RootObject, FName(Dungeon->ItemFolderPath.ToString() + FDungeonArchitectConstants::DungeonFolderPathSuffix_SnapInstances)));
            Folders.CreateFolder(World, FFolder(RootObject, FName(Dungeon->ItemFolderPath.ToString() + FDungeonArchitectConstants::DungeonFolderPathSuffix_SnapConnections)));
        }
    }
    else {
        // Folder manager does not exist.  Clear the folder path from the dungeon actor,
        // so they are spawned in the root folder node
        Dungeon->ItemFolderPath = FName();
    }
}

void FDungeonEditorUtils::SetupBuildPriorityLocation(ADungeon* Dungeon) {
	if (Dungeon) {
		Dungeon->BuildPriorityLocation = GetEditorViewportLocation();
	}
}

void FDungeonEditorUtils::CollapseDungeonItemFolder(ADungeon* Dungeon) {
    if (Dungeon && Dungeon->GetWorld()) {
        const FFolder::FRootObject& RootObject(Dungeon);
        const FFolder DungeonFolder(RootObject, Dungeon->ItemFolderPath);

        // TODO: This does not work. Investigate
        FActorFolders::Get().SetIsFolderExpanded(*Dungeon->GetWorld(), DungeonFolder, false);
    }
}

void FDungeonEditorUtils::BuildDungeon(ADungeon* Dungeon, const FDungeonBuildSettings& InBuildSettings, bool bShowBuildNotification) {
	check(IsInGameThread());
	
	if (Dungeon) {
		SwitchToRealtimeMode();
		CreateDungeonItemFolder(Dungeon);

		SetupBuildPriorityLocation(Dungeon);
		Dungeon->BuildDungeonWithSettings(InBuildSettings);

		if (bShowBuildNotification) {
			UDungeonArchitectNotificationSubsystem::Get().OnDungeonBuildStarted(Dungeon);
		}

		CollapseDungeonItemFolder(Dungeon);
	}
}

void FDungeonEditorUtils::HandleOpenURL(const TCHAR* URL) {
    FPlatformProcess::LaunchURL(URL, nullptr, nullptr);
}

void FDungeonEditorUtils::CaptureThumbnailFromViewport(FViewport* InViewport, const TArray<FAssetData>& InAssetsToAssign, const TFunction<FColor(const FColor&)>& InColorTransform) {
    //capture the thumbnail
	uint32 SrcWidth = InViewport->GetSizeXY().X;
	uint32 SrcHeight = InViewport->GetSizeXY().Y;
	// Read the contents of the viewport into an array.
	TArray<FColor> OrigBitmap;
	if (InViewport->ReadPixels(OrigBitmap))
	{
		check(OrigBitmap.Num() == SrcWidth * SrcHeight);

		//pin to smallest value
		int32 CropSize = FMath::Min<uint32>(SrcWidth, SrcHeight);
		//pin to max size
		int32 ScaledSize  = FMath::Min<uint32>(ThumbnailTools::DefaultThumbnailSize, CropSize);

		//calculations for cropping
		TArray<FColor> CroppedBitmap;
		CroppedBitmap.AddUninitialized(CropSize*CropSize);
		//Crop the image
		int32 CroppedSrcTop  = (SrcHeight - CropSize)/2;
		int32 CroppedSrcLeft = (SrcWidth - CropSize)/2;
		for (int32 Row = 0; Row < CropSize; ++Row)
		{
			//Row*Side of a row*byte per color
			int32 SrcPixelIndex = (CroppedSrcTop+Row)*SrcWidth + CroppedSrcLeft;
			const void* SrcPtr = &(OrigBitmap[SrcPixelIndex]);
			void* DstPtr = &(CroppedBitmap[Row*CropSize]);
			FMemory::Memcpy(DstPtr, SrcPtr, CropSize*4);
		}

		//Scale image down if needed
		TArray<FColor> ScaledBitmap;
		if (ScaledSize < CropSize)
		{
			FImageUtils::ImageResize( CropSize, CropSize, CroppedBitmap, ScaledSize, ScaledSize, ScaledBitmap, true );
		}
		else
		{
			//just copy the data over. sizes are the same
			ScaledBitmap = CroppedBitmap;
		}

		// Transform the color
		for (FColor& Color : ScaledBitmap) {
			Color = InColorTransform(Color);
		} 
		
		//setup actual thumbnail
		FObjectThumbnail TempThumbnail;
		TempThumbnail.SetImageSize( ScaledSize, ScaledSize );
		TArray<uint8>& ThumbnailByteArray = TempThumbnail.AccessImageData();

		// Copy scaled image into destination thumb
		int32 MemorySize = ScaledSize*ScaledSize*sizeof(FColor);
		ThumbnailByteArray.AddUninitialized(MemorySize);
		FMemory::Memcpy(&(ThumbnailByteArray[0]), &(ScaledBitmap[0]), MemorySize);

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

		//check if each asset should receive the new thumb nail
		for ( auto AssetIt = InAssetsToAssign.CreateConstIterator(); AssetIt; ++AssetIt )
		{
			const FAssetData& CurrentAsset = *AssetIt;

			//assign the thumbnail and dirty
			const FString ObjectFullName = CurrentAsset.GetFullName();
			const FString PackageName    = CurrentAsset.PackageName.ToString();

			UPackage* AssetPackage = FindObject<UPackage>( NULL, *PackageName );
			if ( ensure(AssetPackage) )
			{
				FObjectThumbnail* NewThumbnail = ThumbnailTools::CacheThumbnail(ObjectFullName, &TempThumbnail, AssetPackage);
				if ( ensure(NewThumbnail) )
				{
					//we need to indicate that the package needs to be resaved
					AssetPackage->MarkPackageDirty();

					// Let the content browser know that we've changed the thumbnail
					NewThumbnail->MarkAsDirty();
						
					// Signal that the asset was changed if it is loaded so thumbnail pools will update
					if ( CurrentAsset.IsAssetLoaded() )
					{
						CurrentAsset.GetAsset()->PostEditChange();
					}

					//Set that thumbnail as a valid custom thumbnail so it'll be saved out
					NewThumbnail->SetCreatedAfterCustomThumbsEnabled();
				}
			}
		}
	}
}

FVector FDungeonEditorUtils::GetEditorViewportLocation() {
	FVector ViewLocation = FVector::ZeroVector;

	if (GCurrentLevelEditingViewportClient) {
		ViewLocation = GCurrentLevelEditingViewportClient->GetViewLocation();
	}
	else {
		// Fallback: try to get the first perspective viewport
		for (FLevelEditorViewportClient* LevelVC : GEditor->GetLevelViewportClients()) {
			if (LevelVC && LevelVC->IsPerspective()) {
				ViewLocation = LevelVC->GetViewLocation();
				break;
			}
		}
	}

	return ViewLocation;
}

void FAssetPackageInfo::AddReferencedObjects(FReferenceCollector& Collector) {
    Collector.AddReferencedObject(Asset);
    Collector.AddReferencedObject(Package);
}

FString FAssetPackageInfo::GetReferencerName() const {
    static const FString NameString = TEXT("FAssetPackageInfo");
    return NameString;
}

FAssetPackageInfo FDungeonAssetUtils::DuplicateAsset(UObject* SourceAsset, const FString& TargetPackageName,
                                                     const FString& TargetObjectName) {
    FAssetPackageInfo Duplicate;
    if (SourceAsset) {
        // Make sure the referenced object is deselected before duplicating it.
        GEditor->GetSelectedObjects()->Deselect(SourceAsset);

        // Duplicate the asset
        Duplicate.Package = CreatePackage(*TargetPackageName);
        Duplicate.Asset = StaticDuplicateObject(SourceAsset, Duplicate.Package, *TargetObjectName);

        if (Duplicate.Asset) {
            Duplicate.Asset->MarkPackageDirty();

            // Notify the asset registry
            FAssetRegistryModule::AssetCreated(Duplicate.Asset);
        }
        else {
            UE_LOG(LogDungeonEditorUtils, Error, TEXT("Failed to duplicate asset %s"), *TargetObjectName);
        }
    }

    return Duplicate;
}

void FDungeonAssetUtils::SaveAsset(const FAssetPackageInfo& Info) {
    if (Info.Asset && Info.Package) {
        Info.Package->SetDirtyFlag(true);
        const FString PackagePath = Info.Package->GetOutermost()->GetName();
        const FString Filename = *FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Standalone;
        SaveArgs.SaveFlags = SAVE_NoError;
        SaveArgs.bForceByteSwapping = false;
        SaveArgs.bWarnOfLongFilename = true;
        SaveArgs.Error = GError;

        if (!UPackage::SavePackage(Info.Package, nullptr, *Filename, SaveArgs)) {
            UE_LOG(LogDungeonEditorUtils, Display, TEXT("Unable to save asset %s"), *Info.Asset->GetName());
        }
    }
}


void FDungeonLevelEditorUtils::GatherLevelViewportInfo(FMinimalViewInfo& OutViewInfo, FIntPoint& OutViewSize) {
	if (FLevelEditorViewportClient* ViewportClient = GCurrentLevelEditingViewportClient) {
		
		// Then use it in your original code
		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
			ViewportClient->Viewport,
			ViewportClient->GetScene(),
			ViewportClient->EngineShowFlags)
			.SetRealtimeUpdate(ViewportClient->IsRealtime()));

		FSceneView* SceneView = ViewportClient->CalcSceneView(&ViewFamily);
		
		FMinimalViewInfo ViewInfo;
		ViewInfo.Location = ViewportClient->GetViewLocation();
		ViewInfo.Rotation = ViewportClient->GetViewRotation();
		ViewInfo.FOV = ViewportClient->FOVAngle;

		const FMatrix& ProjMatrix = SceneView->ViewMatrices.GetProjectionMatrix();
		ViewInfo.AspectRatio = ProjMatrix.M[1][1] / ProjMatrix.M[0][0];

		if (SceneView->ViewMatrices.IsPerspectiveProjection()) {
			ViewInfo.ProjectionMode = ECameraProjectionMode::Perspective;
		}
		else {
			ViewInfo.ProjectionMode = ECameraProjectionMode::Orthographic;
			ViewInfo.OrthoWidth = SceneView->UnscaledViewRect.Width();
		}

		constexpr float ViewRectScaleMultiplier = 0.5f;
		FIntPoint ViewSize = FIntPoint(
			SceneView->UnscaledViewRect.Width(),
			SceneView->UnscaledViewRect.Height()) / 2;

		OutViewSize = ViewSize;
		OutViewInfo = ViewInfo;
	}
	else {
		OutViewSize = {};
		OutViewInfo = {};
	}
}

void FDungeonLevelEditorUtils::UpdateAssetThumbnail(const UObject* InAssetObject, FViewport* InViewportForThumbnail) {
	if (InAssetObject && InViewportForThumbnail) {
		// Update the thumbnail
		const FAssetData AssetData(InAssetObject);
		TArray<FAssetData> ThemeAssetList;
		ThemeAssetList.Add(AssetData);

		IContentBrowserSingleton& ContentBrowserSingleton = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get();
		ContentBrowserSingleton.CaptureThumbnailFromViewport(InViewportForThumbnail, ThemeAssetList);
	}
}

TSharedPtr<IDetailsView> FDungeonLevelEditorUtils::FindPropertyEditor() {
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	static const FName DetailsTabIdentifiers[] = {
		LevelEditorTabIds::LevelEditorSelectionDetails,
		LevelEditorTabIds::LevelEditorSelectionDetails2,
		LevelEditorTabIds::LevelEditorSelectionDetails3,
		LevelEditorTabIds::LevelEditorSelectionDetails4
	};
	
	for (const FName& DetailsTabIdentifier : DetailsTabIdentifiers) {
		TSharedPtr<IDetailsView> DetailsView = PropertyEditorModule.FindDetailView(DetailsTabIdentifier);
		if(DetailsView.IsValid()) {
			return DetailsView;
		}
	}

	return nullptr;
}

void FDungeonLevelEditorUtils::SetDetailsViewObjects(const TArray<UObject*>& InObjects, bool bForceRefresh) {
	TSharedPtr<IDetailsView> DetailsView = FindPropertyEditor();
	if (DetailsView.IsValid()) {
		DetailsView->SetObjects(InObjects, bForceRefresh);
	}
}

AActor* FDungeonLevelEditorUtils::FindNearestTaggedActor(const UWorld* InWorld, const FVector& InLocation, const FName& InTag) {
	AActor* BestActor = nullptr;
	float BestDist = MAX_flt;
	if (InWorld) {
		for (TActorIterator<AActor> It(InWorld); It; ++It) {
			AActor* Actor = *It;
			if (Actor->Tags.Contains(InTag)) {
				const float DistanceSq = (Actor->GetActorLocation() - InLocation).SizeSquared();
				if (!BestActor || DistanceSq < BestDist) {
					BestDist = DistanceSq;
					BestActor = Actor;
				}
			}
		}
	}
	return BestActor;
}

AActor* FDungeonLevelEditorUtils::FindNearestNodeMesh(const FEditorViewportClient* ViewportClient, const FName& InNodeId) {
	if (!ViewportClient) {
		return nullptr;
	}
	
	// Zoom in on the nearest spawned actor in the viewport
	const FName NodeTag = FDungeonThemeEngineUtils::CreateNodeTagFromId(InNodeId);
	const FVector ViewLocation = ViewportClient->GetViewLocation();
	UWorld* World = ViewportClient->GetWorld();
	return FindNearestTaggedActor(World, ViewLocation, NodeTag);
}

void FDungeonLevelEditorUtils::FocusViewportOnActor(FEditorViewportClient* InViewportClient, const AActor* InActor) {
	if (InActor) {
		// Zoom in on the actor
		FVector BoundsOrigin, BoundsExtent;
		InActor->GetActorBounds(false, BoundsOrigin, BoundsExtent);
		BoundsExtent += FVector(100, 100, 100);
		const FBox Bounds(BoundsOrigin - BoundsExtent, BoundsOrigin + BoundsExtent);
		InViewportClient->FocusViewportOnBox(Bounds);
	}
}


#undef LOCTEXT_NAMESPACE

