//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"

class UEdGraphNode_DungeonThemeActorBase;
class UDungeonThemeAsset;
struct FDungeonThemeVisualNodeData;
struct FDungeonThemeGraphBuildError;

class IDungeonThemeCompiler {
public:
	virtual ~IDungeonThemeCompiler() = default;
	virtual bool Compile(UDungeonThemeAsset* InThemeAsset, TArray<FDungeonThemeGraphBuildError>& OutErrors) = 0;

public:
	static TSharedPtr<IDungeonThemeCompiler> CreateLatestCompiler();
};

