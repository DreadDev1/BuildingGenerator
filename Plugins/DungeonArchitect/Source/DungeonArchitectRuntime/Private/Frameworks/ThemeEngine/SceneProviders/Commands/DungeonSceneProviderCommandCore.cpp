//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommandCore.h"

#include "Core/Actors/DungeonActorTemplate.h"
#include "Core/Actors/DungeonMesh.h"
#include "Core/Utils/EditorService/IDungeonEditorService.h"
#include "Frameworks/ThemeEngine/Common/DungeonThemeEngineUtils.h"
#include "Frameworks/ThemeEngine/SceneProviders/DungeonSceneProvider.h"

#include "AI/NavigationSystemBase.h"
#include "Components/BaseDynamicMeshSceneProxy.h"
#include "DynamicMesh/MeshNormals.h"
#include "DynamicMeshActor.h"
#include "Engine/Engine.h"
#include "Engine/Light.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GeometryScript/MeshUVFunctions.h"
#include "GeometryScript/MeshUVFunctions.h"

void FDungeonSceneProviderCommand_CreateMesh::ExecuteImpl(UWorld* World) {
    FActorSpawnParameters SpawnParams;
    SpawnParams.OverrideLevel = Context.LevelOverride.Get();
    SpawnParams.bDeferConstruction = true;
    AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), SpawnParams);
    if (!MeshActor) {
        return;
    }
    
    UStaticMeshComponent* MeshComponent = MeshActor->GetStaticMeshComponent();
    if (!MeshComponent) {
        return;
    }
    
    EComponentMobility::Type OriginalMobility = MeshComponent->Mobility;
    if (Mesh.IsValid()) {
        if (Mesh->Template) {
            OriginalMobility = Mesh->Template->Mobility;
        }
        MeshActor->SetMobility(EComponentMobility::Movable);
        MeshComponent->SetStaticMesh(Mesh->StaticMesh);
        MeshActor->SetMobility(OriginalMobility);

        for (const FMaterialOverride& MaterialOverride : Mesh->MaterialOverrides) {
            MeshComponent->SetMaterial(MaterialOverride.index, MaterialOverride.Material);
        }

        SetMeshComponentAttributes(MeshComponent, Mesh->Template);
        MeshComponent->SetCanEverAffectNavigation(Mesh->bCanEverAffectNavigation);

        if (Mesh->bUseCustomCollision) {
            MeshComponent->BodyInstance = Mesh->BodyInstance;
        }
        MeshComponent->RecreatePhysicsState();
    }

    MeshActor->FinishSpawning(Context.Transform);

    PostInitializeActor(MeshActor);
}


#define SET_MESH_ATTRIB(Attrib) StaticMeshComponent->Attrib = StaticMeshTemplate->Attrib

void FDungeonSceneProviderCommand_CreateMesh::SetMeshComponentAttributes(UStaticMeshComponent* StaticMeshComponent,
                                                                 UStaticMeshComponent* StaticMeshTemplate) {
    if (StaticMeshTemplate) {
        UEngine::CopyPropertiesForUnrelatedObjects(StaticMeshTemplate, StaticMeshComponent);

        StaticMeshComponent->SetCanEverAffectNavigation(StaticMeshTemplate->CanEverAffectNavigation());
    }
    else {
        StaticMeshComponent->SetMobility(EComponentMobility::Static);
        StaticMeshComponent->SetCanEverAffectNavigation(true);
        StaticMeshComponent->bCastStaticShadow = true;
    }
}

