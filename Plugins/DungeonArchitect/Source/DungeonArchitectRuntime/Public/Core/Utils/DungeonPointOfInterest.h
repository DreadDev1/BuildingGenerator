//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "DungeonPointOfInterest.generated.h"

class UBillboardComponent;

USTRUCT()
struct DUNGEONARCHITECTRUNTIME_API FDungeonPointOfInterest {
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category="Point of Interest")
	FName Id;
    
	UPROPERTY(VisibleAnywhere, Category="Point of Interest")
	FName Caption;
    
	UPROPERTY(VisibleAnywhere, Category="Point of Interest")
	FTransform Transform;
};

UCLASS(HideDropdown, hidecategories=(Physics, Lighting, LOD, Rendering, TextureStreaming, Transform, Activation, "Components|Activation"))
class DUNGEONARCHITECTRUNTIME_API UDungeonPointOfInterestComponent : public UPrimitiveComponent {
	GENERATED_BODY()
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Point of Interest")
	FName Id;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Point of Interest")
	FName Caption;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	bool bVisibleInCanvas = {};

public:
	FDungeonPointOfInterest GetPointOfInterest() const;
};


UCLASS(Blueprintable, ConversionRoot, ComponentWrapperClass)
class DUNGEONARCHITECTRUNTIME_API ADungeonPointOfInterestActor : public AActor {
	GENERATED_UCLASS_BODY()
public:
	UPROPERTY(VisibleAnywhere, Category = "Dungeon")
	TObjectPtr<UDungeonPointOfInterestComponent> PointOfInterestComponent;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Billboard;
#endif // WITH_EDITORONLY_DATA
};

