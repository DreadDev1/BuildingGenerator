//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonThemeEdModeGizmoProxy.generated.h"

UCLASS()
class ADungeonThemeEdModeGizmoProxy : public AActor {
	GENERATED_BODY()
public:
	ADungeonThemeEdModeGizmoProxy();
	
	virtual bool IsEditorOnly() const override { return true; }

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnGizmoEditChanged, bool);
	FOnGizmoEditChanged OnGizmoEditChanged;
	
#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
#endif // WITH_EDITOR
	
public:
	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot;
};

