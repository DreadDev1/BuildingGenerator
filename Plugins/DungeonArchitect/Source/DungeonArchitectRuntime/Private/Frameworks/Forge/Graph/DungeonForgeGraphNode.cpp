//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

#include "Core/Utils/DungeonLog.h"
#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraph.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphEdge.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphPin.h"

#include "UObject/Package.h"

UDungeonForgeGraphNode::UDungeonForgeGraphNode() {
	if (!HasAnyFlags(RF_ClassDefaultObject)) {
		NodeGuid = FGuid::NewGuid();
	}
}

EDungeonForgeGraphChangeType UDungeonForgeGraphNode::PropagateDynamicPinTypes(TSet<UDungeonForgeGraphNode*>& TouchedNodes, const UDungeonForgeGraphNode* FromNode) {
	EDungeonForgeGraphChangeType ChangeType = EDungeonForgeGraphChangeType::None;
	return ChangeType;
	
}

void UDungeonForgeGraphNode::PostDuplicate(bool bDuplicateForPIE) {
	UObject::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE) {
		NodeGuid = FGuid::NewGuid();
	}
}

void UDungeonForgeGraphNode::PostLoad() {
	UObject::PostLoad();
	if (!NodeGuid.IsValid()) {
		NodeGuid = FGuid::NewGuid();
	}
}

#if WITH_EDITOR
void UDungeonForgeGraphNode::TransferEditorProperties(UDungeonForgeGraphNode* OtherNode) const {
	OtherNode->PositionX = PositionX;
	OtherNode->PositionY = PositionY;
	OtherNode->bCommentBubblePinned = bCommentBubblePinned;
	OtherNode->bCommentBubbleVisible = bCommentBubbleVisible;
	OtherNode->NodeComment = NodeComment;
}

void UDungeonForgeGraphNode::GetNodePosition(int32& OutPositionX, int32& OutPositionY) const {
	OutPositionX = PositionX;
	OutPositionY = PositionY;
}

void UDungeonForgeGraphNode::SetNodePosition(int32 InPositionX, int32 InPositionY) {
	PositionX = InPositionX;
	PositionY = InPositionY;
}
#endif // WITH_EDITOR

void UDungeonForgeGraphNode::SetSettingsInterface(UDungeonForgeNodeSettingsInterface* InSettingsInterface, bool bUpdatePins) {
	const bool bDifferentInterface = (SettingsInterface.Get() != InSettingsInterface);
	if (bDifferentInterface && SettingsInterface)
	{
#if WITH_EDITOR
		SettingsInterface->OnSettingsChangedDelegate.RemoveAll(this);
#endif

		// Un-outer the current settings to disassociate old settings from node. Without this one can copy paste
		// a node and get both settings objects in the clipboard text, and the wrong settings can be used upon paste.
		if (ensure(SettingsInterface->GetOuter() == this))
		{
#if WITH_EDITOR
			SettingsInterface->Rename(nullptr, GetTransientPackage(), REN_DoNotDirty | REN_DontCreateRedirectors | REN_NonTransactional);
#endif
			SettingsInterface->MarkAsGarbage();
		}
	}

	SettingsInterface = InSettingsInterface;
	
#if WITH_EDITOR
	if (bDifferentInterface && SettingsInterface)
	{
		check(SettingsInterface->GetSettings());
		SettingsInterface->OnSettingsChangedDelegate.AddUObject(this, &UDungeonForgeGraphNode::OnSettingsChanged);
	}
#endif

	/*
	if (bUpdatePins)
	{
		UpdatePins();
	}
	*/
}

UDungeonForgeNodeSettings* UDungeonForgeGraphNode::GetSettings() const {
	return SettingsInterface ? SettingsInterface->GetSettings() : nullptr;
}

void UDungeonForgeGraphNode::UpdateAfterSettingsChangeDuringCreation() {
	UpdatePins();	
}

UDungeonForgeGraphPin* UDungeonForgeGraphNode::GetInputPin(const FName& Label) {
	for (UDungeonForgeGraphPin* InputPin : InputPins)
	{
		if (InputPin->Properties.Label == Label)
		{
			return InputPin;
		}
	}

	return nullptr;
}

const UDungeonForgeGraphPin* UDungeonForgeGraphNode::GetInputPin(const FName& Label) const {
	for (const UDungeonForgeGraphPin* InputPin : InputPins)
	{
		if (InputPin->Properties.Label == Label)
		{
			return InputPin;
		}
	}

	return nullptr;
}

