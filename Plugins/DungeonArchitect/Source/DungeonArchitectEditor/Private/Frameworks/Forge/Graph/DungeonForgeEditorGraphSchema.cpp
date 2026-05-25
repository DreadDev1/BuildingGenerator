//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphSchema.h"

#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorCommon.h"
#include "Frameworks/Forge/DungeonForgeEditorProjectSettings.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraph.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphNode.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphNodeBase.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphSchemaActions.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraph.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

#include "Framework/Commands/GenericCommands.h"
#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "DungeonForgeEditorGraphSchema"


//////////////////////////////// Schema ////////////////////////////////
UDungeonForgeEditorGraphSchema::UDungeonForgeEditorGraphSchema() {
	
}

FConnectionDrawingPolicy* UDungeonForgeEditorGraphSchema::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const {
	return new FDungeonForgeEditorGraphConnDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements);
}

FLinearColor UDungeonForgeEditorGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const {
	return GetDefault<UDungeonForgeEditorProjectSettings>()->GetPinColor(PinType);
}

const FPinConnectionResponse UDungeonForgeEditorGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const {
	check(A && B);
	const UEdGraphNode* NodeA = A->GetOwningNode();
	const UEdGraphNode* NodeB = B->GetOwningNode();

	if (NodeA == NodeB)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionSameNode", "Both pins are on same node"));
	}

	if (A->Direction == B->Direction)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionSameDirection", "Both pins are the same direction"));
	}

	const UDungeonForgeEditorGraphNodeBase* EditorNodeA = CastChecked<const UDungeonForgeEditorGraphNodeBase>(NodeA);
	const UDungeonForgeEditorGraphNodeBase* EditorNodeB = CastChecked<const UDungeonForgeEditorGraphNodeBase>(NodeB);
	const UDungeonForgeEditorGraphNodeBase* EditorNodeWithInput = nullptr;

	// Check type compatibility & whether we can connect more pins
	const UDungeonForgeGraphPin* InputPin = nullptr;
	const UDungeonForgeGraphPin* OutputPin = nullptr;

	if (A->Direction == EGPD_Output)
	{
		OutputPin = EditorNodeA->GetForgeNode()->GetOutputPin(A->PinName);
		InputPin = EditorNodeB->GetForgeNode()->GetInputPin(B->PinName);
		EditorNodeWithInput = EditorNodeB;
	}
	else
	{
		OutputPin = EditorNodeB->GetForgeNode()->GetOutputPin(B->PinName);
		InputPin = EditorNodeA->GetForgeNode()->GetInputPin(A->PinName);
		EditorNodeWithInput = EditorNodeA;
	}

	if (!InputPin || !OutputPin)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionFailed", "Unable to verify pins"));
	}

	if (!InputPin->IsCompatible(OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("ConnectionTypesIncompatible", "Pins are incompatible"));
	}

	const EDungeonForgeTypeConversion RequiredTypeConversion = InputPin->GetRequiredTypeConversion(OutputPin);
	if (RequiredTypeConversion == EDungeonForgeTypeConversion::CollapseToPoint)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, LOCTEXT("ConnectionConversionToPoint", "Convert to Point"));
	}
	else if (RequiredTypeConversion == EDungeonForgeTypeConversion::Filter)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, LOCTEXT("ConnectionUsingFilter", "Filter data to match type"));
	}
	else if (RequiredTypeConversion == EDungeonForgeTypeConversion::MakeConcrete)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, LOCTEXT("ConnectionUsingMakeConcrete", "Make data concrete"));
	}

	if (!InputPin->AllowsMultipleConnections() && InputPin->EdgeCount() > 0)
	{
		return FPinConnectionResponse((A->Direction == EGPD_Output) ? CONNECT_RESPONSE_BREAK_OTHERS_B : CONNECT_RESPONSE_BREAK_OTHERS_A, LOCTEXT("ConnectionBreakExisting", "Break existing connection?"));
	}

	FText Reason;
	if (!EditorNodeWithInput->IsCompatible(InputPin, OutputPin, Reason))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, Reason);
	}

	return FPinConnectionResponse();
}

bool UDungeonForgeEditorGraphSchema::TryCreateConnection(UEdGraphPin* InA, UEdGraphPin* InB) const {
	return TryCreateConnectionInternal(InA, InB, /*bAddConversionNodeIfNeeded=*/true);
}

void UDungeonForgeEditorGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const {
	const FScopedTransaction Transaction(*FDungeonForgeEditorCommon::ContextIdentifier, LOCTEXT("DungeonForgeEditorBreakPinLinks", "Break Pin Links"), nullptr);

	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);

	UEdGraphNode* GraphNode = TargetPin.GetOwningNode();

	UDungeonForgeEditorGraphNodeBase* DungeonForgeGraphNode = CastChecked<UDungeonForgeEditorGraphNodeBase>(GraphNode);

	UDungeonForgeGraphNode* DungeonForgeNode = DungeonForgeGraphNode->GetForgeNode();
	check(DungeonForgeNode);

	UDungeonForgeGraph* DungeonForgeGraph = DungeonForgeNode->GetGraph();
	check(DungeonForgeGraph);

	if (TargetPin.Direction == EEdGraphPinDirection::EGPD_Input)
	{
		DungeonForgeGraph->RemoveInboundEdges(DungeonForgeNode, TargetPin.PinName);
	}
	else if (TargetPin.Direction == EEdGraphPinDirection::EGPD_Output)
	{
		DungeonForgeGraph->RemoveOutboundEdges(DungeonForgeNode, TargetPin.PinName);
	}
}

void UDungeonForgeEditorGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const {
	const FScopedTransaction Transaction(*FDungeonForgeEditorCommon::ContextIdentifier, LOCTEXT("DungeonForgeEditorBreakSinglePinLink", "Break Single Pin Link"), nullptr);

	Super::BreakSinglePinLink(SourcePin, TargetPin);

	UEdGraphNode* SourceGraphNode = SourcePin->GetOwningNode();
	UEdGraphNode* TargetGraphNode = TargetPin->GetOwningNode();

	UDungeonForgeEditorGraphNodeBase* SourceDungeonForgeGraphNode = CastChecked<UDungeonForgeEditorGraphNodeBase>(SourceGraphNode);
	UDungeonForgeEditorGraphNodeBase* TargetDungeonForgeGraphNode = CastChecked<UDungeonForgeEditorGraphNodeBase>(TargetGraphNode);

	UDungeonForgeGraphNode* SourceDungeonForgeNode = SourceDungeonForgeGraphNode->GetForgeNode();
	UDungeonForgeGraphNode* TargetDungeonForgeNode = TargetDungeonForgeGraphNode->GetForgeNode();
	check(SourceDungeonForgeNode && TargetDungeonForgeNode);

	UDungeonForgeGraph* DungeonForgeGraph = SourceDungeonForgeNode->GetGraph();
	DungeonForgeGraph->RemoveEdge(SourceDungeonForgeNode, SourcePin->PinName, TargetDungeonForgeNode, TargetPin->PinName);
}

void UDungeonForgeEditorGraphSchema::OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2f& GraphPosition) const {
	Super::OnPinConnectionDoubleCicked(PinA, PinB, GraphPosition);
}

