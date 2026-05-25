//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/DungeonCanvas.h"

#include "Core/Dungeon.h"
#include "Core/Layout/DungeonLayoutData.h"
#include "Core/Utils/DungeonBPFunctionLibrary.h"
#include "Frameworks/Canvas/DungeonCanvasEffect.h"
#include "Frameworks/Canvas/DungeonCanvasRendering.h"
#include "Frameworks/Canvas/DungeonCanvasTrackedObject.h"
#include "Frameworks/Canvas/Themes/DungeonCanvasMaterialTheme.h"

#include "Async/Async.h"
#include "Components/SceneComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GlobalRenderResources.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "TextureResource.h"
#include "TimerManager.h"
#include "UObject/Package.h"

class UCanvasPanel;
DEFINE_LOG_CATEGORY_STATIC(LogDungeonCanvas, Log, All)

UDungeonCanvasComponent::UDungeonCanvasComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Guid(FGuid::NewGuid())
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	bWantsInitializeComponent = true;
	
	DungeonLayoutBounds = FBox(ForceInit);

	DefaultMaterialTheme = Cast<UDungeonCanvasMaterialTheme>(StaticLoadObject(UDungeonCanvasMaterialTheme::StaticClass(), nullptr, TEXT("/DungeonArchitect/Core/Runtime/Features/DungeonCanvas/DungeonCanvasTheme.DungeonCanvasTheme")));
	TextureIconMaterialTemplate = Cast<UMaterialInterface>(StaticLoadObject(UMaterial::StaticClass(), nullptr, TEXT("/DungeonArchitect/Core/Runtime/Features/DungeonCanvas/Materials/Utils/M_IconTextureRenderer.M_IconTextureRenderer")));
}

void UDungeonCanvasComponent::InitializeComponent() {
	Super::InitializeComponent();

	// Check if we're attached to an ADungeon actor and auto-bind to its build complete event
	if (ADungeon* OwnerDungeon = Cast<ADungeon>(GetOwner())) {
		OwnerDungeon->OnDungeonBuildComplete.AddDynamic(this, &UDungeonCanvasComponent::OnOwnerDungeonBuildComplete);
	}
}

void UDungeonCanvasComponent::UninitializeComponent() {
	// Unbind from the dungeon's build complete event if we were bound
	if (ADungeon* OwnerDungeon = Cast<ADungeon>(GetOwner())) {
		OwnerDungeon->OnDungeonBuildComplete.RemoveDynamic(this, &UDungeonCanvasComponent::OnOwnerDungeonBuildComplete);
	}

	Super::UninitializeComponent();
}

void UDungeonCanvasComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetWorld()->IsGameWorld()) {
		return;
	}
	
	if (!bInitialized) {
		return;
	}
	
	if (bCanvasEnabled) {
		TMap<int32, TArray<FFogOfWarItemEntry>> FogOfWarExplorersByFloor;
		
		for (const FFogOfWarItemEntry& Explorer : FogOfWarExplorers) {
			if (Explorer.ActorPtr.IsValid()) {
				FVector WorldLocation = Explorer.ActorPtr->GetActorLocation();
				int32 FloorIndex = GetFloorIndexAtHeight(WorldLocation.Z);
				TArray<FFogOfWarItemEntry>& FloorExplorers = FogOfWarExplorersByFloor.FindOrAdd(FloorIndex);
				FloorExplorers.Add(Explorer);
			}
		}

		for (const auto& Entry : FogOfWarExplorersByFloor) {
			int32 FloorIndex = Entry.Key;
			if (LayoutRenderResources.FloorTextures.Contains(FloorIndex)) {
				const FDungeonCanvasLayoutFloorTextures& FloorTextures = LayoutRenderResources.FloorTextures[FloorIndex];
				FDungeonCanvasRenderingLibrary::BeginFogOfWarUpdate(FloorTextures.FogOfWarExplored, FloorTextures.FogOfWarVisibility);
				const TArray<FFogOfWarItemEntry>& FloorExplorers = Entry.Value;
				for (const FFogOfWarItemEntry& Explorer : FloorExplorers) {
					const FDungeonCanvasItemFogOfWarSettings& Settings = Explorer.Settings;
					FVector WorldLocation = Explorer.ActorPtr->GetActorLocation();
					FVector2D LightSource = FVector2D(WorldLocation);
					FDungeonCanvasRenderingLibrary::UpdateFogOfWarExplorer(FloorTextures.FogOfWarExplored, FloorTextures.FogOfWarVisibility, FloorTextures.SDF, FullDungeonTransform, LightSource, Settings.LightRadius, Settings.NumShadowSamples, Settings.ShadowJitterDistance);
				}
			}
		}
		
		for (const auto& Entry : LayoutRenderResources.FloorTextures) {
			int32 FloorIndex = Entry.Key;
			const FDungeonCanvasLayoutFloorTextures& FloorTextures = Entry.Value;
			FDungeonLayoutHeightRange FloorHeightRange = CreateHeightRange(FloorIndex);

			if (bRefreshSDFEveryFrame) {
				// Generate the SDF each frame
				FDungeonCanvasRenderingLibrary::UpdateDynamicOcclusions(GetWorld(), TrackedOverlayActors, LayoutRenderResources, FloorHeightRange, FloorTextures.DynamicOcclusion, FullDungeonTransform, OcclusionIcons);
				UDungeonCanvasBPFunctionLib::GenerateDungeonSDF(FloorTextures.LayoutFill, FloorTextures.LayoutBorder, FloorTextures.DynamicOcclusion, FloorTextures.SDF);
			}
		}
		/*
		{
			// Update the fog of war
			{
				TArray<FFogOfWarItemEntry> FloorFogOfWarExplorers = GetFogOfWarExplorers();
				
				FDungeonCanvasRenderingLibrary::BeginFogOfWarUpdate(FloorTextures.FogOfWarExplored, FloorTextures.FogOfWarVisibility);
				for (const FFogOfWarItemEntry& Explorer : FogOfWarExplorers) {
					if (Explorer.ActorPtr.IsValid()) {
						FVector WorldLocation = Explorer.ActorPtr->GetActorLocation();
						const FDungeonCanvasItemFogOfWarSettings& Settings = Explorer.Settings;
						const float HeightZ = WorldLocation.Z;
						FVector2D LightSource = FVector2D(WorldLocation);
						if (FloorHeightRange.InsideActiveFloorHeightRange(HeightZ)) {
							FDungeonCanvasRenderingLibrary::UpdateFogOfWarExplorer(FloorTextures.FogOfWarExplored, FloorTextures.FogOfWarVisibility, FloorTextures.SDF, FullDungeonTransform, LightSource, Settings.LightRadius, Settings.NumShadowSamples, Settings.ShadowJitterDistance);
						}
					}
				}
			}
		}
		*/
	}

	// Cleanup
	TrackedOverlayActors.RemoveAll([](const FDungeonCanvasTrackedActorRegistryItem& Item) { return !Item.TrackedComponent.IsValid(); });
	FogOfWarExplorers.RemoveAll([](const FFogOfWarItemEntry& Item) { return !Item.ActorPtr.IsValid(); });
}

void UDungeonCanvasComponent::AddTrackedOverlayActor(UDungeonCanvasTrackedObject* TrackedComponent) {
	if (!TrackedComponent) {
		return;
	}
	
	for (FDungeonCanvasTrackedActorRegistryItem& Item : TrackedOverlayActors) {
		if (Item.TrackedComponent == TrackedComponent) {
			// Already added
			return;
		}
	}
	
	// Make sure this object doesn't already exist
	FDungeonCanvasTrackedActorRegistryItem& Item = TrackedOverlayActors.AddDefaulted_GetRef();
	Item.TrackedComponent = TrackedComponent;
	Item.InstanceID = FGuid::NewGuid();
}

int32 UDungeonCanvasComponent::GetFloorIndexAtHeight(float HeightZ) const {
	FDungeonLayoutHeightRange HeightRange = CreateHeightRange(0);
	const FDungeonLayoutData& DungeonLayout = GetDungeonLayout();
	return HeightRange.GetFloorIndexAtHeight(DungeonLayout.FloorSettings, HeightZ);
}

