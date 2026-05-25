//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Minimap/DungeonMinimap2D.h"

#include "Core/Dungeon.h"
#include "Core/DungeonModel.h"
#include "Core/Layout/DungeonLayoutData.h"
#include "Frameworks/Canvas/DungeonCanvasStructs.h"
#include "Frameworks/Minimap/Builders/DungeonMinimapBuilder.h"
#include "Frameworks/Minimap/Builders/DungeonMinimapBuilderFactory.h"

#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "RenderUtils.h"
#include "TextureResource.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogDungeonMiniMap, Log, All);

ADungeonMinimapBase::ADungeonMinimapBase() {
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    if (!IsRunningCommandlet())
    {
        struct FConstructorStatics
        {
            ConstructorHelpers::FObjectFinderOptional<UTexture2D> Texture;
            FConstructorStatics()
                : Texture(TEXT("/DungeonArchitect/Core/Runtime/Assets/Materials/MiniMap/Textures/FogOfWarMask"))
            {
            }
        };
        static FConstructorStatics ConstructorStatics;

        FogOfWarExploreTexture = ConstructorStatics.Texture.Get();
    }
}

void ADungeonMinimapBase::RecreateFogOfWarTexture() {
    if (FogOfWarTexture) {
        UKismetRenderingLibrary::ReleaseRenderTarget2D(FogOfWarTexture);
    }
    FogOfWarTexture = UKismetRenderingLibrary::CreateRenderTarget2D(GetWorld(), FogOfWarTextureSize, FogOfWarTextureSize, RTF_RGBA8);
    UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), FogOfWarTexture, bEnableFogOfWar ? FLinearColor::Black : FLinearColor::White);

    UpdateFogOfWarTexture();
}

void ADungeonMinimapBase::UpdateFogOfWarTexture() const {
    if (bEnableFogOfWar && MinimapBuilder.IsValid()) {
        FDungeonMinimapBuilder::FFogOfWarUpdateSettings UpdateSettings;
        UpdateSettings.WorldToScreen = WorldToScreen;
        UpdateSettings.FogOfWarTrackingItem = FogOfWarTrackingItem;
        UpdateSettings.FogOfWarExploreTexture = FogOfWarExploreTexture;
        UpdateSettings.FogOfWarVisibilityDistance = FogOfWarVisibilityDistance;
        MinimapBuilder->UpdateFogOfWarTexture(GetWorld(), FogOfWarTexture, DynamicTracking, UpdateSettings);
    }
}


ADungeonMinimap2D::ADungeonMinimap2D() {
    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>("SceneRoot");
    RootComponent = SceneRoot;
    MaterialInstance = nullptr;
    
    if (!IsRunningCommandlet()) {
        // TODO: Place a default
        //ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialTemplateRef(TEXT("/DungeonArchitect/Core/Runtime/Assets/Materials/MiniMap/Material/M_MMTemplate_Inst"));
        //MaterialTemplate = MaterialTemplateRef.Object;
    }
    
}

UMaterialInterface* ADungeonMinimap2D::CreateMaterialInstance() {
    if (!MaterialTemplate) return nullptr;
    return CreateMaterialInstanceFromTemplate(MaterialTemplate);
}

UMaterialInterface* ADungeonMinimap2D::CreateMaterialInstanceFromTemplate(UMaterialInterface* InMaterialTemplate) {
    UMaterialInstanceDynamic* NewMaterialInstance = UMaterialInstanceDynamic::Create(InMaterialTemplate, this);
    UpdateMaterial(NewMaterialInstance);
    return NewMaterialInstance;
}

void ADungeonMinimap2D::UpdateMaterial(UMaterialInterface* InMaterial) {
    if (UMaterialInstanceDynamic* NewMaterialInstance = Cast<UMaterialInstanceDynamic>(InMaterial)) {
        NewMaterialInstance->SetTextureParameterValue("MaskTex", MaskTexture);
        NewMaterialInstance->SetTextureParameterValue("StaticOverlayTex", StaticOverlayTexture);
        NewMaterialInstance->SetTextureParameterValue("DynamicOverlayTex", DynamicOverlayTexture);
        NewMaterialInstance->SetTextureParameterValue("FogOfWarTex", FogOfWarTexture);
    }
}

