//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/DungeonBuilder.h"

typedef TSharedPtr<class FDungeonMinimapBuilder> FDungeonMinimapBuilderPtr;

class FDungeonMinimapBuilderFactory {
public:
	static FDungeonMinimapBuilderPtr Create(TSubclassOf<UDungeonBuilder> InBuilderClass);
};

