//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Utils/DungeonShapes.h"

#include "Components/PrimitiveComponent.h"
#include "DungeonCanvasRoomShapeTexture.generated.h"

class UTexture;
class UTexture2D;
class UMaterialInstanceDynamic;
class UMaterialInterface;

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDungeonCanvasRoomShapeTextureList {
	GENERATED_BODY()
	
	/** Texture shapes do not contribute to the bounds */
	UPROPERTY(EditAnywhere, Category=DungeonArchitect)
	TArray<FDAShapeTexture> TextureShapes;

	FDungeonCanvasRoomShapeTextureList TransformBy(const FTransform& InTransform) const;
};

UCLASS(Blueprintable, hidecategories=(Object,Activation,"Components|Activation"), ShowCategories=(Mobility), editinlinenew, meta=(BlueprintSpawnableComponent))
class DUNGEONARCHITECTRUNTIME_API UDungeonCanvasRoomShapeTextureComponent : public UPrimitiveComponent {
	GENERATED_BODY()
public:
	UDungeonCanvasRoomShapeTextureComponent();
	UTexture* GetOverlayTexture() const;
	
	UPROPERTY(EditAnywhere, Category="Dungeon Canvas")
	TSoftObjectPtr<UTexture2D> OverlayTexture{};
	
	UPROPERTY(EditAnywhere, Category="Dungeon Canvas")
	TSoftObjectPtr<UMaterialInterface> OverlayMaterialTemplate;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon Canvas")
	float Opacity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon Canvas")
	bool bVisualizeTexture = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon Canvas")
	bool bEditorShowOnlyWhenSelected = true;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	
private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> OverlayMaterial{};

	UPROPERTY()
	TObjectPtr<UTexture> WhiteTexture;

public:
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual bool IsEditorOnly() const override { return true; }
	
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const override;
	TObjectPtr<UMaterialInstanceDynamic> GetOverlayMaterial() const;

	//~ Begin USceneComponent Interface
	virtual void InitializeComponent() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ End USceneComponent Interface

private:
	void RecreateOverlayMaterial();
};


UCLASS(hidecategories=(Input), ConversionRoot, ComponentWrapperClass)
class DUNGEONARCHITECTRUNTIME_API ADungeonCanvasRoomShapeTextureActor : public AActor {
	GENERATED_BODY()

public:
	ADungeonCanvasRoomShapeTextureActor();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dungeon Canvas", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDungeonCanvasRoomShapeTextureComponent> OverlayTextureComponent;


#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Billboard;
#endif // WITH_EDITORONLY_DATA
};

