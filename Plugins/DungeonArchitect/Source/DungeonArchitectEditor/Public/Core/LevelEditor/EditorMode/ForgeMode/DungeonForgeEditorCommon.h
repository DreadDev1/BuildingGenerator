//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"

namespace FDungeonForgeEditorCommon {
	const FString ContextIdentifier = TEXT("DungeonForgeEditorContext");
	
	const FName SpatialDataType = FName(TEXT("Spatial Data"));
	const FName ParamDataType = FName(TEXT("Attribute Set"));
	const FName SettingsDataType = FName(TEXT("Settings Data"));
	const FName OtherDataType = FName(TEXT("Other Data"));

	const FName ConcreteDataType = FName(TEXT("Concrete Data"));
	const FName PointDataType = FName(TEXT("Point Data"));
	const FName PolyLineDataType = FName(TEXT("Poly Line Data"));
	const FName SurfaceDataType = FName(TEXT("Surface Data"));
	const FName LandscapeDataType = FName(TEXT("Landscape Data"));
	const FName BaseTextureDataType = FName(TEXT("Base Texture Data"));
	const FName TextureDataType = FName(TEXT("Texture Data"));
	const FName RenderTargetDataType = FName(TEXT("Render Target Data"));
	const FName VolumeDataType = FName(TEXT("Volume Data"));
	const FName PrimitiveDataType = FName(TEXT("Primitive Data"));
	const FName DynamicMeshDataType = FName(TEXT("Dynamic Mesh Data"));
	const FName MarkerDataType = FName(TEXT("Marker Data"));
	const FName DungeonLayoutType = FName(TEXT("Dungeon Layout Data"));
	const FName WorldLayoutType = FName(TEXT("World Layout Data"));

	const FName FloatType = FName(TEXT("Float Data"));
	const FName Float2Type = FName(TEXT("Float2 Data"));
	const FName Float3Type = FName(TEXT("Float3 Data"));
	const FName Float4Type = FName(TEXT("Float4 Data"));
	
	extern TAutoConsoleVariable<bool> CVarShowAdvancedAttributesFields;

};

