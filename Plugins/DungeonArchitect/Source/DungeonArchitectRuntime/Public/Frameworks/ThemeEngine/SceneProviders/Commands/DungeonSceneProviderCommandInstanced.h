//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Actors/DungeonMesh.h"
#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommand.h"

class UDungeonMesh;
class ADungeonInstancedMeshActor;
struct FDAInstanceMeshContext  {
    TWeakObjectPtr<ADungeonInstancedMeshActor> InstancedActor;
};

class FDungeonSceneProviderCommand_InstanceMeshBase : public FDungeonSceneProviderCommand {
public:
    FDungeonSceneProviderCommand_InstanceMeshBase(const TSharedPtr<FDAInstanceMeshContext>& InISMContext, const FDungeonSceneProviderContext& SceneContext);

protected:
    bool IsValid() const;

protected:
    TSharedPtr<FDAInstanceMeshContext> ISMContext;
};


class FDungeonSceneProviderCommand_InstanceMesh_FindInstancedActor : public FDungeonSceneProviderCommand_InstanceMeshBase {
public:
    FDungeonSceneProviderCommand_InstanceMesh_FindInstancedActor(const TSharedPtr<FDAInstanceMeshContext>& InContext, const FDungeonSceneProviderContext& SceneContext);
    virtual void ExecuteImpl(UWorld* World) override;
    virtual void UpdateExecutionPriority(const FVector& BuildPosition) override;
};

class FDungeonSceneProviderCommand_InstanceMesh_CreateInstancedActor : public FDungeonSceneProviderCommand_InstanceMeshBase {
public:
    FDungeonSceneProviderCommand_InstanceMesh_CreateInstancedActor(const TSharedPtr<FDAInstanceMeshContext>& InContext, const FDungeonSceneProviderContext& SceneContext);
    virtual void ExecuteImpl(UWorld* World) override;
    virtual void UpdateExecutionPriority(const FVector& BuildPosition) override;
};


class FDungeonSceneProviderCommand_InstanceMesh_OnStart : public FDungeonSceneProviderCommand_InstanceMeshBase {
public:
    FDungeonSceneProviderCommand_InstanceMesh_OnStart(TSharedPtr<FDAInstanceMeshContext> InISMContext, const FDungeonSceneProviderContext& SceneContext);
    virtual void ExecuteImpl(UWorld* World) override;
    virtual void UpdateExecutionPriority(const FVector& BuildPosition) override;
};

class FDungeonSceneProviderCommand_InstanceMesh_OnStop : public FDungeonSceneProviderCommand_InstanceMeshBase {
public:
    FDungeonSceneProviderCommand_InstanceMesh_OnStop(TSharedPtr<FDAInstanceMeshContext> InISMContext, const FDungeonSceneProviderContext& SceneContext);
    virtual void ExecuteImpl(UWorld* World) override;
    virtual void UpdateExecutionPriority(const FVector& BuildPosition) override;
};

class FDungeonSceneProviderCommand_InstanceMesh_AddMeshInstance
    : public FDungeonSceneProviderCommand_InstanceMeshBase
    , public FGCObject
{
public:
    FDungeonSceneProviderCommand_InstanceMesh_AddMeshInstance(TSharedPtr<FDAInstanceMeshContext> InISMContext, const FDungeonSceneProviderContext& SceneContext,
                                                      UDungeonMesh* InMesh, const FTransform& InTransform);

    FDungeonSceneProviderCommand_InstanceMesh_AddMeshInstance(TSharedPtr<FDAInstanceMeshContext> InISMContext, const FDungeonSceneProviderContext& SceneContext,
                                                      UStaticMesh* InStaticMesh, const FTransform& InTransform, bool bCanAffectNavigation);
    
    virtual void ExecuteImpl(UWorld* World) override;
    virtual void UpdateExecutionPriority(const FVector& BuildPosition) override;
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;
    virtual FString GetReferencerName() const override;

private:
    TObjectPtr<UDungeonMesh> Mesh;
    FTransform Transform;
};

