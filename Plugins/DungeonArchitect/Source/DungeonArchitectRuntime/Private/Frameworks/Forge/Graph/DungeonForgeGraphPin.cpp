//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/Graph/DungeonForgeGraphPin.h"

#include "Frameworks/Forge/DungeonForgeNodeSettings.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphEdge.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

namespace PCGPin
{
	bool SortPinsAndRetrieveTypes(const UDungeonForgeGraphPin* InPinA, const UDungeonForgeGraphPin* InPinB, const UDungeonForgeGraphPin*& OutUpstreamPin, const UDungeonForgeGraphPin*& OutDownstreamPin, EDungeonForgeDataType& OutUpstreamTypes, EDungeonForgeDataType& OutDownstreamTypes)
	{
		check(InPinA && InPinB);
		const bool bPinAIsOutput = InPinA->IsOutputPin();

		if (!ensure(bPinAIsOutput != InPinB->IsOutputPin()))
		{
			// Cannot connect two pins of same polarity
			return false;
		}

		OutUpstreamPin = bPinAIsOutput ? InPinA : InPinB;
		OutDownstreamPin = bPinAIsOutput ? InPinB : InPinA;

		check(OutUpstreamPin && OutDownstreamPin);
		OutUpstreamTypes = OutUpstreamPin->GetCurrentTypes();
		OutDownstreamTypes = OutDownstreamPin->Properties.AllowedTypes;

		return true;
	}
}

#if WITH_EDITOR
namespace DungeonForgePinPropertiesHelpers
{
	bool GetDefaultPinExtraIcon(const FDungeonForgePinProperties& InPinProperties, FName& OutExtraIcon, FText& OutTooltip)
	{
		/*
		if (InPinProperties.Usage == EDungeonForgePinUsage::Loop)
		{
			OutTooltip = NSLOCTEXT("PCGPins", "LoopTooltip", "Loop pin, data collection will be split to one data per execution.");
			OutExtraIcon = PCGPinConstants::Icons::LoopPinIcon;
			return true;
		}
		else if (InPinProperties.Usage == EDungeonForgePinUsage::Feedback)
		{
			OutTooltip = NSLOCTEXT("PCGPins", "FeedbackTooltip", "Feedback pin, will daisy-chain results from graph entry or previous loop iteration.");
			OutExtraIcon = PCGPinConstants::Icons::FeedbackPinIcon;
			return true;
		}
		*/
		return false;
	}
	
	bool GetDefaultPinExtraIcon(const UDungeonForgeGraphPin* InPin, FName& OutExtraIcon, FText& OutTooltip)
	{
		return InPin && GetDefaultPinExtraIcon(InPin->Properties, OutExtraIcon, OutTooltip);
	}

}
#endif // WITH_EDITOR

FDungeonForgePinProperties::FDungeonForgePinProperties(const FName& InLabel, EDungeonForgeDataType InAllowedTypes, bool bInAllowMultipleConnections, bool bInAllowMultipleData, const FText& InTooltip)
	: Label(InLabel), AllowedTypes(InAllowedTypes), bAllowMultipleData(bInAllowMultipleData)
#if WITH_EDITORONLY_DATA
	, Tooltip(InTooltip)
#endif
{
	SetAllowMultipleConnections(bInAllowMultipleConnections);
}

void FDungeonForgePinProperties::SetAllowMultipleConnections(bool bInAllowMultipleConnectons)
{
	if (bInAllowMultipleConnectons)
	{
		bAllowMultipleConnections = true;
		bAllowMultipleData = true;
	}
	else
	{
		bAllowMultipleConnections = false;
	}
}

bool FDungeonForgePinProperties::operator==(const FDungeonForgePinProperties& Other) const
{
	return Label == Other.Label &&
		AllowedTypes == Other.AllowedTypes &&
		bAllowMultipleConnections == Other.bAllowMultipleConnections &&
		bAllowMultipleData == Other.bAllowMultipleData &&
		Usage == Other.Usage &&
		PinStatus == Other.PinStatus &&
		bInvisiblePin == Other.bInvisiblePin;
}

uint32 GetTypeHash(const FDungeonForgePinProperties& Value)
{
	uint32 Hash = GetTypeHash(Value.Label);
	Hash = HashCombine(Hash, static_cast<uint32>(Value.AllowedTypes));
	Hash = HashCombine(Hash, Value.bAllowMultipleConnections);
	Hash = HashCombine(Hash, Value.bAllowMultipleData);
	Hash = HashCombine(Hash, static_cast<uint32>(Value.Usage));
	Hash = HashCombine(Hash, static_cast<uint32>(Value.PinStatus));
	Hash = HashCombine(Hash, Value.bInvisiblePin);

	return Hash;
}

