#pragma once

#include "CoreMinimal.h"
#include "RoomPlacement.h"

// Static direction helpers for ERoomFace.
// No UHT reflection — plain C++ utility, no .generated.h.
struct FBuildingMath
{
	// Unit step in grid space for each face.
	// Designer convention: North=+X, South=-X, East=+Y, West=-Y.
	static FIntPoint ToIntPoint(ERoomFace Face)
	{
		switch (Face)
		{
		case ERoomFace::North: return FIntPoint( 1,  0);
		case ERoomFace::South: return FIntPoint(-1,  0);
		case ERoomFace::East:  return FIntPoint( 0,  1);
		case ERoomFace::West:  return FIntPoint( 0, -1);
		default:               return FIntPoint( 0,  0);
		}
	}

	// Yaw (degrees) to orient a BottomBackCenter-pivoted wall mesh so its back faces outward.
	// The mesh's back faces world -X at 0°; each +90° step rotates the back -X -> -Y -> +X -> +Y.
	// With North=+X / East=+Y, the outward direction per face yields:
	//   South(-X)=0°, West(-Y)=90°, North(+X)=180°, East(+Y)=270°.
	static float ToYawDegrees(ERoomFace Face)
	{
		switch (Face)
		{
		case ERoomFace::South: return   0.f;
		case ERoomFace::West:  return  90.f;
		case ERoomFace::North: return 180.f;
		case ERoomFace::East:  return 270.f;
		default:               return   0.f;
		}
	}

	// The face on the opposite side of a shared boundary.
	static ERoomFace Opposite(ERoomFace Face)
	{
		switch (Face)
		{
		case ERoomFace::North: return ERoomFace::South;
		case ERoomFace::South: return ERoomFace::North;
		case ERoomFace::East:  return ERoomFace::West;
		case ERoomFace::West:  return ERoomFace::East;
		default:               return Face;
		}
	}
};
