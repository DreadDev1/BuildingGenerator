//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Common/Utils/DungeonEditorViewportCapture.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"

DEFINE_LOG_CATEGORY_STATIC(LogDAViewportCapture, Log, All);

////////////// UDungeonEditorViewportCapture //////////////
UDungeonEditorViewportCapture::UDungeonEditorViewportCapture() {
	
}

void UDungeonEditorViewportCapture::Initialize(UWorld* InWorld) {
	PrivateWorld = InWorld;

	CaptureComponent = NewObject<USceneCaptureComponent2D>(this, "Thumbnail_Capture_Component");
	CaptureComponent->RegisterComponentWithWorld(InWorld);
	CaptureComponent->bConsiderUnrenderedOpaquePixelAsFullyTranslucent = true;
	CaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	CaptureComponent->bCaptureEveryFrame = false;
	CaptureComponent->bCaptureOnMovement = false;
	CaptureComponent->bAlwaysPersistRenderingState = true;
}

void UDungeonEditorViewportCapture::Deinitialize() {
	ReleaseResources();
	CaptureComponent->UnregisterComponent();
}

void UDungeonEditorViewportCapture::BeginDestroy() {
	UObject::BeginDestroy();

	ReleaseResources();
	
	if (CaptureComponent) {
		CaptureComponent->UnregisterComponent();
		CaptureComponent = nullptr;
	}
}

void UDungeonEditorViewportCapture::SetRenderTargetSize(int32 Width, int32 Height) {
	
	if (SurfaceWidth != Width || SurfaceHeight != Height)
	{
		SurfaceWidth = Width;
		SurfaceHeight = Height;

		if (AlphaMaskRT) {
			AlphaMaskRT->ResizeTarget(SurfaceWidth, SurfaceHeight);
		}
	}
}

UTextureRenderTarget2D* UDungeonEditorViewportCapture::GetOrCreateAlphaMaskRenderTarget() {
	if (AlphaMaskRT == nullptr) {
		AlphaMaskRT = NewObject<UTextureRenderTarget2D>(this, TEXT("DASceneRenderer_AlphaMask"));
		AlphaMaskRT->RenderTargetFormat = RTF_R32f;
		AlphaMaskRT->ClearColor = FLinearColor::Black;
		AlphaMaskRT->InitAutoFormat(SurfaceWidth, SurfaceHeight);
		AlphaMaskRT->UpdateResourceImmediate(true);
	}

	return AlphaMaskRT;
}

void UDungeonEditorViewportCapture::SetAlphaMaskedActors(const TArray<AActor*>& InCaptureTargets) {
	AlphaMaskActorPtrs.Reset();

	for (AActor* CaptureTarget : InCaptureTargets) {
		AlphaMaskActorPtrs.Add(CaptureTarget);
	}
}

void UDungeonEditorViewportCapture::CaptureAlphaMask(const FMinimalViewInfo& InViewInfo) {
	if (UTextureRenderTarget2D* RenderTarget = GetOrCreateAlphaMaskRenderTarget())
	{
		TArray<AActor*> CaptureActors;
		for (const TWeakObjectPtr<AActor>& AlphaMaskTargetPtr : AlphaMaskActorPtrs)
		{
			if (AActor* AlphaMaskTarget = AlphaMaskTargetPtr.Get())
			{
				CaptureActors.Add(AlphaMaskTarget);
			}
		}

		CaptureScene(RenderTarget, CaptureActors, ESceneCaptureSource::SCS_SceneDepth, nullptr, InViewInfo);
	}
}

void UDungeonEditorViewportCapture::ReleaseResources() {
	if (AlphaMaskRT) {
		AlphaMaskRT->ReleaseResource();
	}
}

void UDungeonEditorViewportCapture::ReclaimResources() {
	if (AlphaMaskRT) {
		AlphaMaskRT->UpdateResource();
	}
}

