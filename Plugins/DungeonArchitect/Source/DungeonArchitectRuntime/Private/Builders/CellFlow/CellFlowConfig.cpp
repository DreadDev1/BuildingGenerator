//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Builders/CellFlow/CellFlowConfig.h"

#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

FCellFlowMeshProfile::FCellFlowMeshProfile() {
	if (!IsRunningCommandlet()) {
		static const TCHAR* DefaultMaterialName = TEXT("/DungeonArchitect/Core/Runtime/Features/FlowGraph/CellFlow/M_CellFlow_Voronoi_Inst");
		// Structure to hold one-time initialization
		struct FConstructorStatics {
			ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> Material;

			FConstructorStatics() :
				Material(DefaultMaterialName)
			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		Material = ConstructorStatics.Material.Get();
	}
}

FDungeonConfigGenericSettings UCellFlowConfig::GatherGenericSettings() {
	FDungeonConfigGenericSettings Settings;
	Settings.bIsGridBased = true;
	Settings.GridSize = GridSize;
	return Settings;
}

