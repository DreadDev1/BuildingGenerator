//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Voxel/Assets/DAVoxelStaticMeshBaker.h"

#include "Frameworks/Voxel/Assets/DAChunkMeshCollection.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Core/Utils/DungeonMeshGeometry.h"
#include "PhysicsEngine/BodySetup.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"

namespace {

static FORCEINLINE int32 ChooseProjectionAxisFromNormal(const FVector3f& FaceNormal) {
    const FVector3f N = FaceNormal.GetSafeNormal();
    const FVector3f AbsN(FMath::Abs(N.X), FMath::Abs(N.Y), FMath::Abs(N.Z));
    if (AbsN.Z >= AbsN.X && AbsN.Z >= AbsN.Y) return 0; // XY
    if (AbsN.X >= AbsN.Y && AbsN.X >= AbsN.Z) return 1; // YZ
    return 2; // XZ
}

static FORCEINLINE FVector2f ProjectToAxisUV(const FVector3f& WorldPos, const FVector3f& FaceNormal, int32 Axis, float UVScale) {
    FVector2f UV;
    switch (Axis) {
        case 0: UV = FVector2f(WorldPos.X, WorldPos.Y); if (FaceNormal.Z < 0) UV.X = -UV.X; break; // XY
        case 1: UV = FVector2f(WorldPos.Y, WorldPos.Z); if (FaceNormal.X < 0) UV.X = -UV.X; break; // YZ
        default: UV = FVector2f(WorldPos.X, WorldPos.Z); if (FaceNormal.Y < 0) UV.X = -UV.X; break; // XZ
    }
    const float Scale = (UVScale != 0.0f) ? UVScale : 1.0f;
    return UV / Scale;
}

static void BuildMeshDescriptionFromGeometry(const DA::FDungeonMeshGeometry& Geometry, const FVector& ChunkWorldPosition, float UVScale, FMeshDescription& OutDesc) {
    FStaticMeshAttributes Attrs(OutDesc);
    Attrs.Register();

    TPolygonGroupAttributesRef<FName> MaterialSlotNames = Attrs.GetPolygonGroupMaterialSlotNames();
    const FPolygonGroupID PolyGroupID = OutDesc.CreatePolygonGroup();
    MaterialSlotNames[PolyGroupID] = FName(TEXT("VoxelSection0"));

    TVertexAttributesRef<FVector3f> Positions = Attrs.GetVertexPositions();
    TVertexInstanceAttributesRef<FVector3f> Normals = Attrs.GetVertexInstanceNormals();
    TVertexInstanceAttributesRef<FVector3f> Tangents = Attrs.GetVertexInstanceTangents();
    TVertexInstanceAttributesRef<float> BinormalSigns = Attrs.GetVertexInstanceBinormalSigns();
    TVertexInstanceAttributesRef<FVector4f> Colors = Attrs.GetVertexInstanceColors();
    TVertexInstanceAttributesRef<FVector2f> UVs = Attrs.GetVertexInstanceUVs();

    const int32 NumVerts = Geometry.Vertices.Num();
    TArray<FVertexID> VertexIDs; VertexIDs.Reserve(NumVerts);
    for (int32 i = 0; i < NumVerts; i++) {
        const FVertexID VID = OutDesc.CreateVertex();
        VertexIDs.Add(VID);
        Positions[VID] = (FVector3f)Geometry.Vertices[i].Position;
    }

    if (UVs.GetNumChannels() < 1) { UVs.SetNumChannels(1); }

    const int32 NumTris = Geometry.Triangles.Num();
    TArray<FVertexInstanceID> TriInstanceIDs; TriInstanceIDs.SetNum(3);
    for (int32 t = 0; t < NumTris; t++) {
        const UE::Geometry::FIndex3i Tri = Geometry.Triangles[t];

        const FVector3f P0 = (FVector3f)(ChunkWorldPosition + Geometry.Vertices[Tri.A].Position);
        const FVector3f P1 = (FVector3f)(ChunkWorldPosition + Geometry.Vertices[Tri.B].Position);
        const FVector3f P2 = (FVector3f)(ChunkWorldPosition + Geometry.Vertices[Tri.C].Position);
        const FVector3f FaceNormal = FVector3f((FVector3d)((P1 - P0) ^ (P2 - P0))).GetSafeNormal();
        const int32 Axis = ChooseProjectionAxisFromNormal(FaceNormal);

        const int32 VIdxs[3] = { Tri.A, Tri.B, Tri.C };
        for (int k = 0; k < 3; k++) {
            const int32 VIdx = VIdxs[k];
            const FVertexID VID = VertexIDs[VIdx];
            const FVertexInstanceID VIID = OutDesc.CreateVertexInstance(VID);

            const UE::Geometry::FVertexInfo& V = Geometry.Vertices[VIdx];
            Normals[VIID] = (FVector3f)V.Normal;
            Tangents[VIID] = FVector3f(0, 0, 0);
            BinormalSigns[VIID] = 1.0f;
            Colors[VIID] = (FVector4f)V.Color;
            const FVector3f WorldPos = (FVector3f)(ChunkWorldPosition + V.Position);
            UVs.Set(VIID, 0, ProjectToAxisUV(WorldPos, FaceNormal, Axis, UVScale));

            TriInstanceIDs[k] = VIID;
        }
        OutDesc.CreatePolygon(PolyGroupID, TriInstanceIDs);
    }
}

} // anonymous namespace