UDungeonForgeGraphPin* UDungeonForgeGraphNode::GetOutputPin(const FName& Label) {
	for (UDungeonForgeGraphPin* OutputPin : OutputPins)
	{
		if (OutputPin->Properties.Label == Label)
		{
			return OutputPin;
		}
	}

	return nullptr;
}

const UDungeonForgeGraphPin* UDungeonForgeGraphNode::GetOutputPin(const FName& Label) const {
	for (const UDungeonForgeGraphPin* OutputPin : OutputPins)
	{
		if (OutputPin->Properties.Label == Label)
		{
			return OutputPin;
		}
	}

	return nullptr;
}

bool UDungeonForgeGraphNode::HasInboundEdges() const {
	for (const UDungeonForgeGraphPin* InputPin : InputPins) {
		for (const UDungeonForgeGraphEdge* InboundEdge : InputPin->Edges) {
			if (InboundEdge->IsValid()) {
				return true;
			}
		}
	}

	return false;
}

int32 UDungeonForgeGraphNode::GetInboundEdgesNum() const {
	int32 NumInboundEdges = 0;

	for (const UDungeonForgeGraphPin* InputPin : InputPins)
	{
		check(InputPin);
		NumInboundEdges += InputPin->EdgeCount();
	}
	
	return NumInboundEdges;
}

EDungeonForgeGraphChangeType UDungeonForgeGraphNode::UpdatePins() {
	return UpdatePins([](UDungeonForgeGraphNode* Node){ return NewObject<UDungeonForgeGraphPin>(Node); });
}

