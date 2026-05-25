//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Utils/HeatmapColorRamp.h"

#include "Curves/CurveLinearColor.h"

UHeatmapColorRamp::UHeatmapColorRamp()
	: WrapDistance(1000.0f)
{
	// Create a default color ramp
	ColorRamp = CreateDefaultSubobject<UCurveLinearColor>(TEXT("DefaultColorRamp"));
    
	FRichCurve* RichCurves[4] = { 
		&ColorRamp->FloatCurves[0], &ColorRamp->FloatCurves[1], 
		&ColorRamp->FloatCurves[2], &ColorRamp->FloatCurves[3] 
	};

	// Dark Blue
	RichCurves[0]->AddKey(0.0f, 0.0f);
	RichCurves[1]->AddKey(0.0f, 0.0f);
	RichCurves[2]->AddKey(0.0f, 0.5f);
	RichCurves[3]->AddKey(0.0f, 1.0f);

	// Light Blue
	RichCurves[0]->AddKey(0.25f, 0.0f);
	RichCurves[1]->AddKey(0.25f, 0.5f);
	RichCurves[2]->AddKey(0.25f, 1.0f);
	RichCurves[3]->AddKey(0.25f, 1.0f);

	// Green
	RichCurves[0]->AddKey(0.5f, 0.0f);
	RichCurves[1]->AddKey(0.5f, 1.0f);
	RichCurves[2]->AddKey(0.5f, 0.0f);
	RichCurves[3]->AddKey(0.5f, 1.0f);

	// Yellow
	RichCurves[0]->AddKey(0.75f, 1.0f);
	RichCurves[1]->AddKey(0.75f, 1.0f);
	RichCurves[2]->AddKey(0.75f, 0.0f);
	RichCurves[3]->AddKey(0.75f, 1.0f);

	// Red
	RichCurves[0]->AddKey(1.0f, 1.0f);
	RichCurves[1]->AddKey(1.0f, 0.0f);
	RichCurves[2]->AddKey(1.0f, 0.0f);
	RichCurves[3]->AddKey(1.0f, 1.0f);
}

FLinearColor UHeatmapColorRamp::GetColorForDistance(float Distance) const
{
	if (!ColorRamp)
	{
		return FLinearColor::White;
	}

	// Wrap the distance value
	float WrappedDistance = FMath::Fmod(FMath::Abs(Distance), WrapDistance);

	// Normalize the wrapped distance to 0-1 range
	float NormalizedDistance = WrappedDistance / WrapDistance;

	// Get the color from the curve
	return ColorRamp->GetLinearColorValue(NormalizedDistance);
}
  

