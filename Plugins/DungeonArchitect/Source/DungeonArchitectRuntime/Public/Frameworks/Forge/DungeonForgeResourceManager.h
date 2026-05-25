//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DungeonForgeResourceManager.generated.h"

class UTexture;

UCLASS()
class UDungeonForgeResourceManager : public UObject {
	GENERATED_BODY()
public:
	void RegisterManagedTexture(UTexture* InTexture);
	void ReleaseResources();

	template<typename TObject>
	TObject* CreateObject() {
		return NewObject<TObject>(this);
	}
	
private:
	UPROPERTY()
	TArray<TObjectPtr<UTexture>> ManagedTextures;
};