EDungeonForgeGraphChangeType UDungeonForgeGraphNode::UpdatePins(TFunctionRef<UDungeonForgeGraphPin*(UDungeonForgeGraphNode*)> PinAllocator) {
	TSet<UDungeonForgeGraphNode*> TouchedNodes;
	const UDungeonForgeNodeSettings* Settings = GetSettings();
	
	if (!Settings)
	{
		bool bChanged = !InputPins.IsEmpty() || !OutputPins.IsEmpty();

		if (bChanged)
		{
			Modify();
		}

		// Clean up edges
		for (UDungeonForgeGraphPin* Pin : InputPins)
		{
			if (Pin)
			{
				Pin->BreakAllEdges(&TouchedNodes);
			}
		}

		for (UDungeonForgeGraphPin* Pin : OutputPins)
		{
			if (Pin)
			{
				Pin->BreakAllEdges(&TouchedNodes);
			}
		}

		InputPins.Reset();
		OutputPins.Reset();
		return EDungeonForgeGraphChangeType::Edge | EDungeonForgeGraphChangeType::Node;
	}
	
	TArray<FDungeonForgePinProperties> InboundPinProperties = Settings->AllInputPinProperties();
	TArray<FDungeonForgePinProperties> OutboundPinProperties = Settings->AllOutputPinProperties();

	auto RemoveDuplicates = [this](TArray<FDungeonForgePinProperties>& Properties)
	{
		for (int32 i = Properties.Num() - 2; i >= 0; --i)
		{
			for (int32 j = i + 1; j < Properties.Num(); ++j)
			{
				if (Properties[i].Label == Properties[j].Label)
				{
					const UDungeonForgeGraph* ForgeGraph = GetGraph();
					const FString GraphName = ForgeGraph ? ForgeGraph->GetName() : FString(TEXT("Unknown"));
					UE_LOG(LogDungeonForge, Warning, TEXT("UpdatePins: Pin properties from the settings on node '%s' in graph '%s' contained a duplicate pin '%s', removing this pin properties."),
						*GetName(), *GraphName, *Properties[i].Label.ToString());

					// Remove but preserve order
					Properties.RemoveAt(j);
					break;
				}
			}
		}
	};
	RemoveDuplicates(InboundPinProperties);
	RemoveDuplicates(OutboundPinProperties);

	auto UpdatePins = [this, &PinAllocator, &TouchedNodes](TArray<UDungeonForgeGraphPin*>& Pins, const TArray<FDungeonForgePinProperties>& PinProperties)
	{
		bool bAppliedEdgeChanges = false;
		bool bChangedPins = false;
		bool bChangedTooltips = false;

		// Find unmatched pins vs. properties on a name basis
		TArray<UDungeonForgeGraphPin*> UnmatchedPins;
		for (UDungeonForgeGraphPin* Pin : Pins)
		{
			if (const FDungeonForgePinProperties* MatchingProperties = PinProperties.FindByPredicate([Pin](const FDungeonForgePinProperties& Prop) { return Prop.Label == Pin->Properties.Label; }))
			{
				if (!(Pin->Properties == *MatchingProperties))
				{
					Pin->Modify();
					Pin->Properties = *MatchingProperties;

					bAppliedEdgeChanges |= Pin->BreakAllIncompatibleEdges(&TouchedNodes);
					bChangedPins = true;
				}
#if WITH_EDITOR
				else if (Pin->Properties.Tooltip.CompareTo(MatchingProperties->Tooltip))
				{
					Pin->Properties.Tooltip = MatchingProperties->Tooltip;
					bChangedTooltips = true;
				}
#endif // WITH_EDITOR
			}
			else
			{
				UnmatchedPins.Add(Pin);
			}
		}

		// Find unmatched properties vs pins on a name basis
		TArray<FDungeonForgePinProperties> UnmatchedProperties;
		for (const FDungeonForgePinProperties& Properties : PinProperties)
		{
			if (!Pins.FindByPredicate([&Properties](const UDungeonForgeGraphPin* Pin) { return Pin->Properties.Label == Properties.Label; }))
			{
				UnmatchedProperties.Add(Properties);
			}
		}

		bool bWasModified = false;
		const bool bUpdateUnmatchedPin = UnmatchedPins.Num() == 1 && UnmatchedProperties.Num() == 1;
		if (bUpdateUnmatchedPin)
		{
			UnmatchedPins[0]->Modify();
			UnmatchedPins[0]->Properties = UnmatchedProperties[0];

			bAppliedEdgeChanges |= UnmatchedPins[0]->BreakAllIncompatibleEdges(&TouchedNodes);
			bChangedPins = true;
		}
		else
		{
			// Verification that we don't have 2 pins with the same name
			// If so, mark them to be removed.
			TSet<FName> AllPinNames;
			TArray<UDungeonForgeGraphPin*> DuplicatedNamePins;

			for (UDungeonForgeGraphPin* Pin : Pins)
			{
				if (AllPinNames.Contains(Pin->Properties.Label))
				{
					DuplicatedNamePins.Add(Pin);
				}

				AllPinNames.Add(Pin->Properties.Label);
			}

			if(!UnmatchedPins.IsEmpty() || !UnmatchedProperties.IsEmpty() || !DuplicatedNamePins.IsEmpty())
			{
				bWasModified = true;
				Modify();
				bChangedPins = true;
			}

			auto RemovePins = [&Pins, &AllPinNames, &bAppliedEdgeChanges, &TouchedNodes](TArray<UDungeonForgeGraphPin*>& PinsToRemove, bool bRemoveFromAllNames)
			{
				for (int32 RemovedPinIndex = PinsToRemove.Num() - 1; RemovedPinIndex >= 0; --RemovedPinIndex)
				{
					const int32 PinIndex = Pins.IndexOfByKey(PinsToRemove[RemovedPinIndex]);
					if (PinIndex >= 0)
					{
						if (bRemoveFromAllNames)
						{
							AllPinNames.Remove(Pins[PinIndex]->Properties.Label);
						}

						bAppliedEdgeChanges |= Pins[PinIndex]->BreakAllEdges(&TouchedNodes);
						Pins.RemoveAt(PinIndex);
					}
				}
			};

			RemovePins(UnmatchedPins, /*bRemoveFromAllNames=*/ true);
			RemovePins(DuplicatedNamePins, /*bRemoveFromAllNames=*/ false);

			// Add new pins
			for (const FDungeonForgePinProperties& UnmatchedProperty : UnmatchedProperties)
			{
				if (ensure(!AllPinNames.Contains(UnmatchedProperty.Label)))
				{
					AllPinNames.Add(UnmatchedProperty.Label);

					const int32 InsertIndex = FMath::Min(PinProperties.IndexOfByKey(UnmatchedProperty), Pins.Num());
					UDungeonForgeGraphPin* NewPin = PinAllocator(this);
					NewPin->Modify();
					NewPin->Node = this;
					NewPin->Properties = UnmatchedProperty;
					Pins.Insert(NewPin, InsertIndex);
				}
			}
		}

		// Final pass, to check the order. We re-order if the order is not the same in PinProperties and Pins, without breaking the edges.
		// Also, at this point, we should have the same number of items in the pins and in the pin properties
		check(Pins.Num() == PinProperties.Num());
		for (int32 i = 0; i < PinProperties.Num(); ++i)
		{
			const FDungeonForgePinProperties& CurrentPinProperties = PinProperties[i];
			int32 AssociatedPinPindex = Pins.IndexOfByPredicate([&CurrentPinProperties](const UDungeonForgeGraphPin* Pin) -> bool { return Pin->Properties.Label == CurrentPinProperties.Label; });
			if (i != AssociatedPinPindex)
			{
				if (!bWasModified)
				{
					bWasModified = true;
					Modify();
				}

				bChangedPins = true;
				Pins.Swap(i, AssociatedPinPindex);
			}
		}

		return (bAppliedEdgeChanges ? EDungeonForgeGraphChangeType::Edge : EDungeonForgeGraphChangeType::None)
			| (bChangedPins ? EDungeonForgeGraphChangeType::Node : EDungeonForgeGraphChangeType::None)
			| (bChangedTooltips ? EDungeonForgeGraphChangeType::Cosmetic : EDungeonForgeGraphChangeType::None);
	};

	EDungeonForgeGraphChangeType ChangeType = EDungeonForgeGraphChangeType::None;
	ChangeType |= UpdatePins(MutableView(InputPins), InboundPinProperties);
	ChangeType |= UpdatePins(MutableView(OutputPins), OutboundPinProperties);

#if WITH_EDITOR
	for (UDungeonForgeGraphNode* Node : TouchedNodes)
	{
		if (Node)
		{
			// Only this node gets full change type
			Node->OnNodeChangedDelegate.Broadcast(Node, (Node == this) ? ChangeType : EDungeonForgeGraphChangeType::Node);
		}
	}
#endif // WITH_EDITOR

	return ChangeType;
}

