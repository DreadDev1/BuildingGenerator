//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Graph/DungeonForgeEditorGraphNodeBase.h"

#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorCommands.h"
#include "Core/LevelEditor/EditorMode/ForgeMode/DungeonForgeEditorCommon.h"
#include "Frameworks/Forge/DungeonForgeEditorProjectSettings.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "Frameworks/Forge/Graph/DungeonForgeEditorGraph.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraph.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphEdge.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

#include "GraphEditorActions.h"
#include "Misc/TransactionObjectEvent.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"

#define LOCTEXT_NAMESPACE "DungeonForgeEditorGraphNodeBase"


namespace DungeonForgeEditorGraphSwitches
{
	TAutoConsoleVariable<bool> CVarCheckConnectionCycles{
		TEXT("DungeonForge.Editor.CheckConnectionCycles"),
		true,
		TEXT("Prevents user from creating cycles in graph")
	};
}


void UDungeonForgeEditorGraphNodeBase::Construct(UDungeonForgeGraphNode* InForgeNode) {
	check(InForgeNode);
	ForgeNode = InForgeNode;
	InForgeNode->OnNodeChangedDelegate.AddUObject(this, &UDungeonForgeEditorGraphNodeBase::OnNodeChanged);

	NodePosX = InForgeNode->PositionX;
	NodePosY = InForgeNode->PositionY;
	NodeComment = InForgeNode->NodeComment;
	bCommentBubblePinned = InForgeNode->bCommentBubblePinned;
	bCommentBubbleVisible = InForgeNode->bCommentBubbleVisible;
	bCanRenameNode = false;

	/*
	if (const UDungeonForgeNodeSettingsInterface* DungeonForgeSettingsInterface = InForgeNode->GetSettingsInterface()) {
		const ENodeEnabledState NewEnabledState = !DungeonForgeSettingsInterface->bEnabled ? ENodeEnabledState::Disabled : ENodeEnabledState::Enabled;
		SetEnabledState(NewEnabledState);
	}

	// Update to current graph/inspection state.
	const UDungeonForgeEditorGraph* Graph = Cast<UDungeonForgeEditorGraph>(GetOuter());
	const FDungeonForgeEditor* Editor = Graph ? Graph->GetEditor().Pin().Get() : nullptr;
	UpdateStructuralVisualization(Editor ? Editor->GetDungeonForgeComponentBeingInspected() : nullptr, Editor ? Editor->GetStackBeingInspected() : nullptr, true);
	*/
}

void UDungeonForgeEditorGraphNodeBase::BeginDestroy() {
	if (ForgeNode) {
		ForgeNode->OnNodeChangedDelegate.RemoveAll(this);
	}
	Super::BeginDestroy();	
}

void UDungeonForgeEditorGraphNodeBase::PostTransacted(const FTransactionObjectEvent& TransactionEvent) {
	Super::PostTransacted(TransactionEvent);
	TArray<FName> PropertiesChanged = TransactionEvent.GetChangedProperties();

	if (PropertiesChanged.Contains(TEXT("bCommentBubblePinned"))) {
		UpdateCommentBubblePinned();
	}

	if (PropertiesChanged.Contains(TEXT("NodePosX")) || PropertiesChanged.Contains(TEXT("NodePosY"))) {
		UpdatePosition();
	}
}

void UDungeonForgeEditorGraphNodeBase::GetNodeContextMenuActions(UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const {
	if (!Context->Node) {
		return;
	}
	
	{
		FToolMenuSection& Section = Menu->AddSection("EdGraphSchemaNodeActions", LOCTEXT("NodeActionsHeader", "Node Actions"));
		Section.AddMenuEntry(FDungeonForgeEditorCommands::Get().ToggleEnabled, LOCTEXT("ToggleEnabledLabel", "Enable"));
		Section.AddMenuEntry(FDungeonForgeEditorCommands::Get().ToggleDebug, LOCTEXT("ToggleDebugLabel", "Debug"));
	}
	
	{
		FToolMenuSection& Section = Menu->AddSection("EdGraphSchemaOrganization", LOCTEXT("OrganizationHeader", "Organization"));
		Section.AddSubMenu("Alignment", LOCTEXT("AlignmentHeader", "Alignment"), FText(), FNewToolMenuDelegate::CreateLambda([](UToolMenu* AlignmentMenu)
		{
			{
				FToolMenuSection& SubSection = AlignmentMenu->AddSection("EdGraphSchemaAlignment", LOCTEXT("AlignHeader", "Align"));
				SubSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesTop);
				SubSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesMiddle);
				SubSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesBottom);
				SubSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesLeft);
				SubSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesCenter);
				SubSection.AddMenuEntry(FGraphEditorCommands::Get().AlignNodesRight);
				SubSection.AddMenuEntry(FGraphEditorCommands::Get().StraightenConnections);
			}

			{
				FToolMenuSection& SubSection = AlignmentMenu->AddSection("EdGraphSchemaDistribution", LOCTEXT("DistributionHeader", "Distribution"));
				SubSection.AddMenuEntry(FGraphEditorCommands::Get().DistributeNodesHorizontally);
				SubSection.AddMenuEntry(FGraphEditorCommands::Get().DistributeNodesVertically);
			}
		}));
	}
	
	{
		FToolMenuSection& Section = Menu->AddSection("EdGraphSchemaCommentGroup", LOCTEXT("CommentGroupHeader", "Comment Group"));
		Section.AddMenuEntry(FGraphEditorCommands::Get().CreateComment,
			LOCTEXT("MultiCommentDesc", "Create Comment from Selection"),
			LOCTEXT("CommentToolTip", "Create a resizable comment box around selection."));
	}
}