FDungeonLayoutHeightRange UDungeonCanvasComponent::CreateHeightRange(int32 FloorIndex) const{
	FDungeonLayoutHeightRange Range;
	
	if (GetOwner()) {
		FTransform DungeonTransform = GetOwner()->GetActorTransform();
		DungeonTransform.RemoveScaling();
		Range.SetBaseHeightOffset(DungeonTransform.GetLocation().Z);
	}
	
	if (bDrawAllFloors) {
		Range.SetHeightRangeAcrossAllFloors();
	}
	else {
		const FDungeonLayoutData& DungeonLayout = GetDungeonLayout();
		Range.SetHeightRangeForSingleFloor(DungeonLayout.FloorSettings, FloorIndex);
	}
	

	return Range;
}

const FDungeonLayoutData& UDungeonCanvasComponent::GetDungeonLayout() const {
	return CachedDungeonLayout;
}

void UDungeonCanvasComponent::Initialize(ADungeon* InDungeon) {
	if (InDungeon) {
		UDungeonModel* DungeonModel = InDungeon->GetModel();
		UDungeonBuilder* DungeonBuilder = InDungeon->GetBuilder();
		if (DungeonModel && DungeonBuilder) {
			Initialize(DungeonModel->DungeonLayout, DungeonBuilder->GetCapabilities().bSupportsOverlappingFloors);
		}
	}
}

void UDungeonCanvasComponent::Initialize(const FDungeonLayoutData& InDungeonLayout, bool bSupportMultiFloorTextures) {
	CachedDungeonLayout = InDungeonLayout;
	bDrawAllFloors = !bSupportMultiFloorTextures;
	bCanvasEnabled = true;
	
	// Release existing managed resources
	ReleaseResources();
	
	if (IsRunningDedicatedServer() || !FApp::CanEverRender()) {
		bCanvasEnabled = false;
	}

	LayoutRenderResources.TextureMaterialTemplate = TextureIconMaterialTemplate;
	
	FullDungeonTransform = {};
	
	// Cache the dungeon transform
	if (bCanvasEnabled) {
		bInitialized = true;

		DungeonLayoutBounds = UDungeonBPFunctionLibrary::GenerateDungeonLayoutBounds(CachedDungeonLayout, LayoutDrawMarginPercent);
		const float TextureSizeF = static_cast<float>(LayoutTextureSize);
		FullDungeonTransform.FocusOnCanvas(TextureSizeF, TextureSizeF);
		FullDungeonTransform.SetLocalToWorld(FTransform(FRotator::ZeroRotator, DungeonLayoutBounds.GetCenter(), DungeonLayoutBounds.GetSize()));
	}

	if (!bGenerateFloorTexturesOnDemand) {
		GenerateAllFloorTextures();
	}
}

void UDungeonCanvasComponent::ReleaseResources() {
	DestroyManagedResources();
}