void FDungeonForgePinProperties::PostSerialize(const FArchive& Ar)
{
#if WITH_EDITOR
	if (Ar.IsLoading() && Ar.IsPersistent() && !Ar.HasAnyPortFlags(PPF_Duplicate | PPF_DuplicateForPIE))
	{
		if (bAdvancedPin_DEPRECATED)
		{
			PinStatus = EDungeonForgePinStatus::Advanced;
		}

		bAdvancedPin_DEPRECATED = false;
	}
#endif // WITH_EDITOR
}

#if WITH_EDITOR
bool FDungeonForgePinProperties::CanEditChange(const FEditPropertyChain& PropertyChain) const
{
	if (FProperty* Property = PropertyChain.GetActiveNode()->GetValue())
	{
		if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(FDungeonForgePinProperties, bAllowMultipleData))
		{
			return bAllowEditMultipleData;
		}
		else if (Property->GetFName() == GET_MEMBER_NAME_CHECKED(FDungeonForgePinProperties, bAllowMultipleConnections))
		{
			return bAllowMultipleData && bAllowEditMultipleConnections;
		}
	}

	return true;
}
#endif // WITH_EDITOR

UDungeonForgeGraphPin::UDungeonForgeGraphPin(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetFlags(RF_Transactional);
}

void UDungeonForgeGraphPin::PostLoad()
{
	Super::PostLoad();
}

FText UDungeonForgeGraphPin::GetTooltip() const
{
#if WITH_EDITOR
	return Properties.Tooltip;
#else
	return FText::GetEmpty();
#endif
}

void UDungeonForgeGraphPin::SetTooltip(const FText& InTooltip)
{
#if WITH_EDITOR
	Properties.Tooltip = InTooltip;
#endif
}

bool UDungeonForgeGraphPin::AddEdgeTo(UDungeonForgeGraphPin* OtherPin, TSet<UDungeonForgeGraphNode*>* InTouchedNodes/*= nullptr*/)
{
	if (!OtherPin)
	{
		return false;
	}
	
	for (UDungeonForgeGraphEdge* Edge : Edges)
	{
		if (Edge->GetOtherPin(this) == OtherPin)
		{
			return false;
		}
	}

	// This pin is upstream if the pin is an output pin
	const bool bThisPinIsUpstream = IsOutputPin();
	const bool bOtherPinIsUpstream = OtherPin->IsOutputPin();

	// Pins should not both be upstream or both be downstream..
	if (!ensure(bThisPinIsUpstream != bOtherPinIsUpstream))
	{
		return false;
	}

	Modify();
	OtherPin->Modify();

	UDungeonForgeGraphEdge* NewEdge = Edges.Add_GetRef(NewObject<UDungeonForgeGraphEdge>(this));
	OtherPin->Edges.Add(NewEdge);
	
	NewEdge->Modify();
	NewEdge->InputPin = bThisPinIsUpstream ? this : OtherPin;
	NewEdge->OutputPin = bThisPinIsUpstream ? OtherPin : this;

	if (InTouchedNodes)
	{
		InTouchedNodes->Add(Node);
		InTouchedNodes->Add(OtherPin->Node);
	}

	return true;
}

bool UDungeonForgeGraphPin::BreakEdgeTo(UDungeonForgeGraphPin* OtherPin, TSet<UDungeonForgeGraphNode*>* InTouchedNodes/*= nullptr*/)
{
	if (!OtherPin)
	{
		return false;
	}

	for (UDungeonForgeGraphEdge* Edge : Edges)
	{
		if (Edge->GetOtherPin(this) == OtherPin)
		{
			Modify();
			OtherPin->Modify();

			ensure(OtherPin->Edges.Remove(Edge));
			Edges.Remove(Edge);

			if (InTouchedNodes)
			{
				InTouchedNodes->Add(Node);
				InTouchedNodes->Add(OtherPin->Node);
			}

			return true;
		}
	}

	return false;
}