void UDungeonForgeGraphNode::CreateDefaultPins(TFunctionRef<UDungeonForgeGraphPin*(UDungeonForgeGraphNode*)> PinAllocator) {
	
}

#if WITH_EDITOR
void UDungeonForgeGraphNode::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) {
	UObject::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UDungeonForgeGraphNode, NodeTitle)) {
		OnNodeChangedDelegate.Broadcast(this, EDungeonForgeGraphChangeType::Cosmetic);
	}
}

void UDungeonForgeGraphNode::OnSettingsChanged(UDungeonForgeNodeSettings* InSettings, EDungeonForgeGraphChangeType ChangeType) {
	if (InSettings == GetSettings()) {
		OnNodeChangedDelegate.Broadcast(this, ChangeType);
	}
}
#endif // WITH_EDITOR

UDungeonForgeGraph* UDungeonForgeGraphNode::GetGraph() const {
	return Cast<UDungeonForgeGraph>(GetOuter());	
}

UDungeonForgeGraphNode* UDungeonForgeGraphNode::AddEdgeTo(FName FromPinLabel, UDungeonForgeGraphNode* To, FName ToPinLabel) {
	if (UDungeonForgeGraph* Graph = GetGraph())
	{
		return Graph->AddEdge(this, FromPinLabel, To, ToPinLabel);
	}
	else
	{
		return nullptr;
	}
}

bool UDungeonForgeGraphNode::RemoveEdgeTo(FName FromPinLabel, UDungeonForgeGraphNode* To, FName ToPinLabel) {
	if (UDungeonForgeGraph* Graph = GetGraph())
	{
		return Graph->RemoveEdge(this, FromPinLabel, To, ToPinLabel);
	}
	else
	{
		return false;
	}
}

