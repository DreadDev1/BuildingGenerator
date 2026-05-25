//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionComponent.h"

#include "GameFramework/Actor.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "SnapConnectionActor.generated.h"

class AActor;
class ADungeon;
class UArrowComponent;
class UDecalComponent;
class UBillboardComponent;
class UDecalComponent;
class UTexture2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;


UCLASS(Blueprintable, ConversionRoot, ComponentWrapperClass)
class DUNGEONARCHITECTRUNTIME_API ASnapConnectionActor : public AActor {
    GENERATED_UCLASS_BODY()

public:
    UPROPERTY()
    TObjectPtr<USnapConnectionComponent> ConnectionComponent;

private:
    UPROPERTY()
    FGuid ConnectionId;

public:
    virtual void PostLoad() override;
    virtual void PostActorCreated() override;
    virtual bool IsLevelBoundsRelevant() const override { return false; }
    virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;

    FGuid GetConnectionId() const { return ConnectionId; }

#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& e) override;
#endif

#if WITH_EDITORONLY_DATA
    UTexture2D* GetConstraintTexture(ESnapConnectionConstraint ConnectionConstraint);
    void UpdateConstraintIcon();
    void UpdateConstraintDecal();
#endif	// WITH_EDITORONLY_DATA

public:
    void BuildConnectionInstance(ADungeon* InDungeon, const FGuid& OwningModuleId, const FGuid& OtherModuleId, ULevel* InHostLevel = nullptr);
    void DestroyConnectionInstance();
    void SetHiddenInGame(bool bInHidden);
    TArray<TWeakObjectPtr<AActor>> GetSpawnedInstancesPtr() const { return SpawnedInstances; }
    
private:
    void Initialize();
    void BuildImpl(int32 InSeed, ADungeon* InDungeon, const FGuid& OwningModuleId, const FGuid& OtherModuleId, ULevel* HostLevel = nullptr);
    void BuildImplDeprecated(ULevel* HostLevel = nullptr);
    void CreateInstanceDeprecated(const struct FSnapConnectionVisualInfo_DEPRECATED& VisualInfo, ULevel* HostLevel = nullptr);

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dungeon")
    TArray<AActor*> GetSpawnedInstances() const;
    
private:
    TArray<TWeakObjectPtr<AActor>> SpawnedInstances;

#if WITH_EDITORONLY_DATA
    UPROPERTY()
    TObjectPtr<UBillboardComponent> DoorSpriteComponent;

    UPROPERTY()
    TObjectPtr<UBillboardComponent> ConnectionConstraintSpriteComponent;

    UPROPERTY()
    TObjectPtr<UDecalComponent> ConstraintDebugDecal;

    UPROPERTY(Transient)
    TObjectPtr<UArrowComponent> ArrowComponent;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> IconConstraintMagnet;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> IconConstraintPlugMale;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> IconConstraintPlugFemale;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> IconConstraintRoomEntry;
    
    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> IconConstraintRoomExit;
    
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> MaterialDebugDecal;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> MaterialDebugDecalInstance;


#endif //WITH_EDITORONLY_DATA
};

