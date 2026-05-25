//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "CoreMinimal.h"

#include "Frameworks/Canvas/DungeonCanvasViewport.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonCanvasViewportTransformTest, "DungeonArchitectTests.DungeonCanvasViewportTransform.WorldToCanvasLocation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDungeonCanvasViewportTransformTest::RunTest(const FString& Parameters) {
	const FTransform LocalToWorld(FRotator::ZeroRotator, FVector::Zero(), FVector(1000, 1000, 400));
	const FTransform LocalToCanvas(FRotator::ZeroRotator, FVector(1024, 1024, 0), FVector(2048, 2048, 0));

	const FVector2D CanvasSize(2048, 2048);
	FDungeonCanvasViewportTransform Transform;
	Transform.SetLocalToWorld(LocalToWorld);
	Transform.FocusOnCanvas(CanvasSize.X, CanvasSize.Y);
	
	// Test Case 1: Test a known world location
	const FVector TestWorldLocation(500, 500, 100); // Replace with a relevant test location
	const FVector2D ExpectedCanvasLocation(2048, 2048); // Replace with the expected canvas location for the given world location

	const FVector2D CanvasLocation = Transform.WorldToCanvasLocation(TestWorldLocation);
	if (!TestEqual(TEXT("WorldToCanvasLocation should correctly transform world to canvas coordinates"), CanvasLocation, ExpectedCanvasLocation))
	{
		return false;
	}

	return true;
}