#endif // WITH_EDITOR

UStaticMesh* DA::Voxel::BakeStaticMeshForChunk(UDAChunkMeshCollection* Collection, const FIntVector& ChunkCoord, const DA::FDungeonMeshGeometry& Geometry, UMaterialInterface* Material, const FVector& ChunkWorldPosition, float UVScale) {
#if WITH_EDITOR
    if (!Collection) return nullptr;

    FMeshDescription MeshDesc;
    BuildMeshDescriptionFromGeometry(Geometry, ChunkWorldPosition, UVScale, MeshDesc);

    const FName SubName(*FString::Printf(TEXT("Chunk_%d_%d_%d"), ChunkCoord.X, ChunkCoord.Y, ChunkCoord.Z));

    UStaticMesh* StaticMesh = FindObject<UStaticMesh>(Collection, *SubName.ToString());
    if (!StaticMesh) {
        StaticMesh = NewObject<UStaticMesh>(Collection, SubName, RF_Public | RF_Transactional);
    }

    StaticMesh->SetNumSourceModels(1);
    FMeshNaniteSettings NaniteSettings = StaticMesh->GetNaniteSettings();
    NaniteSettings.bEnabled = Collection->BakeSettings.bEnableNanite;
    StaticMesh->SetNaniteSettings(NaniteSettings);

    if (FMeshDescription* ExistingDesc = StaticMesh->GetMeshDescription(0)) {
        *ExistingDesc = MeshDesc;
    } else {
        FMeshDescription* NewDesc = StaticMesh->CreateMeshDescription(0);
        *NewDesc = MeshDesc;
    }
    StaticMesh->CommitMeshDescription(0);

    // Ensure a matching material slot exists for the single polygon group
    {
        const FName SlotName(TEXT("VoxelSection0"));
        TArray<FStaticMaterial>& Slots = StaticMesh->GetStaticMaterials();
        if (Slots.Num() == 0) {
            Slots.Add(FStaticMaterial(Material ? Material : UMaterial::GetDefaultMaterial(MD_Surface), SlotName, SlotName));
        } else {
            // Update slot 0 material and name
            Slots[0].MaterialInterface = Material ? Material : UMaterial::GetDefaultMaterial(MD_Surface);
            Slots[0].MaterialSlotName = SlotName;
            Slots[0].ImportedMaterialSlotName = SlotName;
        }
    }

    FStaticMeshSourceModel& SrcModel = StaticMesh->GetSourceModel(0);
    SrcModel.BuildSettings.bRecomputeNormals    = false;
    SrcModel.BuildSettings.bRecomputeTangents   = true;
    SrcModel.BuildSettings.bUseFullPrecisionUVs = true;
    SrcModel.BuildSettings.bGenerateLightmapUVs = Collection->BakeSettings.bGenerateLightmapUVs;
    SrcModel.BuildSettings.SrcLightmapIndex     = 0;
    SrcModel.BuildSettings.DstLightmapIndex     = 1;
    StaticMesh->SetLightMapResolution(Collection->BakeSettings.LightmapResolution);

    if (!StaticMesh->GetBodySetup()) {
        StaticMesh->CreateBodySetup();
    }
    if (UBodySetup* BS = StaticMesh->GetBodySetup()) {
        BS->CollisionTraceFlag = Collection->BakeSettings.bUseComplexAsSimpleCollision
            ? ECollisionTraceFlag::CTF_UseComplexAsSimple
            : ECollisionTraceFlag::CTF_UseDefault;
    }

    if (Material) {
        StaticMesh->SetMaterial(0, Material);
    }

    StaticMesh->Build(false);
    StaticMesh->PostEditChange();

    FAssetRegistryModule::AssetCreated(StaticMesh);
    Collection->StaticMeshes.FindOrAdd(ChunkCoord) = StaticMesh;
    Collection->MarkPackageDirty();

    return StaticMesh;
#else
    return nullptr;
#endif // WITH_EDITOR
}

