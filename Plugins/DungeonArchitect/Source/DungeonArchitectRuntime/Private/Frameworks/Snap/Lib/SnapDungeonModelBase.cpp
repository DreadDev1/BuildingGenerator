//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Snap/Lib/SnapDungeonModelBase.h"

#include "Core/Layout/DungeonLayoutData.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionInfo.h"

void USnapDungeonModelBase::GenerateLayoutDataImpl(const UDungeonConfig* InConfig, FDungeonLayoutData& OutLayout) const  {
    // Calculate the floor settings
    OutLayout.FloorSettings = CreateFloorSettings(InConfig);

    auto TransformPointsOfInterest = [](const TArray<FDungeonPointOfInterest>& PointsOfInterest, const FTransform& InTransform) {
        TArray<FDungeonPointOfInterest> Result = PointsOfInterest;
        for (FDungeonPointOfInterest& Item : Result) {
            Item.Transform = Item.Transform * InTransform;
        }
        return Result;
    };

    for (const FSnapModuleInstanceSerializedData& ModuleInstance : ModuleInstances) {
        const FDAShapeList ModuleShapes = ModuleInstance.ModuleBoundShapes
                .TransformBy(ModuleInstance.WorldTransform);
        
        const FDungeonCanvasRoomShapeTextureList CanvasRoomShapeTextures = ModuleInstance.CanvasRoomShapeTextures
                .TransformBy(ModuleInstance.WorldTransform);

        FDungeonLayoutDataChunkInfo& ChunkShapeList = OutLayout.ChunkShapes.AddDefaulted_GetRef();
        ChunkShapeList.Circles = ModuleShapes.Circles;
        ChunkShapeList.ConvexPolys = ModuleShapes.ConvexPolys;
        ChunkShapeList.CanvasShapeTextures = CanvasRoomShapeTextures.TextureShapes;
        ChunkShapeList.PointsOfInterest = TransformPointsOfInterest(ModuleInstance.PointsOfInterest, ModuleInstance.WorldTransform);

        {
            FBox Bounds = ModuleInstance.ModuleBounds.TransformBy(ModuleInstance.WorldTransform);

            FVector Center3D = Bounds.GetCenter();
            FDungeonPointOfInterest PointOfInterest;
            PointOfInterest.Transform = FTransform(Center3D);
            PointOfInterest.Id = ModuleInstance.Category;
            PointOfInterest.Caption = ModuleInstance.Category;

            ChunkShapeList.PointsOfInterest.Add(PointOfInterest);
        }
    }

    // Calculate the bounds
    {
        FDungeonLayoutDataChunkInfo AllShapes;
        for (const FDungeonLayoutDataChunkInfo& ChunkShapeList : OutLayout.ChunkShapes) {
            AllShapes.Append(ChunkShapeList);
        }
        OutLayout.Bounds = FDungeonLayoutUtils::CalculateBounds(AllShapes);
   }

    
    // Add the doors
    for (const FSnapConnectionInstance& Connection : Connections) {
        FDungeonLayoutDataDoorItem& Door = OutLayout.Doors.AddDefaulted_GetRef();
        Door.WorldTransform = Connection.WorldTransform;
        Door.Width = (Connection.ConnectionInfo.IsValid() ? Connection.ConnectionInfo->ConnectionWidth : 400) * 0.9f;
        Door.DoorOcclusionThickness = Door.Width; 
        if (Connection.ConnectionInfo.IsValid()) {
            Door.Width = Connection.ConnectionInfo->ConnectionWidth;
        }
    }
}

void USnapDungeonModelBase::GetFloorIndexRange(float InFloorHeight, float InGlobalHeightOffset, int32& OutIndexMin, int32& OutIndexMax) const {
    FBox DungeonBounds(ForceInit);
    for (const FSnapModuleInstanceSerializedData& ModuleInstance : ModuleInstances) {
        DungeonBounds += ModuleInstance.ModuleBounds.TransformBy(ModuleInstance.WorldTransform);
    }
    float MinDungeonHeight = DungeonBounds.Min.Z - InGlobalHeightOffset;
    float MaxDungeonHeight = DungeonBounds.Max.Z - InGlobalHeightOffset;
    float DungeonZ{};
    if (const AActor* DungeonActor = Cast<AActor>(GetOuter())) {
        DungeonZ = DungeonActor->GetActorLocation().Z;
    }

    // TODO: Max ends up being on more floor higher, creating extra floor textures. fix me
    OutIndexMin = FMath::FloorToInt((MinDungeonHeight - DungeonZ + InFloorHeight * 0.5f) / InFloorHeight);
    OutIndexMax = FMath::FloorToInt((MaxDungeonHeight - DungeonZ + InFloorHeight * 0.5f) / InFloorHeight);
}

