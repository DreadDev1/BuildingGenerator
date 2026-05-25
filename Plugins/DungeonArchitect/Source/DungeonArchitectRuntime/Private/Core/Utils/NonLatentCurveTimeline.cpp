//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Utils/NonLatentCurveTimeline.h"

#include "Curves/CurveFloat.h"
#include "Engine/Engine.h"

void UNonLatentCurveTimelineBlueprintFunctionLib::TickNonLatentTimelineForward(float DeltaSeconds, UPARAM(ref) FNonLatentCurveTimeline& Timeline) {
	TickImpl(Timeline, DeltaSeconds);
}

void UNonLatentCurveTimelineBlueprintFunctionLib::TickNonLatentTimelineBackward(float DeltaSeconds, UPARAM(ref) FNonLatentCurveTimeline& Timeline) {
	TickImpl(Timeline, -DeltaSeconds);
}

float UNonLatentCurveTimelineBlueprintFunctionLib::GetNonLatentTimelineValue(const FNonLatentCurveTimeline& Timeline) {
	if (Timeline.Curve) {
		float MinTime{}, MaxTime{};
		Timeline.Curve->GetTimeRange(MinTime, MaxTime);
		const float ClampedTime = FMath::Clamp(Timeline.Time, MinTime, MaxTime);
		return Timeline.Curve->GetFloatValue(ClampedTime);
	}
	else {
		return 0;
	}
}

void UNonLatentCurveTimelineBlueprintFunctionLib::TickImpl(FNonLatentCurveTimeline& Timeline, float DeltaSeconds) {
	if (Timeline.Curve) {
		float MinTime{}, MaxTime{};
		Timeline.Curve->GetTimeRange(MinTime, MaxTime);
		Timeline.Time += DeltaSeconds;
		Timeline.Time = FMath::Clamp(Timeline.Time, MinTime, MaxTime);
	}
}


