//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Layout/DungeonLayoutData.h"
#include "Frameworks/Canvas/DungeonCanvasRendering.h"
#include "Frameworks/Canvas/DungeonCanvasStructs.h"
#include "Frameworks/Canvas/DungeonCanvasViewport.h"

#include "Components/SceneComponent.h"
#include "Curves/CurveFloat.h"
#include "DungeonCanvas.generated.h"

class ADungeon;
class UDungeonCanvasMaterialTheme;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDungeonCanvasRebuildRequestedEvent, UDungeonCanvasComponent*, Canvas);

UCLASS(Blueprintable, BlueprintType, meta=(BlueprintSpawnableComponent), hidecategories = (Rendering,Input,Actor,Misc,Replication,Collision,LOD,Cooking,HLOD,Physics,Networking,Tags,Activation,AssetUserData,Navigation))
class DUNGEONARCHITECTRUNTIME_API UDungeonCanvasComponent : public USceneComponent {
	GENERATED_BODY()
public:
	UDungeonCanvasComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UFUNCTION()
	void Initialize(ADungeon* InDungeon);
	
	void Initialize(const FDungeonLayoutData& InDungeonLayout, bool bSupportMultiFloorTextures);

	UFUNCTION()
	void ReleaseResources();

	void Draw(const FDungeonCanvasDrawContext& DrawContext);
	void SetupMaterialParameters(const TObjectPtr<UMaterialInstanceDynamic>& InMaterialInstance, const FDungeonCanvasViewportTransform& InViewTransform, int32 InFloorIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="DungeonCanvas")
	const TArray<FDungeonCanvasTrackedActorRegistryItem>& GetTrackedOverlayActors() const { return TrackedOverlayActors; }

	UFUNCTION(BlueprintCallable, Category="DungeonCanvas")
	void AddTrackedOverlayActor(UDungeonCanvasTrackedObject* TrackedComponent);

	UFUNCTION(BlueprintCallable, Category="DungeonCanvas")
	int32 GetFloorIndexAtHeight(float HeightZ) const;

	UFUNCTION(BlueprintCallable, Category = "DungeonCanvas")
	void AddFogOfWarExplorer(AActor* Actor, FDungeonCanvasItemFogOfWarSettings Settings);
	
	UFUNCTION(BlueprintCallable, Category = "DungeonCanvas")
	void GenerateFloorTextures(int32 NewFloorIndex);

	void GenerateFloorTextures(FDungeonCanvasLayoutFloorTextures& FloorTextures, const FDungeonLayoutHeightRange& InHeightRange);

	UFUNCTION(BlueprintCallable, Category = "DungeonCanvas")
	void GenerateAllFloorTextures();

	/** Manually notify that the canvas should rebuild. Use this when the component is not attached to an ADungeon actor */
	UFUNCTION(BlueprintCallable, Category = "DungeonCanvas")
	void NotifyCanvasRebuildRequested();

	void SetCanvasEnabled(bool bValue) { bCanvasEnabled = bValue; }
	const FDungeonCanvasLayoutFloorTextures* GetFloorTextures(int32 FloorIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="DungeonCanvas")
	FDungeonCanvasViewportTransform GetFullDungeonViewTransform() const { return FullDungeonTransform; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="DungeonCanvas")
	FBox GetDungeonLayoutBounds() const { return DungeonLayoutBounds; }
	
	struct FFogOfWarItemEntry {
		/** The actor to track */
		TWeakObjectPtr<AActor> ActorPtr;
		
		/** The settings used to update the fog of war texture for this actor */
		FDungeonCanvasItemFogOfWarSettings Settings;
	};
	const TArray<FFogOfWarItemEntry>& GetFogOfWarExplorers() const { return FogOfWarExplorers; }
	
public:
	/** Broadcast when the canvas should rebuild. If attached to an ADungeon actor, this is automatically triggered when the dungeon rebuilds */
	UPROPERTY(BlueprintAssignable, Category = "DungeonCanvas")
	FDungeonCanvasRebuildRequestedEvent OnCanvasRebuildRequested;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DungeonCanvas")
	TObjectPtr<UDungeonCanvasMaterialTheme> DefaultMaterialTheme;

	// Layout Texture Properties
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DungeonCanvas")
	int32 LayoutTextureSize = 2048;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DungeonCanvas", meta = (UIMin = "0.0", UIMax = "100.0"))
	float LayoutDrawMarginPercent = 20;
	
	/** Icons mappings for dynamic occlusion. provide icon textures here that you want to use to block fog of war visibility */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DungeonCanvas", meta = (UIMin = "0.0", UIMax = "100.0"))
	TArray<FDungeonCanvasOverlayIcon> OcclusionIcons;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DungeonCanvas")
	bool bRefreshSDFEveryFrame = true;

	/** Only create the textures when you need them.  You might want to disable this to force all floor texture and fog of war texture to be created when the dungeon builds */   
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DungeonCanvas")
	bool bGenerateFloorTexturesOnDemand = true;
	
private:
	/** The unique ID of this dungeon canvas */
	UPROPERTY()
	FGuid Guid;

	UPROPERTY(Transient)
	TArray<FDungeonCanvasTrackedActorRegistryItem> TrackedOverlayActors;

	UPROPERTY(Transient)
	FDungeonCanvasLayoutRenderResources LayoutRenderResources;
	
	UPROPERTY()
	FDungeonCanvasViewportTransform FullDungeonTransform;
	
	UPROPERTY()
	FBox DungeonLayoutBounds;
	
	UPROPERTY()
	TObjectPtr<UMaterialInterface> TextureIconMaterialTemplate;
	
	TArray<FFogOfWarItemEntry> FogOfWarExplorers;
	
	UPROPERTY()
	bool bDrawAllFloors = true;

	UPROPERTY()
	FDungeonLayoutData CachedDungeonLayout;
	
private:
	void DestroyManagedResources();
	FDungeonLayoutHeightRange CreateHeightRange(int32 FloorIndex) const;
	const FDungeonLayoutData& GetDungeonLayout() const;

	UFUNCTION()
	void OnOwnerDungeonBuildComplete(ADungeon* Dungeon, bool bSuccess);
	
private:
	bool bInitialized{};
	bool bCanvasEnabled = true;
};