void UDungeonForgeEditorGraphNodeBase::PostCopy() {
	if (ForgeNode)
	{
		UDungeonForgeEditorGraph* ForgeEditorGraph = CastChecked<UDungeonForgeEditorGraph>(GetGraph());
		UDungeonForgeGraph* ForgeGraph = ForgeEditorGraph->GetForgeGraph();
		check(ForgeGraph);
		ForgeNode->Rename(nullptr, ForgeGraph, REN_DontCreateRedirectors | REN_DoNotDirty);
	}
}

void UDungeonForgeEditorGraphNodeBase::RebuildAfterPaste() {
	// TODO: Implement me
}

void UDungeonForgeEditorGraphNodeBase::PostPaste() {
	// TODO: Implement me
}

FLinearColor UDungeonForgeEditorGraphNodeBase::GetNodeTitleColor() const {
	if (ForgeNode)
	{
		const UDungeonForgeNodeSettingsInterface* DungeonForgeSettingsInterface = ForgeNode->GetSettingsInterface();
		const UDungeonForgeNodeSettings* DungeonForgeSettings = DungeonForgeSettingsInterface ? DungeonForgeSettingsInterface->GetSettings() : nullptr;

		if (ForgeNode->NodeTitleColor != FLinearColor::White)
		{
			return ForgeNode->NodeTitleColor;
		}
		else if (DungeonForgeSettings)
		{
			FLinearColor SettingsColor = ForgeNode->GetSettings()->GetNodeTitleColor();
			if (SettingsColor == FLinearColor::White)
			{
				SettingsColor = GetDefault<UDungeonForgeEditorProjectSettings>()->GetColor(ForgeNode->GetSettings());
			}

			if (SettingsColor != FLinearColor::White)
			{
				return SettingsColor;
			}
		}
	}

	return GetDefault<UDungeonForgeEditorProjectSettings>()->DefaultNodeColor;
}

bool UDungeonForgeEditorGraphNodeBase::IsCompatible(const UDungeonForgeGraphPin* InputPin, const UDungeonForgeGraphPin* OutputPin, FText& OutReason) const {
	if (DungeonForgeEditorGraphSwitches::CVarCheckConnectionCycles.GetValueOnAnyThread() && InputPin && OutputPin && InputPin->Node == ForgeNode)
	{
		// Upstream Visitor
		TSet<const UDungeonForgeGraphNode*> VisitedNodes;
		auto Visitor = [&VisitedNodes, ThisForgeNode = ForgeNode](const UDungeonForgeGraphNode* InNode, auto VisitorLambda) -> bool
		{
			if (InNode)
			{
				if (InNode == ThisForgeNode)
				{
					return false;
				}
				else if (VisitedNodes.Contains(InNode))
				{
					return true;
				}

				VisitedNodes.Add(InNode);

				for (const TObjectPtr<UDungeonForgeGraphPin>& InputPin : InNode->GetInputPins())
				{
					if (InputPin)
					{
						for (const TObjectPtr<UDungeonForgeGraphEdge>& Edge : InputPin->Edges)
						{
							if (Edge)
							{
								if (const UDungeonForgeGraphPin* OtherPin = Edge->GetOtherPin(InputPin.Get()))
								{
									if (!VisitorLambda(OtherPin->Node, VisitorLambda))
									{
										return false;
									}
								}
							}
						}
					}
				}
			}

			return true;
		};

		// OutputPin is trying to connect to this nodes InputPin so visit the OutputPin upstream and try to find
		// a existing connection to this UDungeonForgeEditorGraphNodeNamedRerouteDeclaration's ForgeNode. If we do deny connection which would create cycle.
		if (!Visitor(OutputPin->Node, Visitor))
		{
			OutReason = LOCTEXT("ConnectionFailedCyclic", "Connection would create cycle");
			return false;
		}
	}

	return true;
}

