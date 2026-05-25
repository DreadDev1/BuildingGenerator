//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Layout/DungeonLayoutData.h"
#include "Core/Utils/DungeonShapes.h"
#include "Frameworks/Snap/Lib/Connection/SnapConnectionConstants.h"
#include "Frameworks/Snap/Lib/SnapLibrary.h"

#include "Engine/DataAsset.h"
#include "SnapMapModuleDatabase.generated.h"

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FSnapMapModuleDatabaseConnectionInfo {
    GENERATED_USTRUCT_BODY()

    UPROPERTY(VisibleAnywhere, Category = Module)
    FGuid ConnectionId;

    UPROPERTY(VisibleAnywhere, Category = Module)
    FTransform Transform;

    UPROPERTY(VisibleAnywhere, Category = Module)
    TObjectPtr<class USnapConnectionInfo> ConnectionInfo = nullptr;

    UPROPERTY(VisibleAnywhere, Category = Module)
    ESnapConnectionConstraint ConnectionConstraint = ESnapConnectionConstraint::Magnet;
};

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FSnapMapModuleDatabaseItem {
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, Category = Module)
    TSoftObjectPtr<UWorld> Level;

    UPROPERTY(EditAnywhere, Category = Module)
    FName Category = "Room";
    
    /** 
       Can the rooms / modules be rotated while stitching them together. E.g. disable this for isometric / platformer games 
       where the rooms are designed to be viewed at a certain angle 
    */
    UPROPERTY(EditAnywhere, Category = Module)
    bool bAllowRotation = true;
    
    /**
     * Alternate theme level file that should have the same structure as the master level file.
     * Use this to make different themed dungeons using the same generated layout.
     * Great for minimaps,  or creating an alternate world (e.g. player time travels and switches between the modern and ancient versions of the dungeon)
     */
    UPROPERTY(EditAnywhere, Category = Module)
    TMap<FName, TSoftObjectPtr<UWorld>> ThemedLevels;

    UPROPERTY(VisibleAnywhere, Category = Module)
    FBox ModuleBounds = FBox(ForceInitToZero);

    UPROPERTY(VisibleAnywhere, Category = Module)
    FDAShapeList ModuleBoundShapes;

    UPROPERTY(VisibleAnywhere, Category = Module)
    FDungeonCanvasRoomShapeTextureList CanvasRoomShapeTextures;

    UPROPERTY(VisibleAnywhere, Category = Module)
    TArray<FDungeonPointOfInterest> PointsOfInterest;
    
    UPROPERTY(VisibleAnywhere, Category = Module)
    TArray<FSnapMapModuleDatabaseConnectionInfo> Connections;
};

uint32 GetTypeHash(const FSnapMapModuleDatabaseItem& A);


UCLASS(Blueprintable)
class DUNGEONARCHITECTRUNTIME_API USnapMapModuleDatabase : public UDataAsset {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category = Module)
    TArray<FSnapMapModuleDatabaseItem> Modules;

    /** Set the floor height of your assets, if you to support floor rendering in the dungeon canvas */ 
    UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Module)
    float HintFloorHeight = 1000;
    
    /** The amount of floor to capture for the canvas minimap. This should not account for the ceiling height, as it might overlap with the floor above */ 
    UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Module)
    float HintFloorCaptureHeight = 900; 
    
    /** Set the floor height of your assets, if you to support floor rendering in the dungeon canvas */ 
    UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Module)
    float HintGroundThickness = 100;

};

