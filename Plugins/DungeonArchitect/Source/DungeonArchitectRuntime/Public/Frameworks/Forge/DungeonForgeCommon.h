//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonForgeCommon.generated.h"

class AActor;
using FDungeonForgeTaskId = uint64;
static const FDungeonForgeTaskId InvalidDungeonForgeTaskId = (uint64)-1;


UENUM(meta = (Bitflags))
enum class EDungeonForgeGraphChangeType : uint8
{
	None = 0,
	Cosmetic = 1 << 0,
	Settings = 1 << 1,
	Input = 1 << 2,
	Edge = 1 << 3,
	Node = 1 << 4,
	Structural = 1 << 5
};
ENUM_CLASS_FLAGS(EDungeonForgeGraphChangeType);

// Bitmask containing the various data types supported in Dungeon Forge. Note that this enum cannot be a blueprint type because
// enums have to be uint8 for blueprint, and we already use more than 8 bits in the bitmask.
// This is why we have a parallel enum just below that must match on a name basis 1:1 to allow the make/break functions to work properly
// in blueprint.
// WARNING: Please be mindful that combination of flags that are not explicitly defined there won't be serialized correctly, inducing data loss.
UENUM(meta = (Bitflags))
enum class EDungeonForgeDataType : uint32
{
	None = 0 UMETA(Hidden),
	Point = 1 << 1,

	Spline = 1 << 2,
	LandscapeSpline = 1 << 3,
	PolyLine = Spline | LandscapeSpline UMETA(DisplayName = "Curve"),

	Landscape = 1 << 4,
	Texture = 1 << 5,
	RenderTarget = 1 << 6,
	BaseTexture = Texture | RenderTarget UMETA(Hidden),
	Surface = Landscape | BaseTexture,

	Volume = 1 << 7,
	Primitive = 1 << 8,
	DynamicMesh = 1 << 10,
	Marker = 1 << 11,

	Float = 1 << 12,
	Float2 = 1 << 13,
	Float3 = 1 << 14,
	Float4 = 1 << 15,
	Literal = Float | Float2 | Float3 | Float4,
	
	DungeonLayout = 1 << 16,
	WorldLayout = 1 << 17,
	Layout = DungeonLayout | WorldLayout,
	
	DungeonLayoutTextures = 1 << 18,
	
	/** Simple concrete data. */
	Concrete = Point | PolyLine | Surface | Volume | Primitive | DynamicMesh | Marker | Layout | DungeonLayoutTextures | Literal,

	/** Boolean operations like union, difference, intersection. */
	Composite = 1 << 9 UMETA(Hidden),

	/** Combinations of concrete data and/or boolean operations. */
	Spatial = Composite | Concrete,
	
	Param = 1 << 27 UMETA(DisplayName = "Attribute Set"),

	// Combination of Param and Point, necessary for named-based serialization of enums.
	PointOrParam = Point | Param,

	Settings = 1 << 28 UMETA(Hidden),
	Other = 1 << 29,
	Any = (1 << 30) - 1
};
ENUM_CLASS_FLAGS(EDungeonForgeDataType);


// As discussed just before, a parallel version for "exclusive" (as in only type) of the EDungeonForgeDataType enum. Needed for blueprint compatibility.
UENUM(BlueprintType, meta=(DisplayName="Forge Data Type"))
enum class EDungeonForgeExclusiveDataType : uint8
{
	None = 0 UMETA(Hidden),
	Point,
	Spline,
	LandscapeSpline,
	PolyLine UMETA(DisplayName = "Curve"),
	Landscape,
	Texture,
	RenderTarget,
	BaseTexture UMETA(Hidden),
	Surface,
	Volume,
	Primitive,
	DynamicMesh,
	DungeonLayout,
	WorldLayout,
	Layout,
	Concrete,
	Composite UMETA(Hidden),
	Spatial,
	Param UMETA(DisplayName = "Attribute Set"),
	Settings UMETA(Hidden),
	Other,
	Any,
	PointOrParam
};

UENUM()
enum class EDungeonForgeNodeTitleType : uint8
{
	/** The full title, may be multiple lines. */
	FullTitle,
	/** More concise, single line title. */
	ListView,
};

namespace DungeonForgePinConstants
{
	const FName DefaultInputLabel = TEXT("In");
	const FName DefaultOutputLabel = TEXT("Out");
	const FName DefaultParamsLabel = TEXT("Overrides");
	const FName DefaultDependencyOnlyLabel = TEXT("Dependency Only");

	const FName DefaultInFilterLabel = TEXT("InsideFilter");
	const FName DefaultOutFilterLabel = TEXT("OutsideFilter");

	const FName MarkersLabel = TEXT("Markers");
	
	const FName WorldGraphLabel = TEXT("World Graph");
	const FName DungeonLayoutLabel = TEXT("Dungeon Layout");
	const FName DungeonLayoutTexturesLabel = TEXT("Dungeon Layout Textures");
	const FName TextureLabel = TEXT("Texture");
	const FName MaskLabel = TEXT("Mask");

	
	namespace Private
	{
		const FName OldDefaultParamsLabel = TEXT("Params");
	}

	namespace Icons
	{
		const FName LoopPinIcon = TEXT("GraphEditor.Macro.Loop_16x");
		const FName FeedbackPinIcon = TEXT("GraphEditor.GetSequenceBinding");
	}
}


class FDungeonForgeActorTag {
public:
	explicit FDungeonForgeActorTag(const FGuid& InNodeGuid);

	void ApplyToActor(AActor* InActor) const;
	bool Contains(AActor* InActor) const;

private:
	FName CreateTag() const;
	
private:
	FGuid NodeGuid;
	FName Tag;
}; 

