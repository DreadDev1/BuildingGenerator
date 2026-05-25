#pragma once

#include "CoreMinimal.h"
#include "RoomPlacement.h"

// Static direction helpers for ERoomFace.
// No UHT reflection — plain C++ utility, no .generated.h.
struct FBuildingMath
{
	// Unit step in grid space for each face (+X, -X, +Y, -Y).
	static FIntPoint ToIntPoint(ERoomFace Face)
	{
		switch (Face)
		{
		case ERoomFace::North: return FIntPoint( 0,  1);
		case ERoomFace::South: return FIntPoint( 0, -1);
		case ERoomFace::East:  return FIntPoint( 1,  0);
		case ERoomFace::West:  return FIntPoint(-1,  0);
		default:               return FIntPoint( 0,  0);
		}
	}

	// Yaw (degrees) to orient a BottomBackCenter-pivoted wall mesh so its back faces outward.
	// Default (0°) has the back face pointing West (-X). Each step is 90° CCW.
	static float ToYawDegrees(ERoomFace Face)
	{
		switch (Face)
		{
		case ERoomFace::West:  return   0.f;
		case ERoomFace::South: return  90.f;
		case ERoomFace::East:  return 180.f;
		case ERoomFace::North: return 270.f;
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