void ADungeonMinimap2D::BuildLayout(ADungeon* Dungeon) {
    if (!Dungeon) {
        UE_LOG(LogDungeonMiniMap, Error, TEXT("Cannot build minimap layout. missing dungeon actor"));
        return;
    }
    
    const UDungeonModel* DungeonModel = Dungeon->GetModel();
    const UDungeonConfig* DungeonConfig = Dungeon->GetConfig();
    const UDungeonBuilder* DungeonBuilder = Dungeon->GetBuilder();
    if (!DungeonModel || !DungeonConfig || !DungeonBuilder) {
        UE_LOG(LogDungeonMiniMap, Error, TEXT("Cannot build minimap layout. Invalid dungeon state"));
        return;
    }

    MinimapBuilder = FDungeonMinimapBuilderFactory::Create(DungeonBuilder->GetClass());
    if (!MinimapBuilder.IsValid()) {
        UE_LOG(LogDungeonMiniMap, Error, TEXT("Cannot build minimap layout. Invalid minimap builder"));
        return;
    }

    UWorld* World = GetWorld();
    FDungeonLayoutData LayoutData;
    DungeonModel->GenerateLayoutData(DungeonConfig, LayoutData);

    {
        FBox WorldBounds(ForceInit);
        for (const FDungeonLayoutDataChunkInfo& ChunkShapeList : LayoutData.ChunkShapes) {
            WorldBounds += FDungeonLayoutUtils::CalculateBounds(ChunkShapeList);
        }
        
        FVector Offset = -WorldBounds.Min;
        FVector WorldSize = WorldBounds.GetSize();
        float Scale = 1.0f / FMath::Max(WorldSize.X, WorldSize.Y);
        FVector Scale3D(Scale);
        Offset *= Scale3D;
        Offset.Z = 0;

        constexpr float Padding = 0.1f;
        FTransform NormalizeTransform = FTransform(FRotator::ZeroRotator, Offset, Scale3D);
        FTransform PaddingTransform = FTransform(FRotator::ZeroRotator, FVector(Padding), FVector(1.0f - Padding * 2));
        WorldToScreen = NormalizeTransform * PaddingTransform;
    }
    
    FDungeonMinimapBuilder::FLayoutBuildSettings LayoutBuildSettings;
    LayoutBuildSettings.TextureSize = TextureSize;
    LayoutBuildSettings.OutlineThickness = OutlineThickness;
    LayoutBuildSettings.DoorThickness = DoorThickness;
    LayoutBuildSettings.BlurRadius = BlurRadius;
    LayoutBuildSettings.BlurIterations = BlurIterations;
    
    // Create the layout mask texture
    MinimapBuilder->BuildLayoutTexture(World, WorldToScreen, LayoutBuildSettings, LayoutData, DungeonModel, MaskTexture);

    // Build the static texture
    {
        if (StaticOverlayTexture) {
            UKismetRenderingLibrary::ReleaseRenderTarget2D(StaticOverlayTexture);
            StaticOverlayTexture = nullptr;
        }
        StaticOverlayTexture = UKismetRenderingLibrary::CreateRenderTarget2D(World, TextureSize, TextureSize, RTF_RGBA8);
        UKismetRenderingLibrary::ClearRenderTarget2D(World, StaticOverlayTexture, FLinearColor::Transparent);
        MinimapBuilder->BuildStaticOverlayTexture(World, WorldToScreen, StaticOverlayTexture, LayoutData, OverlayIcons);
    }

    // Build the dynamic overlay texture
    if (DynamicOverlayTexture) {
        UKismetRenderingLibrary::ReleaseRenderTarget2D(DynamicOverlayTexture);
    }
    DynamicOverlayTexture = UKismetRenderingLibrary::CreateRenderTarget2D(World, TextureSize, TextureSize, RTF_RGBA8);
    MinimapBuilder->UpdateDynamicOverlayTexture(World, WorldToScreen, DynamicOverlayTexture, DynamicTracking, OverlayIcons);
    RecreateFogOfWarTexture();

    MaterialInstance = CreateMaterialInstance();
    UpdateMaterial(MaterialInstance);
}

void ADungeonMinimap2D::BeginDestroy() {
    Super::BeginDestroy();

    if (DynamicOverlayTexture) {
        UKismetRenderingLibrary::ReleaseRenderTarget2D(DynamicOverlayTexture);
        DynamicOverlayTexture = nullptr;
    }
}

void ADungeonMinimap2D::Tick(float DeltaSeconds) {
    Super::Tick(DeltaSeconds);

    if (MinimapBuilder.IsValid()) {
        MinimapBuilder->UpdateDynamicOverlayTexture(GetWorld(), WorldToScreen, DynamicOverlayTexture, DynamicTracking, OverlayIcons);
    }
    
    UpdateFogOfWarTexture();
}

float ADungeonMinimap2D::GetIconPixelSize(const FDungeonCanvasOverlayIcon& OverlayData, const FVector2D& CanvasSize) const {
    if (OverlayData.ScreenSizeType == EDungeonCanvasIconCoordinateSystem::Pixels) {
        return OverlayData.ScreenSize * GetAttributeScaleMultiplier();
    }
    if (OverlayData.ScreenSizeType == EDungeonCanvasIconCoordinateSystem::WorldCoordinates) {
        return OverlayData.ScreenSize * WorldToScreen.GetScale3D().X * CanvasSize.X;
    }
    return 0;
}

float ADungeonMinimap2D::GetAttributeScaleMultiplier() const {
    constexpr float ReferenceTextureSize = 512;
    return TextureSize / ReferenceTextureSize;
}

