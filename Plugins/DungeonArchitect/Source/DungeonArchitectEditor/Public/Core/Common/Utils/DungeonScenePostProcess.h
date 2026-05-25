//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonScenePostProcess.generated.h"

class APostProcessVolume;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class UDungeonScenePostProcess : public UObject {
    GENERATED_BODY()
public:
    virtual void Init(UWorld* InWorld, UMaterialInterface* InMasterMaterial);
    virtual void Destroy();
    virtual void Update(UWorld* InTargetWorld);

    TObjectPtr<UMaterialInstanceDynamic> GetPostProcessMaterial() const { return PostProcessMaterial; }
    
protected:
    virtual void UpdateResources(UWorld* InWorld);

protected:
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> PostProcessMaterial;
	
    UPROPERTY()
    TObjectPtr<APostProcessVolume> PostProcessVolume;

    UPROPERTY()
    TObjectPtr<UMaterialInterface> MasterMaterial;
    
};

