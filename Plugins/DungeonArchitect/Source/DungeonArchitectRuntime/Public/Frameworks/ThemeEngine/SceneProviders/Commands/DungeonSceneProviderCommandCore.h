//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Actors/DungeonActorTemplate.h"
#include "Core/Actors/DungeonMesh.h"
#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommand.h"
#include "Frameworks/Voxel/VDB/VDBUtils.h"

#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Particles/ParticleSystemComponent.h"

class UDungeonActorTemplate;
namespace DA {
    typedef TSharedPtr<struct FDungeonMeshGeometry, ESPMode::ThreadSafe> FDungeonMeshGeometryPtr;
}


class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_CreateMesh : public FDungeonSceneProviderCommand {
public:
    FDungeonSceneProviderCommand_CreateMesh(const FDungeonSceneProviderContext& InContext, UDungeonMesh* InMesh)
        : FDungeonSceneProviderCommand(InContext), Mesh(InMesh) {
    }

    static void SetMeshComponentAttributes(UStaticMeshComponent* StaticMeshComponent,
                                           UStaticMeshComponent* StaticMeshTemplate);

    virtual void UpdateExecutionPriority(const FVector& BuildPosition) override {
        UpdateExecutionPriorityByDistance(BuildPosition, Context.Transform);
    }

protected:
    virtual void ExecuteImpl(UWorld* World) override;

private:
    TWeakObjectPtr<UDungeonMesh> Mesh;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_CreateMeshSimple : public FDungeonSceneProviderCommand {
public:
    FDungeonSceneProviderCommand_CreateMeshSimple(const FDungeonSceneProviderContext& InContext, UStaticMesh* InMesh, bool bInCanAffectNavigation)
        : FDungeonSceneProviderCommand(InContext)
        , Mesh(InMesh)
        , bCanAffectNavigation(bInCanAffectNavigation)
    {
    }

    virtual void UpdateExecutionPriority(const FVector& BuildPosition) override {
        UpdateExecutionPriorityByDistance(BuildPosition, Context.Transform);
    }

protected:
    virtual void ExecuteImpl(UWorld* World) override;

private:
    TWeakObjectPtr<UStaticMesh> Mesh;
    bool bCanAffectNavigation{};
};

class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_AddLight : public FDungeonSceneProviderCommand {
public:
    FDungeonSceneProviderCommand_AddLight(const FDungeonSceneProviderContext& InContext, UPointLightComponent* pLightTemplate)
        : FDungeonSceneProviderCommand(InContext), LightTemplate(pLightTemplate) {
    }

    static void SetSpotLightAttributes(USpotLightComponent* SpotLightComponent, USpotLightComponent* SpotLightTemplate);
    static void SetPointLightAttributes(UPointLightComponent* PointLightComponent, UPointLightComponent* LightTemplate);

