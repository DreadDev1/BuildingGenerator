//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "Stats/StatsMisc.h"

DECLARE_STATS_GROUP(TEXT("CellFlow"), STATGROUP_CellFlow, STATCAT_Advanced);

DECLARE_CYCLE_STAT_EXTERN(TEXT("Build - MASTER"), STAT_CFBuild, STATGROUP_CellFlow, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Build - Fit Tiles"), STAT_CFBuild_FitTiles, STATGROUP_CellFlow, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Build - Gen Cell Graph"), STAT_CFBuild_BuildCellGraph, STATGROUP_CellFlow, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Build - Collapse Edges"), STAT_CFBuild_CollapseEdges, STATGROUP_CellFlow, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Build - Assign Preview Loc"), STAT_CFBuild_AssignPreviewLoc, STATGROUP_CellFlow, );

DECLARE_CYCLE_STAT_EXTERN(TEXT("Collapse - PrepNodeGroups"), STAT_CFColl_PrepNodeGroups, STATGROUP_CellFlow, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Collapse - SortNodeGroups"), STAT_CFColl_SortNodeGroups, STATGROUP_CellFlow, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Collapse - PrepConnGroups"), STAT_CFColl_PrepConnGroups, STATGROUP_CellFlow, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Collapse - FindArea"), STAT_CFColl_FindArea, STATGROUP_CellFlow, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Collapse - Merge"), STAT_CFColl_Merge, STATGROUP_CellFlow, );