EDungeonForgeGraphChangeType UDungeonForgeEditorGraphNodeBase::UpdateErrorsAndWarnings() {
	return EDungeonForgeGraphChangeType::None;
}

void UDungeonForgeEditorGraphNodeBase::EnableDeferredReconstruct() {
	ensure(DeferredReconstructCounter >= 0);
	++DeferredReconstructCounter;
}

void UDungeonForgeEditorGraphNodeBase::DisableDeferredReconstruct() {
	ensure(DeferredReconstructCounter > 0);
	--DeferredReconstructCounter;

	if (DeferredReconstructCounter == 0 && bDeferredReconstruct)
	{
		ReconstructNode();
		bDeferredReconstruct = false;
	}
}

void UDungeonForgeEditorGraphNodeBase::OnNodeChanged(UDungeonForgeGraphNode* InNode, EDungeonForgeGraphChangeType ChangeType) {
	if (InNode == ForgeNode) {
		if (!!(ChangeType & EDungeonForgeGraphChangeType::Settings))
		{
			if (const UDungeonForgeNodeSettingsInterface* ForgeSettingsInterface = InNode->GetSettingsInterface())
			{
				const ENodeEnabledState NewEnabledState = ForgeSettingsInterface->bEnabled ? ENodeEnabledState::Enabled : ENodeEnabledState::Disabled;
				if (NewEnabledState != GetDesiredEnabledState())
				{
					SetEnabledState(NewEnabledState);
				}
			}
		}

		ChangeType |= UpdateErrorsAndWarnings();
		if (!!(ChangeType & (EDungeonForgeGraphChangeType::Structural | EDungeonForgeGraphChangeType::Node | EDungeonForgeGraphChangeType::Edge | EDungeonForgeGraphChangeType::Cosmetic)))
		{
			ReconstructNodeOnChange();
		}
	}
}

void UDungeonForgeEditorGraphNodeBase::UpdateCommentBubblePinned() {
	if (ForgeNode) {
		ForgeNode->Modify();
		ForgeNode->bCommentBubblePinned = bCommentBubblePinned;
	}
}

void UDungeonForgeEditorGraphNodeBase::UpdatePosition() {
	if (ForgeNode) {
		ForgeNode->Modify();
		ForgeNode->PositionX = NodePosX;
		ForgeNode->PositionY = NodePosY;
	}
}

void UDungeonForgeEditorGraphNodeBase::CreatePins(const TArray<UDungeonForgeGraphPin*>& InInputPins, const TArray<UDungeonForgeGraphPin*>& InOutputPins)
{
	bool bHasAdvancedPin = false;

	for (const UDungeonForgeGraphPin* InputPin : InInputPins)
	{
		if (!ShouldCreatePin(InputPin))
		{
			continue;
		}

		UEdGraphPin* Pin = CreatePin(EEdGraphPinDirection::EGPD_Input, GetPinType(InputPin), InputPin->Properties.Label);
		Pin->PinFriendlyName = GetPinFriendlyName(InputPin);
		Pin->bAdvancedView = InputPin->Properties.IsAdvancedPin();
		bHasAdvancedPin |= Pin->bAdvancedView;
	}

	for (const UDungeonForgeGraphPin* OutputPin : InOutputPins)
	{
		if (!ShouldCreatePin(OutputPin))
		{
			continue;
		}

		UEdGraphPin* Pin = CreatePin(EEdGraphPinDirection::EGPD_Output, GetPinType(OutputPin), OutputPin->Properties.Label);
		Pin->PinFriendlyName = GetPinFriendlyName(OutputPin);
		Pin->bAdvancedView = OutputPin->Properties.IsAdvancedPin();
		bHasAdvancedPin |= Pin->bAdvancedView;
	}

	if (bHasAdvancedPin && AdvancedPinDisplay == ENodeAdvancedPins::NoPins)
	{
		AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
	}
	else if (!bHasAdvancedPin)
	{
		AdvancedPinDisplay = ENodeAdvancedPins::NoPins;
	}
}

bool UDungeonForgeEditorGraphNodeBase::ShouldCreatePin(const UDungeonForgeGraphPin* InPin) const {
	return InPin && !InPin->Properties.bInvisiblePin;
}

