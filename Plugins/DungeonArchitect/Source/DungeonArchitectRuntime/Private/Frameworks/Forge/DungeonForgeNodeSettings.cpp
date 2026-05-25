//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/DungeonForgeNodeSettings.h"

#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphPin.h"

void UDungeonForgeNodeSettingsInterface::SetEnabled(bool bInEnabled) {
	if (bEnabled != bInEnabled) {
		bEnabled = bInEnabled;
		// TODO: Notify
	}
}

UDungeonForgeGraphNode* UDungeonForgeNodeSettings::CreateNode() const {
	return NewObject<UDungeonForgeGraphNode>();
}

TArray<FDungeonForgePinProperties> UDungeonForgeNodeSettings::AllInputPinProperties() const {
	TArray<FDungeonForgePinProperties> InputPins = InputPinProperties();
	//FillOverridableParamsPins(InputPins);
	return InputPins;
}

TArray<FDungeonForgePinProperties> UDungeonForgeNodeSettings::AllOutputPinProperties() const {
	return OutputPinProperties();
}

TArray<FDungeonForgePinProperties> UDungeonForgeNodeSettings::DefaultInputPinProperties() const {
	return InputPinProperties();
}

TArray<FDungeonForgePinProperties> UDungeonForgeNodeSettings::DefaultOutputPinProperties() const {
	return OutputPinProperties();
}

TArray<FDungeonForgePinProperties> UDungeonForgeNodeSettings::InputPinProperties() const {
	TArray<FDungeonForgePinProperties> PinProperties;
	FDungeonForgePinProperties& InputPinProperty = PinProperties.Emplace_GetRef(DungeonForgePinConstants::DefaultInputLabel, EDungeonForgeDataType::Any);
	InputPinProperty.SetRequiredPin();

	return PinProperties;
}

TArray<FDungeonForgePinProperties> UDungeonForgeNodeSettings::OutputPinProperties() const {
	TArray<FDungeonForgePinProperties> PinProperties;
	// This is not true for everything, use a virtual call?
	PinProperties.Emplace(DungeonForgePinConstants::DefaultOutputLabel, EDungeonForgeDataType::Spatial);

	return PinProperties;
}


EDungeonForgeDataType UDungeonForgeNodeSettings::GetCurrentPinTypes(const UDungeonForgeGraphPin* InPin) const {
	check(InPin);

	/*
	if (HasDynamicPins() && InPin->IsOutputPin())
	{
		const UDungeonForgeGraphNode* Node = Cast<const UDungeonForgeGraphNode>(GetOuter());
		if (Node && Node->GetInputPin(DungeonForgePinConstants::DefaultInputLabel) != nullptr)
		{
			const EDungeonForgeDataType InputTypeUnion = GetTypeUnionOfIncidentEdges(PCGPinConstants::DefaultInputLabel);
			return InputTypeUnion != EDungeonForgeDataType::None ? InputTypeUnion : EDungeonForgeDataType::Any;
		}
	}
	*/

	return InPin->Properties.AllowedTypes;
}

#if WITH_EDITOR
UObject* UDungeonForgeNodeSettings::GetJumpTargetForDoubleClick() const {
	return const_cast<UObject*>(Cast<UObject>(this));
}

void UDungeonForgeNodeSettings::PostPaste() {
	
}
#endif // WITH_EDITOR
