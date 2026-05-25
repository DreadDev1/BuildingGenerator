//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Graph/DungeonForgeGraphEdge.h"

#include "Frameworks/Forge/Graph/DungeonForgeGraphPin.h"

UDungeonForgeGraphEdge::UDungeonForgeGraphEdge(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetFlags(RF_Transactional);
}

void UDungeonForgeGraphEdge::PostLoad()
{
	Super::PostLoad();
	SetFlags(RF_Transactional);
}

bool UDungeonForgeGraphEdge::IsValid() const
{
	return InputPin.Get() && OutputPin.Get();
}

UDungeonForgeGraphPin* UDungeonForgeGraphEdge::GetOtherPin(const UDungeonForgeGraphPin* Pin)
{
	check(Pin == InputPin || Pin == OutputPin);
	return Pin == InputPin ? OutputPin : InputPin;
}

const UDungeonForgeGraphPin* UDungeonForgeGraphEdge::GetOtherPin(const UDungeonForgeGraphPin* Pin) const
{
	check(Pin == InputPin || Pin == OutputPin);
	return Pin == InputPin ? OutputPin : InputPin;
}