void FDungeonSceneProviderCommand_CreateMeshSimple::ExecuteImpl(UWorld* World) {
    FActorSpawnParameters SpawnParams;
    SpawnParams.OverrideLevel = Context.LevelOverride.Get();
    SpawnParams.bDeferConstruction = true;
    AStaticMeshActor* MeshActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), SpawnParams);
    if (!MeshActor) {
        return;
    }
    UStaticMeshComponent* MeshComponent = MeshActor->GetStaticMeshComponent();

    const EComponentMobility::Type OriginalMobility = MeshComponent->Mobility;
    MeshActor->SetMobility(EComponentMobility::Movable);
    MeshComponent->SetStaticMesh(Mesh.Get());
    MeshComponent->SetCanEverAffectNavigation(bCanAffectNavigation);
    MeshActor->SetMobility(OriginalMobility);
    MeshActor->FinishSpawning(Context.Transform);

    PostInitializeActor(MeshActor);
}

void FDungeonSceneProviderCommand_AddLight::ExecuteImpl(UWorld* World) {
    ALight* Light = nullptr;
    UPointLightComponent* PointLightComponent = nullptr;
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.OverrideLevel = Context.LevelOverride.Get();
    SpawnParams.bDeferConstruction = true;

    if (USpotLightComponent* SpotLightTemplate = Cast<USpotLightComponent>(LightTemplate)) {
        ASpotLight* SpotLight = World->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Context.Transform, SpawnParams);
        USpotLightComponent* SpotLightComponent = SpotLight->SpotLightComponent;
        SetSpotLightAttributes(SpotLightComponent, SpotLightTemplate);

        Light = SpotLight;
        PointLightComponent = SpotLightComponent;
    }
    else {
        APointLight* PointLight = World->SpawnActor<APointLight>(APointLight::StaticClass(), Context.Transform,
                                                                 SpawnParams);

        Light = PointLight;
        PointLightComponent = PointLight->PointLightComponent;
    }

    SetPointLightAttributes(PointLightComponent, LightTemplate.Get());
    Light->FinishSpawning(Context.Transform, true);
    Light->ReregisterAllComponents();

    PostInitializeActor(Light);
}

#define SET_SPOT_ATTRIB(Attrib) SpotLightComponent->Attrib = SpotLightTemplate->Attrib

void FDungeonSceneProviderCommand_AddLight::SetSpotLightAttributes(USpotLightComponent* SpotLightComponent,
                                                           USpotLightComponent* SpotLightTemplate) {
    FGuid OrigLightGuid = SpotLightComponent->LightGuid;
    UEngine::CopyPropertiesForUnrelatedObjects(SpotLightTemplate, SpotLightComponent);
    SpotLightComponent->LightGuid = OrigLightGuid;
}

#define SET_LIGHT_ATTRIB(Attrib) PointLightComponent->Attrib = LightTemplate->Attrib;

void FDungeonSceneProviderCommand_AddLight::SetPointLightAttributes(UPointLightComponent* PointLightComponent,
                                                            UPointLightComponent* LightTemplate) {
    if (PointLightComponent && LightTemplate) {
        FGuid OrigLightGuid = PointLightComponent->LightGuid;
        UEngine::CopyPropertiesForUnrelatedObjects(LightTemplate, PointLightComponent);
        PointLightComponent->LightGuid = OrigLightGuid;
    }
}

void FDungeonSceneProviderCommand_AddParticleSystem::ExecuteImpl(UWorld* World) {
    FActorSpawnParameters SpawnParams;
    SpawnParams.OverrideLevel = Context.LevelOverride.Get();
    AEmitter* ParticleEmitter = World->SpawnActor<AEmitter>(SpawnParams);
    ParticleEmitter->SetActorTransform(Context.Transform);
    ParticleEmitter->GetParticleSystemComponent()->SetTemplate(ParticleTemplate.Get());

    PostInitializeActor(ParticleEmitter);
}

namespace DA::SceneProvider::Private {
    void SetActorTransform(AActor* Actor, const FTransform& transform) {
        if (!Actor) return;
        USceneComponent* SceneComponent = Actor->GetRootComponent();

        if (SceneComponent) {
            EComponentMobility::Type OriginialMobility = SceneComponent->Mobility;
            SceneComponent->SetMobility(EComponentMobility::Movable);
            SceneComponent->SetWorldTransform(transform);
            SceneComponent->UpdateChildTransforms();
            SceneComponent->SetMobility(OriginialMobility);
            SceneComponent->ReregisterComponent();
        }
    }
}

