//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Editors/ThemeEditor/AppModes/MarkerGenerator/PatternEditor/Impl/Grid/GridPatternEditorSettings.h"
#include "Core/Utils/MathUtils.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "GameFramework/Actor.h"
#include "GridPatternEdActors.generated.h"

class UCanvasRenderTarget2D;
class UMarkerGenGridPatternRule;
enum class EMarkerGenGridPatternRuleType : uint8;
class UProceduralMeshComponent;
struct FMGGridPatternEdGroundSettings;
class UFont;

UCLASS()
class DUNGEONARCHITECTEDITOR_API UGridPatternEdGroundItem : public UStaticMeshComponent {
	GENERATED_BODY()
public:
	UGridPatternEdGroundItem();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void Initialize(const FVector& InLocation, const FVector2D& InScale);
	void SetBorderColor(const FLinearColor& InColor) const;
	void SetColor(const FLinearColor& InColor) const;
	void SetBorderThickness(int32 InThickness) const;

	void OnHover();

public:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance = nullptr;;

	UPROPERTY()
	TObjectPtr<UCurveFloat> HoverAnimation = nullptr;;

	UPROPERTY()
	FIntPoint Coord;
	
	UPROPERTY()
	EMarkerGenGridPatternRuleType ItemType;

	FLinearColor BodyColor = FLinearColor::White;
	FLinearColor BorderColor = FLinearColor::Black;
	FLinearColor HoverBodyColor = FLinearColor::White;
	FLinearColor HoverBorderColor = FLinearColor::Black;
	float BorderThickness = 0;
	float HoverBorderThickness = 0;
	float HoverAnimationTime = 0;

private:
	float HoverEnergy = 0.0f;
	float HoverEnergySmooth = 0.0f;
};


UCLASS()
class DUNGEONARCHITECTEDITOR_API AGridPatternEdCursorActor : public AActor {
	GENERATED_BODY()
public:
	AGridPatternEdCursorActor();
	virtual void Tick(float DeltaSeconds) override;
	
	void SetCursorLocation(const FVector& InLocation);

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> CursorMesh = nullptr;;
	
	FVector CursorLocation;
	FVector SnapLocation;
	bool bSnapPointVisible;
};

UCLASS()
class DUNGEONARCHITECTEDITOR_API AGridPatternEdRuleActor : public AActor {
	GENERATED_BODY()
public:
	AGridPatternEdRuleActor();
	virtual void Tick(float DeltaSeconds) override;
	virtual void Destroyed() override;
	
	void Initialize(const FMGGridPatternEdGroundSettings& InSettings, UMarkerGenGridPatternRule* InRule);
	void UpdateState(const FIntPoint& InCoord, EMarkerGenGridPatternRuleType InItemType, bool bAnimate);
	void UpdateTransform(bool bAnimate);
	void UpdateColor() const;
	void SetSelected(bool bInSelected) const;
	void RefreshCanvas();

	UMarkerGenGridPatternRule* GetRule() const { return Rule; }
	void SetRule(UMarkerGenGridPatternRule* InRule) { Rule = InRule; }
	
private:
	void SetColor(const FLinearColor& InColor, bool bVisuallyDominant) const;
	
	UFUNCTION()
	void RenderCanvas(UCanvas* Canvas, int32 Width, int32 Height);
	//FVector GetScale(EMarkerGenGridPatternRuleType InItemType) const;
	
public:
	UPROPERTY()
	FLinearColor Color = FLinearColor::White;


	UPROPERTY()
	TObjectPtr<UCanvasRenderTarget2D> CanvasTexture = nullptr;;
	
private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> MeshComponent = nullptr;;
	
	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot = nullptr;;
	
	UPROPERTY()
	TObjectPtr<UMarkerGenGridPatternRule> Rule = nullptr;;
	
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance = nullptr;;

	UPROPERTY(Transient)
	TObjectPtr<UFont> Font = nullptr;;
	
	UPROPERTY(Transient)
	TObjectPtr<UFont> FontTitle = nullptr;;
	
	FMGGridPatternEdGroundSettings Settings;
	TDASmoothValue<FVector> DesiredLocation;
	TDASmoothValue<FVector> DesiredScale;
	TArray<FString> ActionTextList;
	FString ConditionText;
	
};

UCLASS()
class DUNGEONARCHITECTEDITOR_API AGridPatternEdGroundActor : public AActor {
	GENERATED_BODY()
public:
	AGridPatternEdGroundActor();
	void Build(const FMGGridPatternEdGroundSettings& InSettings);
	void OnMouseHover(const FVector& InWorldIntersection);
	bool Deproject(const FVector& InWorldIntersection, UGridPatternEdGroundItem*& OutItem, EMarkerGenGridPatternRuleType& OutItemType);
	
private:
	UGridPatternEdGroundItem* CreateItem(const FMGGridPatternEdGroundSettings& InSettings, const struct FMGGridPatternGroundMeshItemColors& InColors,
	                                     const FVector& InLocation, const FVector2D& InScale);

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot;
	
	UPROPERTY()
	TMap<FIntPoint, TObjectPtr<UGridPatternEdGroundItem>> Tiles;

	UPROPERTY()
	TMap<FIntPoint, TObjectPtr<UGridPatternEdGroundItem>> XEdges;

	UPROPERTY()
	TMap<FIntPoint, TObjectPtr<UGridPatternEdGroundItem>> YEdges;
	
	UPROPERTY()
	TMap<FIntPoint, TObjectPtr<UGridPatternEdGroundItem>> Corners;

	FMGGridPatternEdGroundSettings Settings;
};