bool UDungeonForgeEditorGraphSchema::TryCreateConnectionInternal(UEdGraphPin* InA, UEdGraphPin* InB, bool bAddConversionNodeIfNeeded) const {
	check(InA && InB);
	if (InA->Direction == InB->Direction)
	{
		// Don't connect same polarity
		return false;
	}

	UEdGraphPin* A = (InA->Direction == EGPD_Output) ? InA : InB;
	UEdGraphPin* B = (InA->Direction == EGPD_Input) ? InA : InB;
	check(A->Direction == EGPD_Output && B->Direction == EGPD_Input);

	UEdGraphNode* NodeA = A->GetOwningNodeUnchecked();
	UEdGraphNode* NodeB = B->GetOwningNodeUnchecked();
	if (!ensure(NodeA && NodeB))
	{
		// TODO: We've had crashes where one of these nodes was nullptr, we need to figure out why this can happen.
		return false;
	}

	UDungeonForgeEditorGraphNodeBase* DungeonForgeEdGraphNodeA = CastChecked<UDungeonForgeEditorGraphNodeBase>(NodeA);
	UDungeonForgeEditorGraphNodeBase* DungeonForgeEdGraphNodeB = CastChecked<UDungeonForgeEditorGraphNodeBase>(NodeB);

	UDungeonForgeGraphNode* DungeonForgeNodeA = DungeonForgeEdGraphNodeA->GetForgeNode();
	UDungeonForgeGraphNode* DungeonForgeNodeB = DungeonForgeEdGraphNodeB->GetForgeNode();
	check(DungeonForgeNodeA && DungeonForgeNodeB);

	const UDungeonForgeGraphPin* DungeonForgePinA = DungeonForgeNodeA->GetOutputPin(A->PinName);
	const UDungeonForgeGraphPin* DungeonForgePinB = DungeonForgeNodeB->GetInputPin(B->PinName);
	check(DungeonForgePinA && DungeonForgePinB);
	if (!DungeonForgePinA->IsCompatible(DungeonForgePinB))
	{
		return false;
	}

	UDungeonForgeGraph* DungeonForgeGraph = DungeonForgeNodeA->GetGraph();
	check(DungeonForgeGraph);

	// UDungeonForgeEditorGraphSchema::TryCreateConnectionInternal is called directly by FDragConnection::DroppedOnPin
	//DungeonForgeGraph->PrimeGraphCompilationCache();

	// Creates a connection via an intermediate conversion node.
	auto ConnectViaIntermediate = [this, DungeonForgeGraph, NodeA, NodeB, A, B](UDungeonForgeGraphNode* IntermediateNode)
	{
		UEdGraph* Graph = NodeA->GetGraph();
		check(Graph);
		Graph->Modify();

		FGraphNodeCreator<UDungeonForgeEditorGraphNode> NodeCreator(*Graph);
		UDungeonForgeEditorGraphNode* ConversionNode = NodeCreator.CreateUserInvokedNode(/*bSelectNewNode=*/false);
		ConversionNode->Construct(IntermediateNode);

		// Put the conversion node between A & B but make it stay within a radius of B to keep things tidy.
		{
			// Initially place at mid point
			ConversionNode->NodePosX = (NodeA->NodePosX + NodeB->NodePosX) / 2;
			ConversionNode->NodePosY = (NodeA->NodePosY + NodeB->NodePosY) / 2;

			// A hand tweaked distance that keeps it reasonably close.
			constexpr float MaxDistFromB = 200;
			const FVector2D OffsetFromB(ConversionNode->NodePosX - NodeB->NodePosX, ConversionNode->NodePosY - NodeB->NodePosY);
			const float Dist = OffsetFromB.Length();
			if (Dist > MaxDistFromB)
			{
				const float Scale = MaxDistFromB / Dist;
				ConversionNode->NodePosX = NodeB->NodePosX + Scale * OffsetFromB.X;
				ConversionNode->NodePosY = NodeB->NodePosY + Scale * OffsetFromB.Y;
			}
		}

		NodeCreator.Finalize();

		IntermediateNode->PositionX = ConversionNode->NodePosX;
		IntermediateNode->PositionY = ConversionNode->NodePosY;

		bool bModifiedA = false, bModifiedB = false;

		UEdGraphPin*const* ConversionInputPin = ConversionNode->GetAllPins().FindByPredicate([](const UEdGraphPin* InPin)
		{
			return InPin->Direction == EGPD_Input && InPin->GetFName() == DungeonForgePinConstants::DefaultInputLabel;
		});

		if (ensure(ConversionInputPin && *ConversionInputPin))
		{
			// Last argument: don't allow recursively adding conversion nodes.
			bModifiedA = TryCreateConnectionInternal(A, *ConversionInputPin, /*bAddConversionNodeIfNeeded=*/false);
		}

		// Call GetAllPins() a second time. It's important that we wire up the pins one at a time. Wiring a pin can change dynamic pin types
		// which can refresh the node, so we must re-query the pins after each connection is made.
		UEdGraphPin*const* ConversionOutputPin = ConversionNode->GetAllPins().FindByPredicate([](const UEdGraphPin* InPin)
		{
			return InPin->Direction == EGPD_Output && (InPin->GetFName() == DungeonForgePinConstants::DefaultOutputLabel || InPin->GetFName() == DungeonForgePinConstants::DefaultInFilterLabel);
		});

		if (ensure(ConversionOutputPin && *ConversionOutputPin))
		{
			// Last argument: don't allow recursively adding conversion nodes.
			bModifiedB = TryCreateConnectionInternal(*ConversionOutputPin, B, /*bAddConversionNodeIfNeeded=*/false);
		}

		return bModifiedA || bModifiedB;
	};

	const EDungeonForgeTypeConversion Conversion = bAddConversionNodeIfNeeded ? DungeonForgePinA->GetRequiredTypeConversion(DungeonForgePinB) : EDungeonForgeTypeConversion::NoConversionRequired;

	/*
	if (Conversion == EDungeonForgeTypeConversion::Filter)
	{
		UDungeonForgeSettings* NodeSettings = nullptr;
		UDungeonForgeGraphNode* ConversionDungeonForgeNode = DungeonForgeGraph->AddNodeOfType(UDungeonForgeFilterByTypeSettings::StaticClass(), NodeSettings);

		// Setup the output pin based on the conversion target type, before new node is finalized.
		UDungeonForgeFilterByTypeSettings* Settings = CastChecked<UDungeonForgeFilterByTypeSettings>(NodeSettings);
		Settings->TargetType = DungeonForgePinB->Properties.AllowedTypes;
		ConversionDungeonForgeNode->UpdateAfterSettingsChangeDuringCreation();

		return ConnectViaIntermediate(ConversionDungeonForgeNode);
	}
	else if (Conversion == EDungeonForgeTypeConversion::MakeConcrete)
	{
		UDungeonForgeSettings* NodeSettings = nullptr;
		UDungeonForgeGraphNode* ConversionDungeonForgeNode = DungeonForgeGraph->AddNodeOfType(UDungeonForgeMakeConcreteSettings::StaticClass(), NodeSettings);

		return ConnectViaIntermediate(ConversionDungeonForgeNode);
	}
	else if (Conversion == EDungeonForgeTypeConversion::SplineToSurface)
	{
		UDungeonForgeSettings* NodeSettings = nullptr;
		UDungeonForgeGraphNode* ConversionDungeonForgeNode = DungeonForgeGraph->AddNodeOfType(UDungeonForgeCreateSurfaceFromSplineSettings::StaticClass(), NodeSettings);

		UDungeonForgeCreateSurfaceFromSplineSettings* Settings = CastChecked<UDungeonForgeCreateSurfaceFromSplineSettings>(NodeSettings);
		Settings->bShouldDrawNodeCompact = true;

		return ConnectViaIntermediate(ConversionDungeonForgeNode);
	}
	else
	*/
	
	{
		const bool bModified = Super::TryCreateConnection(InA, InB);
		if (bModified)
		{
			DungeonForgeGraph->AddLabeledEdge(DungeonForgeNodeA, A->PinName, DungeonForgeNodeB, B->PinName);
		}

		return bModified;
	}
}

void UDungeonForgeEditorGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const {
	const UEdGraph* Graph = ContextMenuBuilder.CurrentGraph;
	TArray<TSharedPtr<FEdGraphSchemaAction>> Actions;

	GetNativeElementActions(ContextMenuBuilder, ContextMenuBuilder.CurrentGraph);

	for (TSharedPtr<FEdGraphSchemaAction> Action : Actions) {
		ContextMenuBuilder.AddAction(Action);
	}
}

void UDungeonForgeEditorGraphSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const {
	if (Context && Context->Node) {
		FToolMenuSection& Section = Menu->AddSection("PatternGraphNodeActions", LOCTEXT("NodeActionsMenuHeader", "Node Actions"));
		if (!Context->bIsDebugging)
		{
			// Node contextual actions
			Section.AddMenuEntry(FGenericCommands::Get().Delete);
			Section.AddMenuEntry(FGenericCommands::Get().Cut);
			Section.AddMenuEntry(FGenericCommands::Get().Copy);
			Section.AddMenuEntry(FGenericCommands::Get().Duplicate);
			Section.AddMenuEntry(FGraphEditorCommands::Get().ReconstructNodes);
			Section.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
		}
	}
	Super::GetContextMenuActions(Menu, Context);
}

void UDungeonForgeEditorGraphSchema::GetNativeElementActions(FGraphActionMenuBuilder& ActionMenuBuilder, const UEdGraph* CurrentGraph) const {
	TArray<UClass*> SettingsClasses;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;

		if (Class->IsChildOf(UDungeonForgeNodeSettings::StaticClass()) &&
			!Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_Hidden))
		{
			SettingsClasses.Add(Class);
		}
	}

	for (UClass* SettingsClass : SettingsClasses)
	{
		if (const UDungeonForgeNodeSettings* ForgeSettingsCDO = SettingsClass->GetDefaultObject<UDungeonForgeNodeSettings>())
		{
			if (ForgeSettingsCDO->bExposeToLibrary)
			{
				const FText MenuDesc = ForgeSettingsCDO->GetDefaultNodeTitle();
				const FText Category = StaticEnum<EDungeonForgeSettingsType>()->GetDisplayNameTextByValue(static_cast<__underlying_type(EDungeonForgeSettingsType)>(ForgeSettingsCDO->GetType()));
				const FText Description = ForgeSettingsCDO->GetNodeTooltipText();

				TArray<FDungeonForgePreConfiguredSettingsInfo> AllPreconfiguredInfo = ForgeSettingsCDO->GetPreconfiguredInfo();

				if (AllPreconfiguredInfo.IsEmpty() || !ForgeSettingsCDO->OnlyExposePreconfiguredSettings())
				{
					TSharedPtr<FDungeonForgeEditorGraphSchemaAction_NewNativeElement> NewAction(new FDungeonForgeEditorGraphSchemaAction_NewNativeElement(Category, MenuDesc, Description, 0));
					NewAction->SettingsClass = SettingsClass;
					ActionMenuBuilder.AddAction(NewAction);

					// Also add all aliases
					for (const FText& Alias : ForgeSettingsCDO->GetNodeTitleAliases())
					{
						TSharedPtr<FDungeonForgeEditorGraphSchemaAction_NewNativeElement> NewAliasAction(new FDungeonForgeEditorGraphSchemaAction_NewNativeElement(Category, Alias, Description, 0));
						NewAliasAction->SettingsClass = SettingsClass;
						ActionMenuBuilder.AddAction(NewAliasAction);
					}
				}

				// Also add preconfigured settings
				const FText NewCategory = ForgeSettingsCDO->GroupPreconfiguredSettings() ? FText::Format(LOCTEXT("PreconfiguredSettingsCategory", "{0}|{1}"), Category, MenuDesc) : Category;

				for (FDungeonForgePreConfiguredSettingsInfo PreconfiguredInfo : AllPreconfiguredInfo)
				{
					TSharedPtr<FDungeonForgeEditorGraphSchemaAction_NewNativeElement> NewPreconfiguredAction(new FDungeonForgeEditorGraphSchemaAction_NewNativeElement(NewCategory, PreconfiguredInfo.Label, PreconfiguredInfo.Tooltip.IsEmpty() ? Description : PreconfiguredInfo.Tooltip, 0));
					NewPreconfiguredAction->SettingsClass = SettingsClass;
					NewPreconfiguredAction->PreconfiguredInfo = std::move(PreconfiguredInfo);
					ActionMenuBuilder.AddAction(NewPreconfiguredAction);
				}
			}
		}
	}

	/*
	if (const UDungeonForgeEditorGraph* Graph = Cast<UDungeonForgeEditorGraph>(CurrentGraph))
	{
		if (const UDungeonForgeGraph* ForgeGraph = Graph->GetForgeGraph())
		{
			if (const FInstancedPropertyBag* UserParameters = ForgeGraph->GetUserParametersStruct())
			{
				if (const UPropertyBag* BagStruct = UserParameters->GetPropertyBagStruct())
				{
					const FText Category = LOCTEXT("UserParametersCategoryName", "Graph Parameters");

					for (const FPropertyBagPropertyDesc& PropertyDesc : BagStruct->GetPropertyDescs())
					{
						const FText MenuDesc = FText::Format(LOCTEXT("GetterNodeName", "Get {0}"), FText::FromName(PropertyDesc.Name));
						const FText Description = FText::Format(LOCTEXT("NodeTooltip", "Get the value from '{0}' parameter, can be overridden by the graph instance."), FText::FromName(PropertyDesc.Name));

						TSharedPtr<FDungeonForgeEditorGraphSchemaAction_NewGetParameterElement> NewAction(new FDungeonForgeEditorGraphSchemaAction_NewGetParameterElement(Category, MenuDesc, Description, 0));
						NewAction->SettingsClass = UDungeonForgeUserParameterGetSettings::StaticClass();
						NewAction->PropertyName = PropertyDesc.Name;
						NewAction->PropertyGuid = PropertyDesc.ID;
						ActionMenuBuilder.AddAction(NewAction);
					}
				}
			}
		}
	}
	*/
}

//////////////////////////////// Connection Drawing Policy ////////////////////////////////
FDungeonForgeEditorGraphConnDrawingPolicy::FDungeonForgeEditorGraphConnDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements)
{
	ArrowImage = nullptr;
	ArrowRadius = FVector2D::ZeroVector;
}

void FDungeonForgeEditorGraphConnDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FConnectionParams& Params) {
	FConnectionDrawingPolicy::DetermineWiringStyle(OutputPin, InputPin, Params);

	Params.WireThickness = GetDefault<UDungeonForgeEditorProjectSettings>()->DefaultWireThickness;

	// Emphasize wire thickness on hovered pins
	if (HoveredPins.Contains(InputPin) && HoveredPins.Contains(OutputPin))
	{
		Params.WireThickness *= GetDefault<UDungeonForgeEditorProjectSettings>()->HoverEdgeEmphasis;
	}

	// Base the color of the wire on the color of the output pin
	if (OutputPin)
	{
		Params.WireColor = GetDefault<UDungeonForgeEditorProjectSettings>()->GetPinColor(OutputPin->PinType);
	}
}

#undef LOCTEXT_NAMESPACE

