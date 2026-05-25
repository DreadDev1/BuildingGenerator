//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/PCG/DungeonPCGAttribute.h"


const FDAPCGAttribute<int32> FDungeonPCGAttributes::ID(TEXT("ID"), 0, false, true, true);
const FDAPCGAttribute<FName> FDungeonPCGAttributes::MarkerName(TEXT("MarkerName"), TEXT(""), false, true, true);
const FDAPCGAttribute<float> FDungeonPCGAttributes::SDF(TEXT("SDF"), 0.0f, true, true, true);
const FDAPCGAttribute<float> FDungeonPCGAttributes::GroundHeight(TEXT("GroundHeight"), 0.0f, true, true, true);

