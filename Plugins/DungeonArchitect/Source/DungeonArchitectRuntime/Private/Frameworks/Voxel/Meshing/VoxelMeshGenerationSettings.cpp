//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Voxel/Meshing/VoxelMeshGenerationSettings.h"

#include "Frameworks/ThemeEngine/DungeonThemeAsset.h"
#include "Frameworks/Voxel/Utils/VoxelLib.h"

FDAVoxelMeshGenerationSettings::FDAVoxelMeshGenerationSettings() {
	const FSoftObjectPath PathVoxelShapeTheme(TEXT("/DungeonArchitect/Core/Runtime/Features/Voxel/DA_Voxel_Core_Shape_Cave_Theme.DA_Voxel_Core_Shape_Cave_Theme"));
	VoxelShapeTheme = PathVoxelShapeTheme;
	
	//static const FString DefaultThemePath = TEXT("/DungeonArchitect/Core/Runtime/Features/Voxel/DA_Voxel_Core_Shape_Cave_Theme.DA_Voxel_Core_Shape_Cave_Theme");
	//VoxelShapeTheme = Cast<UDungeonThemeAsset>(PathMasterMaterial.TryLoad());
	//if (UObject* LoadedAsset = StaticLoadObject(UDungeonThemeAsset::StaticClass(), nullptr, *DefaultThemePath)) {
	//	VoxelShapeTheme = Cast<UDungeonThemeAsset>(LoadedAsset);
	//}
}

float FDAVoxelNoiseSettings::GetNoiseScale(bool bIsGPU) const {
	return NoiseScaleNormalized * (bIsGPU ? DA::FVoxelConstants::VoxelScaleMultiplierGPU : DA::FVoxelConstants::VoxelScaleMultiplierCPU);
}