void UDungeonCanvasComponent::Draw(const FDungeonCanvasDrawContext& DrawContext) {
	LayoutRenderResources.MaterialPool.UnlockAll();

	if (bCanvasEnabled && DrawContext.ThemeRenderResources) {
		if (UMaterialInstanceDynamic* ThemeMaterial = DrawContext.ThemeRenderResources->ThemeMaterialInstance) {
			const int32 FloorIndex = bDrawAllFloors ? 0 : DrawContext.FloorIndex;
			GenerateFloorTextures(FloorIndex);

			if (LayoutRenderResources.FloorTextures.Contains(FloorIndex)) {
				FDungeonLayoutHeightRange FloorHeightRange = CreateHeightRange(FloorIndex);
				const FDungeonCanvasLayoutFloorTextures& FloorTextures = LayoutRenderResources.FloorTextures[FloorIndex];
				
				auto FuncSetupMaterial = [&](UMaterialInstanceDynamic* MID) {
					FDungeonCanvasRenderingLibrary::SetupMaterialParameters(MID, FloorTextures, DrawContext.ViewTransform, FullDungeonTransform);
				};
				
				// Initialize the canvas theme material
				{
					FDungeonCanvasRenderingLibrary::SetupMaterialFloorTextures(ThemeMaterial, DrawContext.ThemeRenderResources->FogOfWarMaterialInstance, FloorTextures);

					bool bFogOfWarEnabled = DrawContext.DrawSettings.bFogOfWarEnabled;
					auto bFullyExplored = DrawContext.DrawSettings.bFogOfWarFullyExplored;
					FDungeonCanvasRenderingLibrary::SetupMaterialFogOfWarState(ThemeMaterial, DrawContext.ThemeRenderResources->FogOfWarMaterialInstance, bFogOfWarEnabled, bFullyExplored);

					if (DrawContext.Theme) {
						for (UDungeonCanvasEffectBase* Effect : DrawContext.Theme->Effects) {
							if (Effect) {
								Effect->InitCanvasMaterial(ThemeMaterial);
								Effect->Draw();	// TODO: Merge init and draw and make it stateless
							}
						}
					}
				}

				const FDungeonLayoutData& DungeonLayout = GetDungeonLayout();
				
				if (UDungeonCanvasMaterialTheme* Theme = DrawContext.Theme) {
					const FTransform WorldBounds = FullDungeonTransform.GetLocalToWorld();
					
					// Render the theme material on to the canvas
					FDungeonCanvasRenderingLibrary::DungeonCanvasDrawMaterial(DrawContext.CanvasLayout, DrawContext.ViewTransform, ThemeMaterial, WorldBounds, EDungeonCanvasDrawMaterialBlendMode::Translucent);

					//DrawContext.CanvasLayout->Clear( DrawContext.DrawSettings.BackgroundColor );

					{
						// Draw the static room icons
						FDungeonCanvasRenderingLibrary::DungeonCanvasDrawLayoutIcons(DrawContext.CanvasLayout, LayoutRenderResources, FuncSetupMaterial, DrawContext.ViewTransform, FloorHeightRange, DungeonLayout, Theme->OverlayIcons, Theme->OverlayIconOpacity);

						// Draw the stair icons
						constexpr float StairIconOpacity = 1.0f;
						FDungeonCanvasRenderingLibrary::DungeonCanvasDrawStairIcons(DrawContext.CanvasLayout, LayoutRenderResources, FuncSetupMaterial, DrawContext.ViewTransform, FloorHeightRange, DungeonLayout, Theme->StairIcon, StairIconOpacity);

						// Draw the dynamic overlay icons
						constexpr float TrackedActorsIconOpacity = 1.0f;
						FDungeonCanvasRenderingLibrary::DungeonCanvasDrawTrackedActorIcons(DrawContext.CanvasLayout, LayoutRenderResources, FuncSetupMaterial, DrawContext.ViewTransform, TrackedOverlayActors,
								DrawContext.DrawSettings.bFogOfWarEnabled, FloorHeightRange, Theme->OverlayIcons, TrackedActorsIconOpacity, DrawContext.DrawSettings.OverlayActorIconFilters);
					}

					// Render fog of war
					if (DrawContext.DrawSettings.bFogOfWarEnabled && DrawContext.ThemeRenderResources->FogOfWarMaterialInstance) {
						FDungeonCanvasRenderingLibrary::DungeonCanvasDrawMaterial(DrawContext.CanvasLayout, DrawContext.ViewTransform, DrawContext.ThemeRenderResources->FogOfWarMaterialInstance, WorldBounds);
					}
				}
			}
		}
	}
}

void UDungeonCanvasComponent::SetupMaterialParameters(const TObjectPtr<UMaterialInstanceDynamic>& InMaterialInstance, const FDungeonCanvasViewportTransform& InViewTransform, int32 InFloorIndex) {
	int32 FloorIndex = bDrawAllFloors ? 0 : InFloorIndex;
	if (LayoutRenderResources.FloorTextures.Contains(FloorIndex)) {
		FDungeonLayoutHeightRange FloorHeightRange = CreateHeightRange(FloorIndex);
		const FDungeonCanvasLayoutFloorTextures& FloorTextures = LayoutRenderResources.FloorTextures[FloorIndex];
		FDungeonCanvasRenderingLibrary::SetupMaterialParameters(InMaterialInstance, FloorTextures, InViewTransform, FullDungeonTransform);
	}
}