bool UDungeonForgeGraphPin::BreakAllEdges(TSet<UDungeonForgeGraphNode*>* InTouchedNodes/*= nullptr*/)
{
	bool bChanged = false;

	if (!Edges.IsEmpty())
	{
		if (InTouchedNodes)
		{
			InTouchedNodes->Add(Node);
		}

		Modify();
	}

	for (UDungeonForgeGraphEdge* Edge : Edges)
	{
		if (UDungeonForgeGraphPin* OtherPin = Edge->GetOtherPin(this))
		{
			OtherPin->Modify();
			ensure(OtherPin->Edges.Remove(Edge));
			bChanged = true;

			if (InTouchedNodes)
			{
				InTouchedNodes->Add(OtherPin->Node);
			}
		}
	}

	Edges.Reset();

	return bChanged;
}

bool UDungeonForgeGraphPin::BreakAllIncompatibleEdges(TSet<UDungeonForgeGraphNode*>* InTouchedNodes/*= nullptr*/)
{
	bool bChanged = false;
	bool bHasAValidEdge = false;

	for (int32 EdgeIndex = Edges.Num() - 1; EdgeIndex >= 0; --EdgeIndex)
	{
		UDungeonForgeGraphEdge* Edge = Edges[EdgeIndex];
		UDungeonForgeGraphPin* OtherPin = Edge->GetOtherPin(this);

		bool bRemoveEdge = !IsCompatible(OtherPin) || (!AllowsMultipleConnections() && bHasAValidEdge);

		if (bRemoveEdge)
		{
			Modify();
			Edges.RemoveAtSwap(EdgeIndex);

			if (InTouchedNodes)
			{
				InTouchedNodes->Add(Node);
			}

			if (OtherPin)
			{
				OtherPin->Modify();
				ensure(OtherPin->Edges.Remove(Edge));
				bChanged = true;

				if (InTouchedNodes)
				{
					InTouchedNodes->Add(OtherPin->Node);
				}
			}
		}
		else
		{
			bHasAValidEdge = true;
		}
	}

	return bChanged;
}

bool UDungeonForgeGraphPin::IsConnected() const
{
	for (const UDungeonForgeGraphEdge* Edge : Edges)
	{
		if (Edge->IsValid())
		{
			return true;
		}
	}

	return false;
}

bool UDungeonForgeGraphPin::IsOutputPin() const
{
	check(Node);
	return Node->GetOutputPin(Properties.Label) == this;
}

int32 UDungeonForgeGraphPin::EdgeCount() const
{
	int32 EdgeNum = 0;
	for (const UDungeonForgeGraphEdge* Edge : Edges)
	{
		if (Edge->IsValid())
		{
			++EdgeNum;
		}
	}

	return EdgeNum;
}

EDungeonForgeDataType UDungeonForgeGraphPin::GetCurrentTypes() const
{
	check(Node);
	const UDungeonForgeNodeSettings* Settings = Node->GetSettings();
	return Settings ? Settings->GetCurrentPinTypes(this) : Properties.AllowedTypes;
}

bool UDungeonForgeGraphPin::IsCompatible(const UDungeonForgeGraphPin* OtherPin) const
{
	if (!OtherPin)
	{
		return false;
	}

	const UDungeonForgeGraphPin* UpstreamPin = nullptr;
	const UDungeonForgeGraphPin* DownstreamPin = nullptr;
	EDungeonForgeDataType UpstreamTypes = EDungeonForgeDataType::None, DownstreamTypes = EDungeonForgeDataType::None;
	if (!PCGPin::SortPinsAndRetrieveTypes(this, OtherPin, UpstreamPin, DownstreamPin, UpstreamTypes, DownstreamTypes))
	{
		return false;
	}
	check(UpstreamPin && DownstreamPin);

	// Types missing
	if (UpstreamTypes == EDungeonForgeDataType::None || DownstreamTypes == EDungeonForgeDataType::None)
	{
		return false;
	}

	// Concrete can always be used as a composite - allow connections from concrete to composite
	const bool bUpstreamInConcrete = !(UpstreamTypes & ~EDungeonForgeDataType::Concrete);
	const bool bDownstreamIsSpatial = (DownstreamTypes == EDungeonForgeDataType::Spatial);
	if (bUpstreamInConcrete && bDownstreamIsSpatial)
	{
		return true;
	}

	// Anything spatial can collapse to point as long as the downstream pin does not support any other spatial types. For example, Point | Param is collapsible, but Point | Surface is not
	const bool bUpstreamIsSpatial = !(UpstreamTypes & ~EDungeonForgeDataType::Spatial);
	const bool bDownstreamIsPoint = (DownstreamTypes & EDungeonForgeDataType::Spatial) == EDungeonForgeDataType::Point;
	if (bUpstreamIsSpatial && bDownstreamIsPoint)
	{
		return true;
	}

	// Spline -> Surface conversion
	if (UpstreamTypes == EDungeonForgeDataType::Spline && DownstreamTypes == EDungeonForgeDataType::Surface)
	{
		return true;
	}

	// Otherwise allow if there is overlap. Don't detect wide -> narrow issues - conversion nodes deal with that
	return !!(UpstreamTypes & DownstreamTypes);
}

