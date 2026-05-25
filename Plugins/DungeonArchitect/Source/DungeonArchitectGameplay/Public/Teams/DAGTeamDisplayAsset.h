//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DAGTeamDisplayAsset.generated.h"

struct FPropertyChangedEvent;

class UMaterialInstanceDynamic;
class UMeshComponent;
class UNiagaraComponent;
class AActor;
class UTexture;

// Represents the display information for team definitions (e.g., colors, display names, textures, etc...)
UCLASS(BlueprintType)
class UDAG_TeamDisplayAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DAG")
	TMap<FName, float> ScalarParameters;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DAG|Teams")
	TMap<FName, FLinearColor> ColorParameters;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DAG|Teams")
	TMap<FName, TObjectPtr<UTexture>> TextureParameters;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DAG|Teams")
	FText TeamShortName;

public:
	UFUNCTION(BlueprintCallable, Category="DAG|Teams")
	void ApplyToMaterial(UMaterialInstanceDynamic* Material);

	UFUNCTION(BlueprintCallable, Category="DAG|Teams")
	void ApplyToMeshComponent(UMeshComponent* MeshComponent);

	UFUNCTION(BlueprintCallable, Category="DAG|Teams")
	void ApplyToNiagaraComponent(UNiagaraComponent* NiagaraComponent);

	UFUNCTION(BlueprintCallable, Category="DAG|Teams", meta=(DefaultToSelf="TargetActor"))
	void ApplyToActor(AActor* TargetActor, bool bIncludeChildActors = true);

public:

	//~UObject interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~End of UObject interface
};

