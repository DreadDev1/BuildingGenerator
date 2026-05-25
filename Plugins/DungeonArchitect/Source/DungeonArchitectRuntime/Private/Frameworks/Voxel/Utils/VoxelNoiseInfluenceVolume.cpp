//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Voxel/Utils/VoxelNoiseInfluenceVolume.h"

#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"

ADAVoxelNoiseInfluenceVolume::ADAVoxelNoiseInfluenceVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Create root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	
	// Create inner box component for visualization
	InnerBoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("InnerBox"));
	InnerBoxComponent->SetupAttachment(RootComponent);
	InnerBoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InnerBoxComponent->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	InnerBoxComponent->SetLineThickness(2.0f);
	InnerBoxComponent->ShapeColor = FColor(0, 255, 0, 255); // Green for inner zone
	
	// Create outer box component for visualization
	OuterBoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("OuterBox"));
	OuterBoxComponent->SetupAttachment(RootComponent);
	OuterBoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OuterBoxComponent->SetBoxExtent(FVector(200.0f, 200.0f, 200.0f));
	OuterBoxComponent->SetLineThickness(1.0f);
	OuterBoxComponent->ShapeColor = FColor(255, 255, 0, 255); // Yellow for outer zone

#if WITH_EDITORONLY_DATA
	// Make it visible in editor
	bEditable = true;
	bListedInSceneOutliner = true;
#endif
	
	// Initialize default influence zone data
	InfluenceZoneData.InnerExtent = FVector(100.0f, 100.0f, 100.0f);
	InfluenceZoneData.OuterExtent = FVector(200.0f, 200.0f, 200.0f);
	InfluenceZoneData.bEnabled = true;
	InfluenceZoneData.bUseEllipsoidFalloff = false;
	
	// Default to overriding noise amplitude to 0 (no noise)
	InfluenceZoneData.bOverrideNoiseAmplitude = true;
	InfluenceZoneData.OverrideNoiseAmplitude = 0.0f;
	
	// Don't override other settings by default
	InfluenceZoneData.bOverrideNoiseFloorScale = false;
	InfluenceZoneData.OverrideNoiseFloorScale = 1.0f;
	InfluenceZoneData.bOverrideNoiseScale = false;
	InfluenceZoneData.OverrideNoiseScale = 100.0f;
	InfluenceZoneData.bOverrideNoiseScaleVector = false;
	InfluenceZoneData.OverrideNoiseScaleVector = FVector(1.0f, 1.0f, 1.0f);
}

void ADAVoxelNoiseInfluenceVolume::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	// Update the influence zone transform to match the actor transform
	InfluenceZoneData.Transform = GetActorTransform();
	
	UpdateVisualization();
}

#if WITH_EDITOR
void ADAVoxelNoiseInfluenceVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.Property)
	{
		FName PropertyName = PropertyChangedEvent.Property->GetFName();
		
		// Update visualization when properties change
		if (PropertyName == GET_MEMBER_NAME_CHECKED(FDANoiseInfluenceZone, InnerExtent) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(FDANoiseInfluenceZone, OuterExtent) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(FDANoiseInfluenceZone, bUseEllipsoidFalloff))
		{
			UpdateVisualization();
		}
	}
	
	// Always update the transform
	InfluenceZoneData.Transform = GetActorTransform();
}
#endif

void ADAVoxelNoiseInfluenceVolume::UpdateVisualization()
{
	if (InnerBoxComponent)
	{
		InnerBoxComponent->SetBoxExtent(InfluenceZoneData.InnerExtent);
		
		// Change shape color based on ellipsoid mode
		if (InfluenceZoneData.bUseEllipsoidFalloff)
		{
			InnerBoxComponent->ShapeColor = FColor(0, 128, 255, 255); // Blue for ellipsoid
		}
		else
		{
			InnerBoxComponent->ShapeColor = FColor(0, 255, 0, 255); // Green for box
		}
	}
	
	if (OuterBoxComponent)
	{
		OuterBoxComponent->SetBoxExtent(InfluenceZoneData.OuterExtent);
		
		// Change shape color based on ellipsoid mode
		if (InfluenceZoneData.bUseEllipsoidFalloff)
		{
			OuterBoxComponent->ShapeColor = FColor(128, 192, 255, 255); // Light blue for ellipsoid
		}
		else
		{
			OuterBoxComponent->ShapeColor = FColor(255, 255, 0, 255); // Yellow for box
		}
	}
}