bool UDungeonForgeGraphPin::AllowsMultipleConnections() const
{
	// Always allow multiple connection on output pin
	return IsOutputPin() || Properties.AllowsMultipleConnections();
}

bool UDungeonForgeGraphPin::AllowsMultipleData() const
{
	return Properties.bAllowMultipleData;
}

bool UDungeonForgeGraphPin::CanConnect(const UDungeonForgeGraphPin* OtherPin) const
{
	return OtherPin && (Edges.IsEmpty() || AllowsMultipleConnections());
}

EDungeonForgeTypeConversion UDungeonForgeGraphPin::GetRequiredTypeConversion(const UDungeonForgeGraphPin* InOtherPin) const
{
	if (!InOtherPin)
	{
		return EDungeonForgeTypeConversion::Failed;
	}

	const UDungeonForgeGraphPin* UpstreamPin = nullptr;
	const UDungeonForgeGraphPin* DownstreamPin = nullptr;
	EDungeonForgeDataType UpstreamTypes = EDungeonForgeDataType::None;
	EDungeonForgeDataType DownstreamTypes = EDungeonForgeDataType::None;
	if (!PCGPin::SortPinsAndRetrieveTypes(this, InOtherPin, UpstreamPin, DownstreamPin, UpstreamTypes, DownstreamTypes))
	{
		return EDungeonForgeTypeConversion::Failed;
	}
	check(UpstreamPin && DownstreamPin);

	// Types same - trivial early out.
	if (DownstreamTypes == UpstreamTypes)
	{
		return EDungeonForgeTypeConversion::NoConversionRequired;
	}

	// Type missing, or types are the same - trivial early out.
	if (!DownstreamTypes || !UpstreamTypes)
	{
		return EDungeonForgeTypeConversion::Failed;
	}

	// Spatial -> Point - "To Point" conversion.
	// Anything spatial can collapse to point as long as the downstream pin does not support any other spatial types. For example, Point | Param is collapsible, but Point | Surface is not.
	const bool bUpstreamIsSpatial = !(UpstreamTypes & ~EDungeonForgeDataType::Spatial);
	const bool bUpstreamIsPoint = UpstreamTypes == EDungeonForgeDataType::Point;
	const bool bDownstreamIsSpatialAndPointOnly = (DownstreamTypes & EDungeonForgeDataType::Spatial) == EDungeonForgeDataType::Point;
	if (bUpstreamIsSpatial && bDownstreamIsSpatialAndPointOnly)
	{
		return bUpstreamIsPoint ? EDungeonForgeTypeConversion::NoConversionRequired : EDungeonForgeTypeConversion::CollapseToPoint;
	}

	// Spline -> Surface conversion
	if (UpstreamTypes == EDungeonForgeDataType::Spline && DownstreamTypes == EDungeonForgeDataType::Surface)
	{
		return EDungeonForgeTypeConversion::SplineToSurface;
	}

	// Any or Spatial -> Concrete - "Make Concrete" conversion. We decided to support Any as it is a superset of Concrete,
	// and some pins are Any until they dynamically narrow, and other pins may stay Any but still want to be supported.
	bool bUpstreamIsSpatialOrAny = (UpstreamTypes == EDungeonForgeDataType::Any) || (UpstreamTypes == EDungeonForgeDataType::Spatial);
	if (bUpstreamIsSpatialOrAny && DownstreamTypes == EDungeonForgeDataType::Concrete)
	{
		return EDungeonForgeTypeConversion::MakeConcrete;
	}

	// Requires filter if upstream type is broader than downstream type.
	if (!!(UpstreamTypes & (~DownstreamTypes)))
	{
		return EDungeonForgeTypeConversion::Filter;
	}

	return EDungeonForgeTypeConversion::NoConversionRequired;
}

