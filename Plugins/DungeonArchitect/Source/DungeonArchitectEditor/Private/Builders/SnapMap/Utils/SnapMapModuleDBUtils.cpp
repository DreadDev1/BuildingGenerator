//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/SnapMap/Utils/SnapMapModuleDBUtils.h"

#include "Frameworks/Snap/Lib/Module/SnapModuleBoundShape.h"
#include "Frameworks/Snap/SnapMap/SnapMapModuleDatabase.h"
#include "Frameworks/Snap/SnapModuleDBBuilder.h"

void FSnapMapModuleDBUtils::BuildModuleDatabaseCache(USnapMapModuleDatabase* InDatabase) {
    if (!InDatabase) return;

    struct FSnapMapModulePolicy {
        FBox CalculateBounds(ULevel* Level) const {
            return SnapModuleDatabaseBuilder::FDefaultModulePolicy::CalculateBounds(Level);
        }
        void Initialize(FSnapMapModuleDatabaseItem& ModuleItem, const ULevel* Level, const UObject* InModuleDB) {}
        void PostProcess(FSnapMapModuleDatabaseItem& ModuleItem, const ULevel* Level) const {
            FSnapModuleBoundShapeUtils::InitializeModuleBoundShapes(Level->Actors, ModuleItem.ModuleBounds,
                ModuleItem.ModuleBoundShapes, ModuleItem.CanvasRoomShapeTextures, ModuleItem.PointsOfInterest);
        }
    };

    typedef TSnapModuleDatabaseBuilder<
        FSnapMapModuleDatabaseItem,
        FSnapMapModuleDatabaseConnectionInfo,
        FSnapMapModulePolicy,
        SnapModuleDatabaseBuilder::TDefaultConnectionPolicy<FSnapMapModuleDatabaseConnectionInfo>
    > FSnapMapDatabaseBuilder;
    
    FSnapMapDatabaseBuilder::Build(InDatabase->Modules, InDatabase);
    
    InDatabase->Modify();
}

