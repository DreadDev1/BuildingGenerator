//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/Tools/DungeonForgeToolBase.h"

#include "BaseBehaviors/BehaviorTargetInterfaces.h"
#include "DungeonForgePaintBrushTool.generated.h"

class UCanvasRenderTarget2D;
class UTextureRenderTarget2D;
class UDungeonForgePaintableTextureSettings;
class UClickDragInputBehavior;
class UDungeonForgeNodeSettings;
class AStaticMeshActor;
class ADungeon;
class UMaterialInstanceDynamic;


UCLASS()
class DUNGEONARCHITECTEDITOR_API UDungeonForgePaintBrushSettings : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="Settings")
	float BrushWorldSize = 2000.0f;

	UPROPERTY(EditAnywhere, Category="Settings", meta = (UIMin = "0.0", UIMax = "1.0"))
	float BrushHardness = 0.0f;
};

UCLASS()
class DUNGEONARCHITECTEDITOR_API UDungeonForgePaintBrushToolBuilder
	: public UDungeonForgeToolBuilderBase {
	GENERATED_BODY()
public:
	UDungeonForgePaintBrushToolBuilder();
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
	void SetState(UDungeonForgeNodeSettings* InNodeSettings, ADungeon* InDungeon);

private:
	TWeakObjectPtr<UDungeonForgeNodeSettings> NodeSettings;
	TWeakObjectPtr<ADungeon> Dungeon;
};


UCLASS()
class DUNGEONARCHITECTEDITOR_API UDungeonForgePaintBrushTool
	: public UDungeonForgeToolBase
	, public IClickDragBehaviorTarget
	, public IHoverBehaviorTarget {
	GENERATED_BODY()
	
public:
	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;
	virtual void OnTick(float DeltaTime) override;
	virtual TArray<UObject*> GetToolProperties(bool bEnabledOnly = true) const override;
	
	void SetState(UDungeonForgeNodeSettings* InNodeSettings, ADungeon* InDungeon);

	virtual FInputRayHit CanBeginClickDragSequence(const FInputDeviceRay& PressPos) override;
	virtual void OnClickPress(const FInputDeviceRay& PressPos) override;
	virtual void OnClickDrag(const FInputDeviceRay& DragPos) override;
	virtual void OnClickRelease(const FInputDeviceRay& ReleasePos) override;
	virtual void OnTerminateDragSequence() override;

	virtual FInputRayHit BeginHoverSequenceHitTest(const FInputDeviceRay& PressPos) override;
	virtual void OnBeginHover(const FInputDeviceRay& DevicePos) override;
	virtual bool OnUpdateHover(const FInputDeviceRay& DevicePos) override;
	virtual void OnEndHover() override;

	
private:
	UCanvasRenderTarget2D* GetCanvasTexture() const;
	void DrawStroke(const FVector& InLocalUV);
	bool GetLocalTextureUV(const FInputDeviceRay& DevicePos, FVector& OutUV, FHitResult* OutHitResult = nullptr) const;
	void UpdateCursorPosition(const FVector& InLocalUV) const;
	float GetLocalBrushSize() const;
	
private:
	TWeakObjectPtr<UDungeonForgeNodeSettings> TargetNodeSettings;
	TWeakObjectPtr<ADungeon> Dungeon;
	FTransform TextureWorldTransform;
	FVector PreviousStrokeUV = FVector::ZeroVector;
	bool bHovering{};
	bool bDrawing{};
	
	UPROPERTY()
	TObjectPtr<UDungeonForgePaintBrushSettings> BrushSettings;
	
	UPROPERTY()
	TObjectPtr<AStaticMeshActor> TexturePlaneMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PlaneMaterial;
	
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BrushMaterial;
};