    virtual void UpdateExecutionPriority(const FVector& BuildPosition) override {
        UpdateExecutionPriorityByDistance(BuildPosition, Context.Transform);
    }

protected:
    virtual void ExecuteImpl(UWorld* World) override;

private:
    TWeakObjectPtr<UPointLightComponent> LightTemplate;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_AddParticleSystem : public FDungeonSceneProviderCommand {
public:
    FDungeonSceneProviderCommand_AddParticleSystem(const FDungeonSceneProviderContext& InContext, UParticleSystem* pParticleTemplate)
        : FDungeonSceneProviderCommand(InContext), ParticleTemplate(pParticleTemplate) {
    }

    virtual void UpdateExecutionPriority(const FVector& BuildPosition) override {
        UpdateExecutionPriorityByDistance(BuildPosition, Context.Transform);
    }

protected:
    virtual void ExecuteImpl(UWorld* World) override;

private:
    TWeakObjectPtr<UParticleSystem> ParticleTemplate;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_AddActor : public FDungeonSceneProviderCommand {
public:
    FDungeonSceneProviderCommand_AddActor(const FDungeonSceneProviderContext& InContext, UClass* InClassTemplate, bool bInCanAffectNavigation)
        : FDungeonSceneProviderCommand(InContext)
        , ClassTemplate(InClassTemplate)
        , bCanAffectNavigation(bInCanAffectNavigation)
    {
    }

    virtual void UpdateExecutionPriority(const FVector& BuildPosition) override {
        UpdateExecutionPriorityByDistance(BuildPosition, Context.Transform);
    }

protected:
    virtual void ExecuteImpl(UWorld* World) override;

private:
    TWeakObjectPtr<UClass> ClassTemplate;
    bool bCanAffectNavigation;
};


class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_AddMeshGeometry : public FDungeonSceneProviderCommand {
public:
    struct FSettings {
        DA::FDungeonMeshGeometryPtr Geometry;
        TWeakObjectPtr<UMaterialInterface> Material;
        bool bEnableCollision{};
        float UVScale = 100.0f;
        TArray<FName> ActorTags;
    };
    
    FDungeonSceneProviderCommand_AddMeshGeometry(const FDungeonSceneProviderContext& InContext, const FSettings& InSettings)
        : FDungeonSceneProviderCommand(InContext)
        , Settings(InSettings)
    {
    }

    virtual void UpdateExecutionPriority(const FVector& BuildPosition) override;

protected:
    virtual void ExecuteImpl(UWorld* World) override;
    void ApplyBoxProjectionUVs(class UDynamicMeshComponent* MeshComponent);

private:
    FSettings Settings;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_CreateGroupActor : public FDungeonSceneProviderCommand {
public:
    FDungeonSceneProviderCommand_CreateGroupActor(const FDungeonSceneProviderContext& InContext, const TArray<FName>& InActorNodeIds)
        : FDungeonSceneProviderCommand(InContext)
        , ActorNodeIds(InActorNodeIds)
    {
    }

    virtual void UpdateExecutionPriority(const FVector& BuildPosition) override {
        ExecutionPriority = MAX_int32;
    }

protected:
    virtual void ExecuteImpl(UWorld* World) override;

private:
    TArray<FName> ActorNodeIds;
};


class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_CloneActor : public FDungeonSceneProviderCommand {
public:
    FDungeonSceneProviderCommand_CloneActor(const FDungeonSceneProviderContext& InContext, UDungeonActorTemplate* InActorTemplate)
        : FDungeonSceneProviderCommand(InContext)
        , ActorTemplate(InActorTemplate)
    {
    }

    virtual void UpdateExecutionPriority(const FVector& BuildPosition) override {
        UpdateExecutionPriorityByDistance(BuildPosition, Context.Transform);
    }
    
protected:
    virtual void ExecuteImpl(UWorld* World) override;

private:
    TWeakObjectPtr<UDungeonActorTemplate> ActorTemplate;
};


class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_SetActorTransform : public FDungeonSceneProviderCommand {
public:
    FDungeonSceneProviderCommand_SetActorTransform(const FDungeonSceneProviderContext& InContext, AActor* InActor)
        : FDungeonSceneProviderCommand(InContext)
        , Actor(InActor)
    {
    }

    virtual void UpdateExecutionPriority(const FVector& BuildPosition) override {
        UpdateExecutionPriorityByDistance(BuildPosition, Context.Transform);
    }

protected:
    virtual void ExecuteImpl(UWorld* World) override;

private:
    TWeakObjectPtr<AActor> Actor;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_DestroyActor : public FDungeonSceneProviderCommand {
public:
    FDungeonSceneProviderCommand_DestroyActor(AActor* pActor)
        : FDungeonSceneProviderCommand(FDungeonSceneProviderContext())
        , Actor(pActor)
    {
    }

protected:
    virtual void ExecuteImpl(UWorld* World) override;

private:
    TWeakObjectPtr<AActor> Actor;
};


class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_DestroyActorWithTag : public FDungeonSceneProviderCommand {
public:
    FDungeonSceneProviderCommand_DestroyActorWithTag(const FName& InTag)
        : FDungeonSceneProviderCommand(FDungeonSceneProviderContext())
        , Tag(InTag)
    {
    }

    virtual void UpdateExecutionPriority(const FVector& BuildPosition) override {
        ExecutionPriority = MAX_int32;
    }

protected:
    virtual void ExecuteImpl(UWorld* World) override;

private:
    FName Tag;
};


class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_ReuseActor : public FDungeonSceneProviderCommand {
public:
    FDungeonSceneProviderCommand_ReuseActor(const FDungeonSceneProviderContext& InContext, AActor* InActorToReuse)
        : FDungeonSceneProviderCommand(InContext)
        , ActorToReuse(InActorToReuse)
    {
    }

    virtual void UpdateExecutionPriority(const FVector& BuildPosition) override {
        UpdateExecutionPriorityByDistance(BuildPosition, Context.Transform);
    }

protected:
    virtual void ExecuteImpl(UWorld* World) override;

protected:
    TWeakObjectPtr<AActor> ActorToReuse;
    bool bRerunConstructionScripts = true;
};


class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_ReuseStaticMesh : public FDungeonSceneProviderCommand_ReuseActor {
public:
    FDungeonSceneProviderCommand_ReuseStaticMesh(const FDungeonSceneProviderContext& InContext, AActor* InActorToReuse, UDungeonMesh* InMesh)
        : FDungeonSceneProviderCommand_ReuseActor(InContext, InActorToReuse)
        , Mesh(InMesh)
    {
    }

protected:
    virtual void ExecuteImpl(UWorld* World) override;

private:
    TWeakObjectPtr<UDungeonMesh> Mesh;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_ReuseStaticMeshSimple : public FDungeonSceneProviderCommand_ReuseActor {
public:
    FDungeonSceneProviderCommand_ReuseStaticMeshSimple(const FDungeonSceneProviderContext& InContext, AActor* InActorToReuse, UStaticMesh* InMesh, bool bInCanAffectNavigation)
        : FDungeonSceneProviderCommand_ReuseActor(InContext, InActorToReuse)
        , Mesh(InMesh)
        , bCanAffectNavigation(bInCanAffectNavigation)
    {
    }

protected:
    virtual void ExecuteImpl(UWorld* World) override;

private:
    TWeakObjectPtr<UStaticMesh> Mesh{};
    bool bCanAffectNavigation{};
};

class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_ReuseLight : public FDungeonSceneProviderCommand_ReuseActor {
public:
    FDungeonSceneProviderCommand_ReuseLight(const FDungeonSceneProviderContext& InContext, AActor* InActorToReuse, UPointLightComponent* InLightTemplate)
        : FDungeonSceneProviderCommand_ReuseActor(InContext, InActorToReuse)
        , LightTemplate(InLightTemplate)
    {
    }

protected:
    virtual void ExecuteImpl(UWorld* World) override;

private:
    TWeakObjectPtr<UPointLightComponent> LightTemplate;
};


class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_ReuseParticleSystem : public FDungeonSceneProviderCommand_ReuseActor {
public:
    FDungeonSceneProviderCommand_ReuseParticleSystem(const FDungeonSceneProviderContext& InContext, AActor* InActorToReuse, UParticleSystem* InParticleTemplate)
        : FDungeonSceneProviderCommand_ReuseActor(InContext, InActorToReuse)
        , ParticleTemplate(InParticleTemplate)
    {
    }

protected:
    virtual void ExecuteImpl(UWorld* World) override;

private:
    TWeakObjectPtr<UParticleSystem> ParticleTemplate;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_ReuseActorTemplate : public FDungeonSceneProviderCommand_ReuseActor {
public:
    FDungeonSceneProviderCommand_ReuseActorTemplate(const FDungeonSceneProviderContext& InContext, AActor* InActorToReuse, UClass* InClassTemplate, bool bInCanAffectNavigation)
        : FDungeonSceneProviderCommand_ReuseActor(InContext, InActorToReuse)
        , ClassTemplate(InClassTemplate)
        , bCanAffectNavigation(bInCanAffectNavigation)
    {
    }

protected:
    virtual void ExecuteImpl(UWorld* World) override;

private:
    TWeakObjectPtr<UClass> ClassTemplate;
    bool bCanAffectNavigation;
};

class DUNGEONARCHITECTRUNTIME_API FDungeonSceneProviderCommand_ReuseClonedActor : public FDungeonSceneProviderCommand_ReuseActor {
public:
    FDungeonSceneProviderCommand_ReuseClonedActor(const FDungeonSceneProviderContext& InContext, AActor* InActorToReuse, UDungeonActorTemplate* InActorTemplate)
        : FDungeonSceneProviderCommand_ReuseActor(InContext, InActorToReuse)
        , ActorTemplate(InActorTemplate)
    {
    }
    
protected:
    virtual void ExecuteImpl(UWorld* World) override;

private:
    TWeakObjectPtr<UDungeonActorTemplate> ActorTemplate;
};