void FDungeonSceneProviderCommand_AddActor::ExecuteImpl(UWorld* World) {
    FActorSpawnParameters SpawnParams;
    SpawnParams.OverrideLevel = Context.LevelOverride.Get();

    if (!ClassTemplate.IsValid()) {
        return;
    }
    
    AActor* Actor = World->SpawnActor<AActor>(ClassTemplate.Get(), Context.Transform, SpawnParams);
    if (!Actor) {
        return;
    }
    Actor->SetActorScale3D(Context.Transform.GetScale3D()); // UE5.2 seems to ignore the scale. Setting it manually
    if (Actor) {
#if WITH_EDITOR
        if (World->WorldType == EWorldType::Editor) {
            Actor->RerunConstructionScripts();
        }
#endif // WITH_EDITOR

        if (USceneComponent* SceneComponent = Actor->GetRootComponent()) {
            SceneComponent->SetCanEverAffectNavigation(bCanAffectNavigation);
        }
        
        TagAsComplexObject(Actor);
        PostInitializeActor(Actor);
    }
}

void FDungeonSceneProviderCommand_AddMeshGeometry::UpdateExecutionPriority(const FVector& BuildPosition) {
    // We want the custom meshes to be spawned last
    ExecutionPriority = MAX_int32;
}

void FDungeonSceneProviderCommand_AddMeshGeometry::ExecuteImpl(UWorld* World) {
    double StartTime = FPlatformTime::Seconds();
    
    FActorSpawnParameters SpawnParams;
    ADynamicMeshActor* DynamicMeshActor = World->SpawnActor<ADynamicMeshActor>();
    DynamicMeshActor->SetActorTransform(Context.Transform);
    DynamicMeshActor->Tags.Append(Settings.ActorTags);
    
    TagAsComplexObject(DynamicMeshActor);
    PostInitializeActor(DynamicMeshActor);
    
    UDynamicMeshComponent* MeshComponent = DynamicMeshActor->GetDynamicMeshComponent();
    MeshComponent->SetMeshDrawPath(EDynamicMeshDrawPath::StaticDraw);
    MeshComponent->bCastShadowAsTwoSided = true;
    
    if (Settings.Geometry.IsValid()) {
        MeshComponent->EditMesh([this](FDynamicMesh3& EditMesh) {
            // Convert to DynamicMesh3
            EditMesh.Clear();
            EditMesh.EnableTriangleGroups();
            EditMesh.EnableAttributes();
            EditMesh.Attributes()->EnablePrimaryColors();
            EditMesh.Attributes()->EnableMaterialID();

            ::FDynamicMeshUVOverlay* UVOverlay = EditMesh.Attributes()->PrimaryUV();
            FDynamicMeshNormalOverlay* NormalOverlay = EditMesh.Attributes()->PrimaryNormals();
            FDynamicMeshColorOverlay* ColorOverlay = EditMesh.Attributes()->PrimaryColors();
		
            // Add vertices
            TArray<int32> VertexIndices;
            for (const UE::Geometry::FVertexInfo& Vertex : Settings.Geometry->Vertices) {
                const int32 VertIdx = EditMesh.AppendVertex(Vertex.Position);
                if (VertIdx >= 0) {
                    VertexIndices.Add(VertIdx);
                    UVOverlay->AppendElement(Vertex.UV);
                    NormalOverlay->AppendElement(Vertex.Normal);
                    ColorOverlay->AppendElement(Vertex.Color);
                }
            }
            
            // Add triangles
            for (const UE::Geometry::FIndex3i& Triangle : Settings.Geometry->Triangles) {
                const UE::Geometry::FIndex3i MappedTri(VertexIndices[Triangle.A], VertexIndices[Triangle.B], VertexIndices[Triangle.C]);
                const int32 TriId = EditMesh.AppendTriangle(MappedTri, 0);
			
                if (TriId >= 0) {
                    UVOverlay->SetTriangle(TriId, MappedTri);
                    NormalOverlay->SetTriangle(TriId, MappedTri);
                    ColorOverlay->SetTriangle(TriId, MappedTri);
                }
            }
        });

        // Apply box projection UVs
        ApplyBoxProjectionUVs(MeshComponent);
        
        if (Settings.bEnableCollision) {
            MeshComponent->bUseAsyncCooking = true;
            MeshComponent->CollisionType = CTF_UseComplexAsSimple;
            MeshComponent->bEnableComplexCollision = true;
            MeshComponent->UpdateCollision(false);
        }
        
        if (Settings.Material.IsValid()) {
            MeshComponent->SetMaterial(0, Settings.Material.Get());
        }

        MeshComponent->SetCanEverAffectNavigation(true);
        
        FNavigationSystem::OnComponentRegistered(*MeshComponent);
    }
}