void UDungeonCanvasComponent::DestroyManagedResources() {
	FDungeonCanvasRenderingLibrary::ReleaseDungeonCanvasLayoutResources(LayoutRenderResources);
	LayoutRenderResources.Reset();
}

void UDungeonCanvasComponent::AddFogOfWarExplorer(AActor* Actor, FDungeonCanvasItemFogOfWarSettings Settings) {
	for (FFogOfWarItemEntry& ExistingEntry : FogOfWarExplorers) {
		if (ExistingEntry.ActorPtr == Actor) {
			ExistingEntry.Settings = Settings;
			return;
		}
	}
	
	FogOfWarExplorers.Add({Actor, Settings});
}

void UDungeonCanvasComponent::GenerateFloorTextures(int32 NewFloorIndex) {
	FDungeonLayoutHeightRange HeightRange = CreateHeightRange(NewFloorIndex);
	const int32 FloorIndex = bDrawAllFloors ? 0 : NewFloorIndex;
	if (!LayoutRenderResources.FloorTextures.Contains(FloorIndex)) {
		// Generate the floor layout textures
		FDungeonCanvasLayoutFloorTextures& FloorTextures = LayoutRenderResources.FloorTextures.FindOrAdd(FloorIndex);

		GenerateFloorTextures(FloorTextures, HeightRange);
	}
}

void UDungeonCanvasComponent::GenerateFloorTextures(FDungeonCanvasLayoutFloorTextures& FloorTextures, const FDungeonLayoutHeightRange& InHeightRange) {
	// Create the floor texture objects in memory, so we can later write to it
	FDungeonCanvasRenderingLibrary::CreateDungeonCanvasLayoutFloorTextures(this, LayoutTextureSize, LayoutTextureSize, FloorTextures);

	// Draw the fill and border layout textures
	const FDungeonLayoutData& DungeonLayout = GetDungeonLayout();
	FDungeonCanvasRenderingLibrary::DrawDungeonLayout(GetWorld(), DungeonLayout, FullDungeonTransform, InHeightRange, FloorTextures.GroundHeightBias, FloorTextures.LayoutFill, FloorTextures.LayoutBorder, FloorTextures.GroundHeightMin, FloorTextures.GroundHeightMax);

	// Draw the SDF texture
	UDungeonCanvasBPFunctionLib::GenerateDungeonSDF(FloorTextures.LayoutFill, FloorTextures.LayoutBorder, FloorTextures.DynamicOcclusion, FloorTextures.SDF);
}

void UDungeonCanvasComponent::GenerateAllFloorTextures() {
	if (bDrawAllFloors) {
		GenerateFloorTextures(0);
	}
	else {
		const FDungeonLayoutData& DungeonLayout = GetDungeonLayout();
		const int32 FloorLowestIdx = DungeonLayout.FloorSettings.FloorLowestIndex;
		const int32 FloorHighestIdx = DungeonLayout.FloorSettings.FloorHighestIndex;
		for (int FloorIdx = FloorLowestIdx; FloorIdx < FloorHighestIdx; ++FloorIdx) {
			GenerateFloorTextures(FloorIdx);
		}
	}
}

const FDungeonCanvasLayoutFloorTextures* UDungeonCanvasComponent::GetFloorTextures(int32 FloorIndex) {
	return LayoutRenderResources.FloorTextures.Find(FloorIndex);
}

void UDungeonCanvasComponent::NotifyCanvasRebuildRequested() {
	OnCanvasRebuildRequested.Broadcast(this);
}

void UDungeonCanvasComponent::OnOwnerDungeonBuildComplete(ADungeon* Dungeon, bool bSuccess) {
	if (bSuccess) {
		OnCanvasRebuildRequested.Broadcast(this);
	}
}