bool UDungeonEditorViewportCapture::CaptureScene(UTextureRenderTarget2D* InRenderTarget, const TArray<AActor*>& InCaptureActors,
                                                 ESceneCaptureSource InCaptureSource, UMaterialInterface* InOverrideMaterial, const FMinimalViewInfo& InCaptureView) const {
	if (InRenderTarget == nullptr)
	{
		//UE_LOG(LogDAViewportCapture, Error, TEXT(""));
		return false;
	}

	if (InCaptureActors.Num() > 0) {
		TArray<UPrimitiveComponent*> PrimitiveComponents = GatherPrimitivesForCapture(InCaptureActors);
		
		TArray<UMaterialInterface*> OriginalMaterials;
		if (InOverrideMaterial)
		{
			for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
			{
				const int32 MaterialCount = PrimitiveComponent->GetNumMaterials();
				for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; MaterialIndex++)
				{
					OriginalMaterials.Add(PrimitiveComponent->GetMaterial(MaterialIndex));

					PrimitiveComponent->SetMaterial(MaterialIndex, InOverrideMaterial);
				}
			}
		}

		CaptureComponent->ShowOnlyActors = InCaptureActors;

		// We need to make sure the texture streamer takes into account this new location,
		// this request only lasts for one tick, so we call it every time we need to draw, 
		// so that they stay resident.

		CaptureComponent->TextureTarget = InRenderTarget;
		CaptureComponent->SetCameraView(InCaptureView);

		CaptureComponent->ShowFlags.SetDepthOfField(false);
		CaptureComponent->ShowFlags.SetMotionBlur(false);
		CaptureComponent->ShowFlags.SetScreenPercentage(false);
		CaptureComponent->ShowFlags.SetScreenSpaceReflections(false);
		CaptureComponent->ShowFlags.SetDistanceFieldAO(false);

		CaptureComponent->ShowFlags.SetLensFlares(false);
		CaptureComponent->ShowFlags.SetOnScreenDebug(false);
		//CaptureComponent->ShowFlags.SetEyeAdaptation(false);
		CaptureComponent->ShowFlags.SetColorGrading(false);
		CaptureComponent->ShowFlags.SetCameraImperfections(false);
		CaptureComponent->ShowFlags.SetVignette(false);
		CaptureComponent->ShowFlags.SetGrain(false);
		CaptureComponent->ShowFlags.SetSeparateTranslucency(false);
		CaptureComponent->ShowFlags.SetScreenPercentage(false);
		CaptureComponent->ShowFlags.SetScreenSpaceReflections(false);
		CaptureComponent->ShowFlags.SetTemporalAA(false);
		// might cause reallocation if we render rarely to it - for now off
		CaptureComponent->ShowFlags.SetAmbientOcclusion(false);
		// Requires resources in the FScene, which get reallocated for every temporary scene if enabled
		CaptureComponent->ShowFlags.SetIndirectLightingCache(false);
		CaptureComponent->ShowFlags.SetLightShafts(false);
		CaptureComponent->ShowFlags.SetPostProcessMaterial(false);
		CaptureComponent->ShowFlags.SetHighResScreenshotMask(false);
		CaptureComponent->ShowFlags.SetHMDDistortion(false);
		CaptureComponent->ShowFlags.SetStereoRendering(false);
		CaptureComponent->ShowFlags.SetVolumetricFog(false);
		CaptureComponent->ShowFlags.SetVolumetricLightmap(false);
		CaptureComponent->ShowFlags.SetSkyLighting(false);
		
		CaptureComponent->CaptureSource = InCaptureSource;
		CaptureComponent->ProfilingEventName = TEXT("DA Scene Capture");
		CaptureComponent->CaptureScene();

		if (OriginalMaterials.Num() > 0) {
			int32 TotalMaterialIndex = 0;
			for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents) {
				const int32 MaterialCount = PrimitiveComponent->GetNumMaterials();
				for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; MaterialIndex++) {
					PrimitiveComponent->SetMaterial(MaterialIndex, OriginalMaterials[TotalMaterialIndex]);
					TotalMaterialIndex++;
				}
			}
		}
	}

	return false;
}

TArray<UPrimitiveComponent*> UDungeonEditorViewportCapture::GatherPrimitivesForCapture(const TArray<AActor*>& InCaptureActors) const {
	const bool bIncludeFromChildActors = true;
	TArray<UPrimitiveComponent*> PrimitiveComponents;

	for (AActor* CaptureActor : InCaptureActors) {
		TArray<UPrimitiveComponent*> ChildPrimitiveComponents;
		CaptureActor->GetComponents(ChildPrimitiveComponents, bIncludeFromChildActors);

		for (UPrimitiveComponent* ChildPrimitiveComponent : ChildPrimitiveComponents) {
			if (!ChildPrimitiveComponent->bHiddenInGame) {
				PrimitiveComponents.Add(ChildPrimitiveComponent);
			}
		}
	}

	return PrimitiveComponents;
}

