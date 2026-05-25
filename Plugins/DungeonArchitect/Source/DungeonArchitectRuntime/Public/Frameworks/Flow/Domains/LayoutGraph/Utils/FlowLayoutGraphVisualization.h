//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "FlowLayoutGraphVisualization.generated.h"

class UFlowAbstractGraphBase;
class UMaterialInstanceDynamic;
class UTextRenderComponent;
class UFlowAbstractNode;
class UFlowGraphItem;

struct DUNGEONARCHITECTRUNTIME_API FDAAbstractGraphVisualizerSettings {
    float NodeRadius = 40.0f;
    float ItemScale = 0.25f;
    float LinkPadding = 10.0f;
    float LinkThickness = 8.0f;
    float LinkRefThickness = 4.0f;
    float DisabledNodeScale = 0.25f;
    float DisabledNodeOpacity = 0.3f;
    FLinearColor OneWayLinkColor = FLinearColor(0.4f, 0.05f, 0.0f);
    FVector NodeSeparationDistance = FVector(400, 400, 400);
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDAbstractNodeVisualizerResources {
    GENERATED_BODY()

    FDAbstractNodeVisualizerResources();
    
    UPROPERTY()
    TSoftObjectPtr<UMaterialInterface> DefaultMaterial;

    UPROPERTY()
    TSoftObjectPtr<UMaterialInterface> SelectedMaterial;
    
    UPROPERTY()
    TSoftObjectPtr<UMaterialInterface> TextMaterial;

    UPROPERTY()
    TSoftObjectPtr<UMaterialInterface> DoorNodeMaterial;
    
    UPROPERTY()
    TSoftObjectPtr<UMaterialInterface> BoundsMaterial;

    UPROPERTY()
    TSoftObjectPtr<UStaticMesh> PlaneMesh;
    
    UPROPERTY()
    TSoftObjectPtr<UStaticMesh> BoundsMesh;
};

UCLASS(HideDropDown, NotPlaceable, NotBlueprintable)
class DUNGEONARCHITECTRUNTIME_API UFDAbstractNodePreview : public USceneComponent {
    GENERATED_UCLASS_BODY()

public:
    void AlignToCamera(const FVector& InCameraLocation);
    void SetNodeState(const UFlowAbstractNode* InNode);
    void SetItemState(const UFlowGraphItem* InItem) const;
    void SetNodeColor(const FLinearColor& InColor) const;
    void SetOpacity(float InOpacity) const;
    void SetBorderSize(float InSize) const;
    void SetAlignToCameraEnabled(bool bEnabled) { bAlignToCamera = bEnabled; }
    void SetSelected(bool bInSelected);

private:
    void InitResources();
    
public:
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> NodeMeshComponent;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> BoundsMeshComponent;
    
    UPROPERTY()
    TObjectPtr<UTextRenderComponent> TextRendererComponent;

private:
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> DefaultMaterialInstance = nullptr;
    
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> SelectedMaterialInstance = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> BoundsMaterialInstance = nullptr;

    bool bAlignToCamera = true;
    bool bSelected = false;
    bool bActiveNode = true;
};


USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDAbstractLinkVisualizerResources {
    GENERATED_BODY()
    
    FDAbstractLinkVisualizerResources();
    
    UPROPERTY()
    TSoftObjectPtr<UMaterialInterface> LineMaterial;

    UPROPERTY()
    TSoftObjectPtr<UMaterialInterface> HeadMaterial;

    UPROPERTY()
    TSoftObjectPtr<UStaticMesh> LinkMesh;
};

UCLASS(HideDropDown, NotPlaceable, NotBlueprintable)
class DUNGEONARCHITECTRUNTIME_API UFDAbstractLinkPreview : public USceneComponent {
    GENERATED_UCLASS_BODY()
public:
    void SetState(const FVector& InStart, const FVector& InEnd, float InThickness, const FLinearColor& InColor, int32 InNumHeads = 0);
    void SetLinkVisibility(bool bInVisible);
    void SetLinkColor(const FLinearColor& InColor) const;
    void AlignToCamera(const FVector& InCameraLocation, const FDAAbstractGraphVisualizerSettings& InSettings);
    void UseHeadMaterial(int32 NumHeads) const;
    void SetDynamicAlignment(USceneComponent* Start, USceneComponent* End);

private:
    void InitResources();
    
public:
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> LineMesh;
    
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> LineMaterialInstance = nullptr;;
    
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> HeadMaterialInstance = nullptr;;

    FVector StartLocation = FVector::ZeroVector;
    FVector EndLocation = FVector::ZeroVector;
    float Thickness = 0;
    int32 NumHeads = 0;
    FLinearColor Color = FLinearColor::Black;

    TWeakObjectPtr<UFDAbstractLinkPreview> HeadComponent;
    
    bool bDynamicUpdate;
    TWeakObjectPtr<USceneComponent> DynamicCompStart;
    TWeakObjectPtr<USceneComponent> DynamicCompEnd;
    static const float MeshSize;
};

UCLASS(HideDropDown, NotPlaceable, NotBlueprintable)
class DUNGEONARCHITECTRUNTIME_API AFlowLayoutGraphVisualizer : public AActor {
    GENERATED_UCLASS_BODY()
public:
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
    virtual void Tick(float DeltaSeconds) override;

    void Generate(UFlowAbstractGraphBase* InGraph, const FDAAbstractGraphVisualizerSettings& InSettings);
    void SetAutoAlignToLevelViewport(bool bEnabled) { bAutoAlignToLevelViewport = bEnabled; }
    void AlignToCamera(const FVector& InCameraLocation) const;

    FDAAbstractGraphVisualizerSettings& GetSettings() { return Settings; }
    const FDAAbstractGraphVisualizerSettings& GetSettings() const { return Settings; }
    FVector GetNodeLocation(const UFlowAbstractNode* InNode) const;
    
public:
    UPROPERTY()
    FGuid DungeonID;
    
private:
    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot = nullptr;;

    UPROPERTY()
    bool bAutoAlignToLevelViewport = false;
    
    FDAAbstractGraphVisualizerSettings Settings;
};

