//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "DungeonArchitectEditorUISubsystem.generated.h"

class FLayoutExtender;

UCLASS()
class UDungeonArchitectEditorUISubsystem : public UEditorSubsystem {
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
private:
	void RegisterLayoutExtensions(FLayoutExtender& Extender) const;
};