FText UDungeonForgeGraphNode::GetNodeTitle(EDungeonForgeNodeTitleType TitleType) const {
	// Title length that looks reasonable.
	constexpr int NodeTitleMaxLen = 45;

	// Clip string at right hand side (standard overflow).
	auto ClipRightSide = [NodeTitleMaxLen](FString& InOutTitle, int MaxLen)
	{
		if (InOutTitle.Len() > MaxLen)
		{
			InOutTitle = InOutTitle.Left(MaxLen - 3) + TEXT("...");
		}
	};

	// Clip left hand side to maximize displayed information at end of string.
	auto ClipLeftSide = [NodeTitleMaxLen](FString& InOutTitle, int MaxLen)
	{
		if (InOutTitle.Len() > MaxLen)
		{
			InOutTitle = TEXT("...") + InOutTitle.Right(MaxLen - 3);
		}
	};

	const bool bFlipTitleLines = HasFlippedTitleLines();
	const bool bIsTitleAuthored = HasAuthoredTitle();

	FString GeneratedTitle = GetGeneratedTitleLine().ToString();
	const bool bHasMultipleTitleLines = !GeneratedTitle.IsEmpty();

	FString PrimaryTitleLine;

	// The normal title line is used if there is no generated title, or if the node has not requested flipped
	// title lines. But if the user has edited the title, always use the users title as primary. 
	if (!bHasMultipleTitleLines || !bFlipTitleLines || bIsTitleAuthored)
	{
		PrimaryTitleLine = GetAuthoredTitleLine().ToString();
		ClipRightSide(PrimaryTitleLine, NodeTitleMaxLen);
	}
	else
	{
		PrimaryTitleLine = GeneratedTitle;

		// Generated title clipped on left side because the end of the string often has the interesting part.
		ClipLeftSide(PrimaryTitleLine, NodeTitleMaxLen);
	}

	// Only add a second title line if the full title is being requested (and if the has multiple title lines).
	if (TitleType == EDungeonForgeNodeTitleType::FullTitle)
	{
		FString SecondaryTitleLine;

		if (bHasMultipleTitleLines)
		{
			// Secondary title is the node-generated title line if titles aren't flipped.
			if (!bFlipTitleLines)
			{
				SecondaryTitleLine = GeneratedTitle;

				// Generated title clipped on left side because the end of the string often has the interesting part.
				ClipLeftSide(SecondaryTitleLine, NodeTitleMaxLen);
			}
			// If titles are flipped and user has not authored something, then just show the standard node name.
			else if (!bIsTitleAuthored)
			{
				SecondaryTitleLine = GetDefaultTitle().ToString();
				ClipRightSide(SecondaryTitleLine, NodeTitleMaxLen);
			}
			// If title has been authored and title lines are flipped, display "<DefaultTitle> - <GeneratedTitle>"
			else
			{
				const FString DefaultTitle = GetDefaultTitle().ToString();

				// Generated title clipped on left side because the end of the string often has the interesting part.
				ClipLeftSide(GeneratedTitle, NodeTitleMaxLen - (DefaultTitle.Len() + 3));
				SecondaryTitleLine = DefaultTitle + TEXT(" - ") + GeneratedTitle;
			}
		}

		// Debug info - append how many copies of this element are currently in the cache to the node title.
#if WITH_EDITOR
		bool bDebuggingEnabled = false;
		uint32 NumCacheEntries = 0;
		//DungeonForgeNodeHelpers::GetGraphCacheDebugInfo(this, bDebuggingEnabled, NumCacheEntries);

		if (bDebuggingEnabled)
		{
			SecondaryTitleLine += FString::Format(TEXT(" [{0}]"), { NumCacheEntries });
		}
#endif

		if (!SecondaryTitleLine.IsEmpty())
		{
			return FText::Format(FText::FromString("{0}\r\n{1}"), FText::FromString(PrimaryTitleLine), FText::FromString(SecondaryTitleLine));
		}
	}

	return FText::FromString(PrimaryTitleLine);
}

bool UDungeonForgeGraphNode::HasAuthoredTitle() const {
	return NodeTitle != NAME_None;
}

FText UDungeonForgeGraphNode::GetDefaultTitle() const {
#if WITH_EDITOR
	if (UDungeonForgeNodeSettings* Settings = GetSettings())
	{
		return Settings->GetDefaultNodeTitle();
	}
#endif

	return NSLOCTEXT("PCGNode", "NodeTitle", "Unnamed Node");
}

FText UDungeonForgeGraphNode::GetAuthoredTitleLine() const {
	if (NodeTitle != NAME_None)
	{
		return FText::FromString(FName::NameToDisplayString(NodeTitle.ToString(), false));
	}
	else
	{
		return GetDefaultTitle();
	}
}

bool UDungeonForgeGraphNode::HasFlippedTitleLines() const {
	const UDungeonForgeNodeSettings* Settings = GetSettings();
	return Settings && Settings->HasFlippedTitleLines();
}

FText UDungeonForgeGraphNode::GetGeneratedTitleLine() const {
	if (UDungeonForgeNodeSettings* Settings = GetSettings())
	{
		const FString AdditionalInformation = Settings->GetAdditionalTitleInformation();
		if (!AdditionalInformation.IsEmpty())
		{
			return FText::FromString(AdditionalInformation);
		}
	}

	return FText::GetEmpty();
}

#if WITH_EDITOR
FText UDungeonForgeGraphNode::GetNodeTooltipText() const {
	if (UDungeonForgeNodeSettings* Settings = GetSettings())
	{
		return Settings->GetNodeTooltipText();
	}
	else
	{
		return FText::GetEmpty();
	}
}
#endif // WITH_EDITOR


 

