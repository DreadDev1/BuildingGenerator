//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "DungeonEditorViewportProperties.generated.h"

class UPCGGraph;
class UDungeonConfig;
class UDungeonBuilder;
class UDungeonMarkerEmitter;
class UProceduralMarkerEmitter;

class IDungeonEditorViewportPropertiesListener {
public:
    virtual ~IDungeonEditorViewportPropertiesListener() = default;
    virtual void OnPropertyChanged(FString PropertyName, class UDungeonEditorViewportProperties* Properties) = 0;
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDungeonEditorViewportCamTransform {
    GENERATED_BODY()
    
    UPROPERTY()
    FVector LookAt = FVector::Zero();
    
    UPROPERTY()
    FVector Location = FVector::Zero();

    UPROPERTY()
    FRotator Rotation = FRotator::ZeroRotator;
};

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonEditorViewportProperties : public UObject {
    GENERATED_BODY()

public:
    UDungeonEditorViewportProperties(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(BlueprintReadWrite, Category = Config)
    TObjectPtr<UDungeonConfig> DungeonConfig = nullptr;;

    /** Lets you swap out the default dungeon builder with your own implementation */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder")
    TSubclassOf<UDungeonBuilder> BuilderClass;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder")
    TArray<TObjectPtr<UPCGGraph>> PCGGraphs;

    /** Lets you emit your own markers into the scene */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder")
    TArray<TObjectPtr<UDungeonMarkerEmitter>> MarkerEmitters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder")
    TArray<TObjectPtr<UProceduralMarkerEmitter>> ProceduralMarkerEmitters;

    UPROPERTY()
    bool bViewTransformValid = false;
    
    UPROPERTY()
    FDungeonEditorViewportCamTransform ViewTransform;
        
#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
    void PostEditChangeConfigProperty(struct FPropertyChangedEvent& e);
#endif // WITH_EDITOR

    TWeakPtr<IDungeonEditorViewportPropertiesListener> PropertyChangeListener;
};