void FDungeonSceneProviderCommand_CloneActor::ExecuteImpl(UWorld* World) {
    if (!ActorTemplate.IsValid()) return;
    if (!ActorTemplate->ClassTemplate) {
        ActorTemplate->ActorTemplate = nullptr;
        return;
    }

    AActor* TargetActorTemplate = ActorTemplate->ActorTemplate;

    // Check if the template actor class matches the latest version
    if (!TargetActorTemplate || TargetActorTemplate->GetClass() != ActorTemplate->ClassTemplate) {
        // The actor version does not match with the class version (the class was modified after the actor template was created)
        // Create a new template and copy the parameters over
        TargetActorTemplate = NewObject<AActor>(World, ActorTemplate->ClassTemplate, NAME_None,
                                                RF_ArchetypeObject | RF_Transactional | RF_Public);
        if (ActorTemplate->ActorTemplate) {
            UEngine::CopyPropertiesForUnrelatedObjects(ActorTemplate->ActorTemplate, TargetActorTemplate);
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.OverrideLevel = Context.LevelOverride.Get();
    SpawnParams.Template = TargetActorTemplate;
    SpawnParams.bDeferConstruction = true;
    AActor* Actor = World->SpawnActor<AActor>(ActorTemplate->ClassTemplate, SpawnParams);
    if (Actor) {
        Actor->FinishSpawning(Context.Transform);

#if WITH_EDITOR
        if (World->WorldType == EWorldType::Editor) {
            Actor->RerunConstructionScripts();
        }
#endif // WITH_EDITOR

        TagAsComplexObject(Actor);
        PostInitializeActor(Actor);
    }
}

void FDungeonSceneProviderCommand_SetActorTransform::ExecuteImpl(UWorld* World) {
    DA::SceneProvider::Private::SetActorTransform(Actor.Get(), Context.Transform);
}

void FDungeonSceneProviderCommand_CreateGroupActor::ExecuteImpl(UWorld* World) {
    TSharedPtr<IDungeonEditorService> EditorService = IDungeonEditorService::Get();
    if (!EditorService.IsValid()) {
        // Editor service not specified. Cannot create editor specific functionality (e.g. when running as standalone game)
        return;
    }

    TArray<FName> GroupActorNodeTags;
    for (const FName& ActorNodeId : ActorNodeIds) {
        GroupActorNodeTags.Add(FDungeonThemeEngineUtils::CreateNodeTagFromId(ActorNodeId));
    }

    TArray<AActor*> GroupedActors;
    for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt) {
        AActor* Actor = *ActorIt;
        for (const FName& Tag : Actor->Tags) {
            if (GroupActorNodeTags.Contains(Tag)) {
                GroupedActors.Add(Actor);
                break;
            }
        }
    }

    AActor* GroupActor = EditorService->CreateGroupActor(World, GroupedActors, Context.Transform);
    if (GroupActor) {
        PostInitializeActor(GroupActor, false, false);
    }
}

void FDungeonSceneProviderCommand_DestroyActorWithTag::ExecuteImpl(UWorld* World) {
    FName NodeTag = FDungeonThemeEngineUtils::CreateNodeTagFromId(Tag);
    for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt) {
        AActor* Actor = *ActorIt;
        if (Actor->Tags.Contains(NodeTag)) {
            Actor->Destroy();
            break;
        }
    }
}

