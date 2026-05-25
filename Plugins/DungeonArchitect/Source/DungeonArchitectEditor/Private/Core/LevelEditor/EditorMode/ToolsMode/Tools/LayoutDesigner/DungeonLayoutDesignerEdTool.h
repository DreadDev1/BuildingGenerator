//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/LevelEditor/EditorMode/ToolsMode/Tools/DungeonEdTool.h"
#include "DungeonLayoutDesignerEdTool.generated.h"

UCLASS()
class DUNGEONARCHITECTEDITOR_API UDungeonLayoutDesignerEdToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
	
public:
	UPROPERTY()
	TSubclassOf<UDungeonLayoutDesignerEdToolBase> ToolClass;
};


UCLASS(Transient)
class UDungeonLayoutDesignerEdToolPropertiesBase : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:

};

UCLASS()
class UDungeonLayoutDesignerEdToolBase : public UInteractiveTool {
	GENERATED_BODY()
	
public:
	virtual void Setup() override;
	void SetWorld(UWorld* InWorld);

protected:
	UPROPERTY()
	TSubclassOf<UDungeonLayoutDesignerEdToolPropertiesBase> PropertyClass;
	
	UPROPERTY()
	TObjectPtr<UDungeonLayoutDesignerEdToolPropertiesBase> Properties = nullptr;

	TWeakObjectPtr<UWorld> TargetWorld{};
};


////////////////////////// UDungeonLayoutDesignerEdTool //////////////////////////
UCLASS(Transient, DisplayName="Layout Designer")
class UDungeonLayoutDesignerEdToolProperties : public UDungeonLayoutDesignerEdToolPropertiesBase
{
	GENERATED_BODY()
public:
	UFUNCTION(CallInEditor, Category="Actions")
	void CreateNewSegment();
};

UCLASS()
class UDungeonLayoutDesignerEdTool : public UDungeonLayoutDesignerEdToolBase {
	GENERATED_BODY()
public:
	UDungeonLayoutDesignerEdTool();

};

