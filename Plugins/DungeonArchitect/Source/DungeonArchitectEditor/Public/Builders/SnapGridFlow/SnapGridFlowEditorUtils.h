//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class USnapGridFlowModuleDatabase;
class ASnapGridFlowModuleBoundsActor;
struct FSnapGridFlowModuleDatabaseConnectionInfo;

class DUNGEONARCHITECTEDITOR_API FSnapGridFlowEditorUtils {
public:
    static void BuildModuleDatabaseCache(USnapGridFlowModuleDatabase* InDatabase);
};