void FDungeonSceneProviderCommand_ReuseActor::ExecuteImpl(UWorld* World) {
    if (ActorToReuse.IsValid()) {
        
#if WITH_EDITOR
        if (bRerunConstructionScripts && World && World->WorldType == EWorldType::Editor) {
            ActorToReuse->RerunConstructionScripts();
        }
#endif // WITH_EDITOR
        
        if (!ActorToReuse->GetTransform().Equals(Context.Transform)) {
            DA::SceneProvider::Private::SetActorTransform(ActorToReuse.Get(), Context.Transform);
        }

        PostInitializeActor(ActorToReuse.Get());
    }
}

void FDungeonSceneProviderCommand_ReuseStaticMesh::ExecuteImpl(UWorld* World) {
    if (Mesh.IsValid()) {
        if (AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(ActorToReuse)) {
            UStaticMeshComponent* MeshComponent = StaticMeshActor->GetStaticMeshComponent();
            MeshComponent->SetStaticMesh(Mesh->StaticMesh);
            MeshComponent->SetCanEverAffectNavigation(Mesh->bCanEverAffectNavigation);

            // Set the mesh component attributes
            FDungeonSceneProviderCommand_CreateMesh::SetMeshComponentAttributes(MeshComponent, Mesh->Template);
            StaticMeshActor->MarkComponentsRenderStateDirty();
        }
    }

    // Call the base class
    bRerunConstructionScripts = false;
    FDungeonSceneProviderCommand_ReuseActor::ExecuteImpl(World);
}

void FDungeonSceneProviderCommand_ReuseStaticMeshSimple::ExecuteImpl(UWorld* World) {
    if (Mesh.IsValid()) {
        if (const AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(ActorToReuse)) {
            UStaticMeshComponent* MeshComponent = StaticMeshActor->GetStaticMeshComponent();
            MeshComponent->SetStaticMesh(Mesh.Get());
            MeshComponent->SetCanEverAffectNavigation(bCanAffectNavigation);
        }
    }

    // Call the base class
    bRerunConstructionScripts = false;
    FDungeonSceneProviderCommand_ReuseActor::ExecuteImpl(World);
}
 
void FDungeonSceneProviderCommand_ReuseLight::ExecuteImpl(UWorld* World) {
    if (USpotLightComponent* SpotLightTemplate = Cast<USpotLightComponent>(LightTemplate)) {
        if (ASpotLight* SpotLightActor = Cast<ASpotLight>(ActorToReuse)) {
            FDungeonSceneProviderCommand_AddLight::SetSpotLightAttributes(SpotLightActor->SpotLightComponent,
                                                                  SpotLightTemplate);
            FDungeonSceneProviderCommand_AddLight::SetPointLightAttributes(SpotLightActor->SpotLightComponent,
                                                                   SpotLightTemplate);
        }
    }
    else if (APointLight* PointLight = Cast<APointLight>(ActorToReuse)) {
        FDungeonSceneProviderCommand_AddLight::SetPointLightAttributes(PointLight->PointLightComponent, LightTemplate.Get());
    }

    // Call the base class
    FDungeonSceneProviderCommand_ReuseActor::ExecuteImpl(World);
}

