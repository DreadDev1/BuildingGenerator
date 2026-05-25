//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Components/LineBatchComponent.h"
#include "DungeonDebugVisualizer.generated.h"

UCLASS() 
class DUNGEONARCHITECTRUNTIME_API UDungeonDebugVisualizer : public ULineBatchComponent {
	GENERATED_BODY()
public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void Clear();
	void DrawTexturedQuad(const FTransform& InTransform, UTexture* InTexture, UMaterialInterface* InRenderMaterial = nullptr);
	
private:
	UPROPERTY()
	TArray<TObjectPtr<AActor>> DebugActors;
};