FText UDungeonForgeEditorGraphNodeBase::GetPinFriendlyName(const UDungeonForgeGraphPin* InPin) const {
	check(InPin);

	// For overridable params, use the display name of properties (for localized version or overridden display name in metadata).
	if (InPin->Properties.IsAdvancedPin() && InPin->Properties.AllowedTypes == EDungeonForgeDataType::Param)
	{
		const UDungeonForgeNodeSettings* Settings = InPin->Node ? InPin->Node->GetSettings() : nullptr;
		if (Settings)
		{
			/*
			const FDungeonForgeSettingsOverridableParam* Param = Settings->OverridableParams().FindByPredicate([Label = InPin->Properties.Label](const FDungeonForgeSettingsOverridableParam& ParamToCheck)
			{
				return ParamToCheck.Label == Label;
			});

			if (Param)
			{
				return Param->GetDisplayPropertyPathText();
			}
			*/
		}
	}

	return FText::FromString(FName::NameToDisplayString(InPin->Properties.Label.ToString(), /*bIsBool=*/false));
}

FEdGraphPinType UDungeonForgeEditorGraphNodeBase::GetPinType(const UDungeonForgeGraphPin* InPin) {
	FEdGraphPinType EdPinType;
	EdPinType.ResetToDefaults();
	EdPinType.PinCategory = NAME_None;
	EdPinType.PinSubCategory = NAME_None;
	EdPinType.ContainerType = EPinContainerType::None;

	check(InPin);
	const EDungeonForgeDataType PinType = InPin->GetCurrentTypes();

	auto CheckType = [PinType](EDungeonForgeDataType AllowedType)
	{
		return !!(PinType & AllowedType) && !(PinType & ~AllowedType);
	};

	if (CheckType(EDungeonForgeDataType::Concrete))
	{
		EdPinType.PinCategory = FDungeonForgeEditorCommon::ConcreteDataType;

		// Assign subcategory if we have precise information
		if (CheckType(EDungeonForgeDataType::Point))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::PointDataType;
		}
		else if (CheckType(EDungeonForgeDataType::PolyLine))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::PolyLineDataType;
		}
		else if (CheckType(EDungeonForgeDataType::Landscape))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::LandscapeDataType;
		}
		else if (CheckType(EDungeonForgeDataType::BaseTexture))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::BaseTextureDataType;
		}
		else if (CheckType(EDungeonForgeDataType::Texture))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::TextureDataType;
		}
		else if (CheckType(EDungeonForgeDataType::RenderTarget))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::RenderTargetDataType;
		}
		else if (CheckType(EDungeonForgeDataType::Surface))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::SurfaceDataType;
		}
		else if (CheckType(EDungeonForgeDataType::Volume))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::VolumeDataType;
		}
		else if (CheckType(EDungeonForgeDataType::DynamicMesh))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::DynamicMeshDataType;
		}
		else if (CheckType(EDungeonForgeDataType::Marker))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::MarkerDataType;
		}
		else if (CheckType(EDungeonForgeDataType::Primitive))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::PrimitiveDataType;
		}
		else if (CheckType(EDungeonForgeDataType::WorldLayout))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::WorldLayoutType;
		}
		else if (CheckType(EDungeonForgeDataType::DungeonLayout))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::DungeonLayoutType;
		}
		else if (CheckType(EDungeonForgeDataType::Float))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::FloatType;
		}
		else if (CheckType(EDungeonForgeDataType::Float2))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::Float2Type;
		}
		else if (CheckType(EDungeonForgeDataType::Float3))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::Float3Type;
		}
		else if (CheckType(EDungeonForgeDataType::Float4))
		{
			EdPinType.PinSubCategory = FDungeonForgeEditorCommon::Float4Type;
		}
	}
	else if (CheckType(EDungeonForgeDataType::Spatial))
	{
		EdPinType.PinCategory = FDungeonForgeEditorCommon::SpatialDataType;
	}
	else if (CheckType(EDungeonForgeDataType::Param))
	{
		EdPinType.PinCategory = FDungeonForgeEditorCommon::ParamDataType;
	}
	else if (CheckType(EDungeonForgeDataType::Settings))
	{
		EdPinType.PinCategory = FDungeonForgeEditorCommon::SettingsDataType;
	}
	else if (CheckType(EDungeonForgeDataType::Other))
	{
		EdPinType.PinCategory = FDungeonForgeEditorCommon::OtherDataType;
	}

	return EdPinType;
}


#undef LOCTEXT_NAMESPACE