void FDungeonSceneProviderCommand_ReuseParticleSystem::ExecuteImpl(UWorld* World) {
    if (AEmitter* EmitterActor = Cast<AEmitter>(ActorToReuse)) {
        EmitterActor->SetTemplate(ParticleTemplate.Get());
    }

    // Call the base class
    FDungeonSceneProviderCommand_ReuseActor::ExecuteImpl(World);
}

void FDungeonSceneProviderCommand_ReuseActorTemplate::ExecuteImpl(UWorld* World) {
    if (ClassTemplate.IsValid() && ActorToReuse.IsValid() && !ActorToReuse->GetClass()->IsChildOf(ClassTemplate.Get())) {
        ActorToReuse->Destroy();
        ActorToReuse = nullptr;

        FDungeonSceneProviderCommand_AddActor AddActorCommand(Context, ClassTemplate.Get(), bCanAffectNavigation);
        AddActorCommand.Execute(World);
    }
    else {
        // The desired template class is correct. call the base class
        FDungeonSceneProviderCommand_ReuseActor::ExecuteImpl(World);
    }
}

void FDungeonSceneProviderCommand_ReuseClonedActor::ExecuteImpl(UWorld* World) {
    if (ActorTemplate.IsValid() && !ActorToReuse->IsA(ActorTemplate->GetClass())) {
        ActorToReuse->Destroy();
        ActorToReuse = nullptr;

        FDungeonSceneProviderCommand_CloneActor CloneActorCommand(Context, ActorTemplate.Get());
        CloneActorCommand.Execute(World);
    }
    else {
        // The desired template class is correct. call the base class
        FDungeonSceneProviderCommand_ReuseActor::ExecuteImpl(World);
    }
}


void FDungeonSceneProviderCommand_DestroyActor::ExecuteImpl(UWorld* World) {
    if (Actor.IsValid() && Actor->IsValidLowLevel()) {
        Actor->Destroy();
        Actor = nullptr;
    }
}

void FDungeonSceneProviderCommand_AddMeshGeometry::ApplyBoxProjectionUVs(UDynamicMeshComponent* MeshComponent) {
    if (!MeshComponent) return;
    
    UDynamicMesh* DynamicMesh = MeshComponent->GetDynamicMesh();
    if (!DynamicMesh) return;
    
    // For voxel chunks, we want to use the chunk's world-space bounds for seamless UVs
    // The Context.Transform contains the chunk's position in world space
    FVector ChunkWorldPos = Context.Transform.GetLocation();
    
    // Calculate the box transform for UV projection
    // Use chunk position as the center but apply UV scale
    FTransform BoxTransform;
    BoxTransform.SetLocation(ChunkWorldPos);
    BoxTransform.SetScale3D(FVector(Settings.UVScale));
    
    // Apply box projection using GeometryScript
    UGeometryScriptLibrary_MeshUVFunctions::SetMeshUVsFromBoxProjection(
        DynamicMesh,
        0, // UV Channel 0
        BoxTransform,
        FGeometryScriptMeshSelection(), // Apply to entire mesh
        2, // MinIslandTriCount
        nullptr // Debug
    );
}

/*
void FSceneProviderCommand_InstanceMesh_FindAndDestroyActor::ExecuteImpl(UWorld* World) {
    // Search for an existing actor that holds the instanced static meshes
    const FName DungeonTag = FDungeonUtils::GetDungeonIdTag(Context.Dungeon.Get());
    TArray<ADungeonInstancedMeshActor*> ActorsToDestroy;
    for (TObjectIterator<ADungeonInstancedMeshActor> ActorItr; ActorItr; ++ActorItr) {
        if (!ActorItr->IsValidLowLevel() || !::IsValid(*ActorItr)) continue;
        if (ADungeonInstancedMeshActor* InstancedActor = *ActorItr) {
            if (InstancedActor->ActorHasTag(DungeonTag)) {
                ActorsToDestroy.Add(InstancedActor);
            }
        }
    }
    for (ADungeonInstancedMeshActor* Actor : ActorsToDestroy) {
        Actor->Destroy();
    }
}
*/

