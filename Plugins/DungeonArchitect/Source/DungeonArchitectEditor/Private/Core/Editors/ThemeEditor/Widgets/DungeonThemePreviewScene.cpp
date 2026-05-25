//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/Widgets/DungeonThemePreviewScene.h"

#include "AssetViewerSettings.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LineBatchComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "UObject/Package.h"

FDungeonThemePreviewScene::FDungeonThemePreviewScene(ConstructionValues CVS, float InFloorOffset)
	: FAdvancedPreviewScene(CVS, InFloorOffset)
{
	// Rollback changes created by parent constructor, so we can recreate the world with our settings
	ReleaseDefaultWorld();

	// Recreate FPreviewScene initialization
	{
		EObjectFlags NewObjectFlags = RF_NoFlags;
		if (CVS.bTransactional)
		{
			NewObjectFlags = RF_Transactional;
		}

		PreviewWorld = NewObject<UWorld>(GetTransientPackage(), NAME_None, NewObjectFlags);
		//PreviewWorld->WorldType = CVS.bEditor ? EWorldType::EditorPreview : EWorldType::GamePreview;
		PreviewWorld->WorldType = EWorldType::Editor;

		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(PreviewWorld->WorldType);
		WorldContext.SetCurrentWorld(PreviewWorld);

		PreviewWorld->InitializeNewWorld(UWorld::InitializationValues()
											.AllowAudioPlayback(CVS.bAllowAudioPlayback)
											.CreatePhysicsScene(CVS.bCreatePhysicsScene)
											.RequiresHitProxies(CVS.bEditor) // Only Need hit proxies in an editor scene
											.CreateNavigation(false)
											.CreateAISystem(false)
											.ShouldSimulatePhysics(CVS.bShouldSimulatePhysics)
											.SetTransactional(CVS.bTransactional)
											.SetDefaultGameMode(CVS.DefaultGameMode)
											.ForceUseMovementComponentInNonGameWorld(CVS.bForceUseMovementComponentInNonGameWorld));

		FURL URL = FURL();

		if (CVS.OwningGameInstance && PreviewWorld->WorldType == EWorldType::GamePreview) {
			PreviewWorld->SetGameInstance(CVS.OwningGameInstance);

			FWorldContext& PreviewWorldContext = GEngine->GetWorldContextFromWorldChecked(PreviewWorld);
			PreviewWorldContext.OwningGameInstance = CVS.OwningGameInstance;
			PreviewWorldContext.GameViewport = CVS.OwningGameInstance->GetGameViewportClient();
			PreviewWorldContext.AddRef(PreviewWorld);
		}

		PreviewWorld->InitializeActorsForPlay(URL);

		if (CVS.bDefaultLighting)
		{
			DirectionalLight = NewObject<UDirectionalLightComponent>(GetTransientPackage(), NAME_None, RF_Transient);
			DirectionalLight->Intensity = CVS.LightBrightness;
			DirectionalLight->LightColor = FColor::White;
			DirectionalLight->bTransmission = true;
			AddComponent(DirectionalLight, FTransform(CVS.LightRotation));

			SkyLight = NewObject<USkyLightComponent>(GetTransientPackage(), NAME_None, RF_Transient);
			SkyLight->bLowerHemisphereIsBlack = false;
			SkyLight->SourceType = ESkyLightSourceType::SLS_SpecifiedCubemap;
			SkyLight->Intensity = CVS.SkyBrightness;
			SkyLight->Mobility = EComponentMobility::Movable;
			AddComponent(SkyLight, FTransform::Identity);

			LineBatcher = NewObject<ULineBatchComponent>(GetTransientPackage());
			LineBatcher->bCalculateAccurateBounds = false;
			AddComponent(LineBatcher, FTransform::Identity);
		}
	}

	
	// Recreate FAdvancedPreviewScene initialization
	{
		DefaultSettings = UAssetViewerSettings::Get();
		check(DefaultSettings);

		CurrentProfileIndex = DefaultSettings->Profiles.IsValidIndex(CurrentProfileIndex) ? GetDefault<UEditorPerProjectUserSettings>()->AssetViewerProfileIndex : 0;
		ensureMsgf(DefaultSettings->Profiles.IsValidIndex(CurrentProfileIndex), TEXT("Invalid default settings pointer or current profile index"));
		FPreviewSceneProfile& Profile = DefaultSettings->Profiles[CurrentProfileIndex];
		Profile.LoadEnvironmentMap();

		const FTransform Transform(FRotator(0, 0, 0), FVector(0, 0, 0), FVector(1));

		// Always set up sky light using the set cube map texture, reusing the sky light from PreviewScene class
		SetSkyCubemap(Profile.EnvironmentCubeMap.Get());
		SetSkyBrightness(Profile.SkyLightIntensity);
		
		// Large scale to prevent sphere from clipping
		const FTransform SphereTransform(FRotator(0, 0, 0), FVector(0, 0, 0), FVector(2000));
		SkyComponent = NewObject<UStaticMeshComponent>(GetTransientPackage());

		// Set up sky sphere showing the same cube map as used by the sky light
		UStaticMesh* SkySphere = LoadObject<UStaticMesh>(NULL, TEXT("/Engine/EditorMeshes/AssetViewer/Sphere_inversenormals.Sphere_inversenormals"), NULL, LOAD_None, NULL);
		check(SkySphere);
		SkyComponent->SetStaticMesh(SkySphere);
		SkyComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SkyComponent->CastShadow = false;
		SkyComponent->bCastDynamicShadow = false;

		UMaterial* SkyMaterial = LoadObject<UMaterial>(NULL, TEXT("/Engine/EditorMaterials/AssetViewer/M_SkyBox.M_SkyBox"), NULL, LOAD_None, NULL);
		check(SkyMaterial);

		InstancedSkyMaterial = NewObject<UMaterialInstanceConstant>(GetTransientPackage());
		InstancedSkyMaterial->Parent = SkyMaterial;		

		UTextureCube* DefaultTexture = LoadObject<UTextureCube>(NULL, TEXT("/Engine/MapTemplates/Sky/SunsetAmbientCubemap.SunsetAmbientCubemap"));
	    
		InstancedSkyMaterial->SetTextureParameterValueEditorOnly(FName("SkyBox"), ( Profile.EnvironmentCubeMap.Get() != nullptr ) ? Profile.EnvironmentCubeMap.Get() : DefaultTexture );
		InstancedSkyMaterial->SetScalarParameterValueEditorOnly(FName("CubemapRotation"), Profile.LightingRigRotation / 360.0f);
		InstancedSkyMaterial->SetScalarParameterValueEditorOnly(FName("Intensity"), Profile.SkyLightIntensity);
		InstancedSkyMaterial->PostLoad();
		SkyComponent->SetMaterial(0, InstancedSkyMaterial);
		AddComponent(SkyComponent, SphereTransform);
		
		PostProcessComponent = NewObject<UPostProcessComponent>();
		PostProcessComponent->Settings = Profile.PostProcessingSettings;
		PostProcessComponent->bUnbound = true;
		AddComponent(PostProcessComponent, Transform);

		UStaticMesh* FloorMesh = LoadObject<UStaticMesh>(NULL, TEXT("/Engine/EditorMeshes/AssetViewer/Floor_Mesh.Floor_Mesh"), NULL, LOAD_None, NULL);
		check(FloorMesh);
		FloorMeshComponent = NewObject<UStaticMeshComponent>(GetTransientPackage());
		FloorMeshComponent->SetStaticMesh(FloorMesh);
		FloorMeshComponent->bSelectable = false;

		FTransform FloorTransform(FRotator(0, 0, 0), FVector(0, 0, -(InFloorOffset)), FVector(4.0f, 4.0f, 1.0f ));
		AddComponent(FloorMeshComponent, FloorTransform);	

		SetLightDirection(Profile.DirectionalLightRotation);

		bRotateLighting = Profile.bRotateLightingRig;
		CurrentRotationSpeed = Profile.RotationSpeed;
		bSkyChanged = false;

		BindCommands();

		// since advance preview scenes are used in conjunction with material/mesh editors we should match their feature level with the global one
		if (GIsEditor && GEditor != nullptr)
		{
			PreviewWorld->ChangeFeatureLevel(GEditor->DefaultWorldFeatureLevel);
		}

		PreviousRotation = Profile.LightingRigRotation;
		UILightingRigRotationDelta = 0.0f;
		
	}
}

