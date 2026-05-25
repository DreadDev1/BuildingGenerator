//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Volumes/DAVoxelVolume.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"

ADAVoxelVolume::ADAVoxelVolume() {
	PrimaryActorTick.bCanEverTick = false;
	
#if WITH_EDITORONLY_DATA
	VolumeComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("VolumeComponent"));
	RootComponent = VolumeComponent;
	VolumeComponent->SetBoxExtent(BoxExtent);
	VolumeComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VolumeComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	VolumeComponent->SetLineThickness(2.0f);
	
	if (bSubtract) {
		VolumeComponent->ShapeColor = FColor(100, 100, 255, 255);  // Blue for subtracting
	} else {
		VolumeComponent->ShapeColor = FColor(255, 100, 100, 255);  // Red for adding
	}
#endif
}

void ADAVoxelVolume::OnConstruction(const FTransform& Transform) {
	Super::OnConstruction(Transform);
	InitializeVoxelVolume();
}

void ADAVoxelVolume::PostInitializeComponents() {
	Super::PostInitializeComponents();
	InitializeVoxelVolume();
}

void ADAVoxelVolume::BeginPlay() {
	Super::BeginPlay();
}

void ADAVoxelVolume::InitializeVoxelVolume() const {
#if WITH_EDITORONLY_DATA
	if (VolumeComponent) {
		VolumeComponent->SetBoxExtent(BoxExtent);
		if (bSubtract) {
			VolumeComponent->ShapeColor = FColor(100, 100, 255, 255);  // Blue for subtracting
		} else {
			VolumeComponent->ShapeColor = FColor(255, 100, 100, 255);  // Red for adding
		}
	}
#endif // WITH_EDITORONLY_DATA
}

FDAVoxelShape ADAVoxelVolume::CreateVoxelShape() const {
	FDAVoxelShape Shape;
	Shape.Type = EDAVoxelShapeType::ConvexPolygon;
	
	// Get the actor transform (rotation happens around center)
	FTransform ShapeTransform = GetActorTransform();
	FVector Scale = ShapeTransform.GetScale3D();
	
	// We need to offset the shape down by half height in LOCAL space
	// This way rotation happens around the center, then we offset
	// Apply the offset in world space after rotation
	FVector LocalOffset = FVector(0, 0, -BoxExtent.Z * Scale.Z);
	FVector WorldOffset = ShapeTransform.GetRotation().RotateVector(LocalOffset);
	FVector Location = ShapeTransform.GetLocation() + WorldOffset;
	ShapeTransform.SetLocation(Location);
	Shape.Transform = ShapeTransform;
	
	// Height and polygon points are defined in local space
	// The transform will handle the scaling, so we don't multiply by scale here
	Shape.Height = BoxExtent.Z * 2.0f;
	// Set operation type
	Shape.Operation = bSubtract ? EDAVoxelOperation::Subtract : EDAVoxelOperation::Add;
	Shape.Layer = EVoxelShapeLayer::AddGeometry;
	Shape.MaterialID = MaterialID;
	Shape.Priority = Priority;
	Shape.bOverrideNoiseSettings = bOverrideNoiseSettings;
	Shape.NoiseSettingsOverride = NoiseSettingsOverride;
	
	// Polygon points are also in local space - no scale multiplication needed
	const float HalfX = BoxExtent.X;
	const float HalfY = BoxExtent.Y;
	Shape.PolygonPoints.Add(FVector2D(-HalfX, -HalfY));
	Shape.PolygonPoints.Add(FVector2D(-HalfX, HalfY));
	Shape.PolygonPoints.Add(FVector2D(HalfX, HalfY));
	Shape.PolygonPoints.Add(FVector2D(HalfX, -HalfY));
	
	return Shape;
}

#if WITH_EDITOR
void ADAVoxelVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.Property) {
		const FName PropertyName = PropertyChangedEvent.Property->GetFName();
		
		if (PropertyName == GET_MEMBER_NAME_CHECKED(ADAVoxelVolume, BoxExtent)) {
			InitializeVoxelVolume();
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(ADAVoxelVolume, bSubtract)) {
			InitializeVoxelVolume();
		}
	}
}
#endif