void FDungeonThemePreviewScene::ReleaseDefaultWorld() {
	// Rollback FAdvancedPreviewScene
	{
		TArray<TObjectPtr<AActor>> ExistingActors = GetWorld()->PersistentLevel->Actors;
	}
	
	// Rollback FPreviewScene
	{
		TArray<UActorComponent*> AttachedComponents = {
			DirectionalLight,
			SkyLight,
			LineBatcher,
			SkyComponent,
			PostProcessComponent,
			FloorMeshComponent
		};
		
		// Remove all the attached components
		for (int32 ComponentIndex = 0; ComponentIndex < AttachedComponents.Num(); ComponentIndex++) {
			UActorComponent* Component = AttachedComponents[ComponentIndex];

			if (bForceAllUsedMipsResident)
			{
				// Remove the mip streaming override on the mesh to be removed
				UMeshComponent* pMesh = Cast<UMeshComponent>(Component);
				if (pMesh != NULL)
				{
					pMesh->SetTextureForceResidentFlag(false);
				}
			}

			Component->UnregisterComponent();
		}

		// Uninitialize can get called from destructor or FCoreDelegates::OnPreExit (or both)
		// so make sure we empty Components and set PreviewWorld to nullptr
		//Components.Empty();
		
		UWorld* LocalPreviewWorld = PreviewWorld;
		PreviewWorld = nullptr;

		// The world may be released by now.
		if (LocalPreviewWorld && GEngine)
		{
			LocalPreviewWorld->CleanupWorld();
			GEngine->DestroyWorldContext(LocalPreviewWorld);
			// Release PhysicsScene for fixing big fbx importing bug
			LocalPreviewWorld->ReleasePhysicsScene();

			// The preview world is a heavy-weight object and may hold a significant amount of resources,
			// including various GPU render targets and buffers required for rendering the scene.
			// Since UWorld is garbage-collected, this memory may not be cleaned for an indeterminate amount of time.
			// By forcing garbage collection explicitly, we allow memory to be reused immediately.
			GEngine->ForceGarbageCollection(true /*bFullPurge*/);
		}
	}
}

