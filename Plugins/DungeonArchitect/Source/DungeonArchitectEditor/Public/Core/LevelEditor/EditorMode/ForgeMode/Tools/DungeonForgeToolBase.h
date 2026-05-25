//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "InteractiveTool.h"
#include "InteractiveToolBuilder.h"
#include "DungeonForgeToolBase.generated.h"

class UDungeonForgeEditorMode;

UCLASS(Transient)
class DUNGEONARCHITECTEDITOR_API UDungeonForgeToolBaseProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
};


UCLASS(Abstract)
class DUNGEONARCHITECTEDITOR_API UDungeonForgeToolBase : public UInteractiveTool
{
	GENERATED_BODY()

	
public:
	virtual void SetWorld(UWorld* World);
	
	template<typename TProperties>
	TObjectPtr<TProperties> SetupToolProperties() {
		TProperties* Properties = NewObject<TProperties>(this);
		AddToolPropertySource(Properties);
		return Properties;
	}

	void SetEdMode(const TWeakObjectPtr<UDungeonForgeEditorMode>& InEdMode);
	UDungeonForgeEditorMode* GetForgeEditorMode() const { return EdMode.Get(); }
	
protected:
	TWeakObjectPtr<UWorld> TargetWorld;
	TWeakObjectPtr<UDungeonForgeEditorMode> EdMode;
};


UCLASS()
class UDungeonForgeToolBuilderBase : public UInteractiveToolBuilder {
	GENERATED_BODY()
public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;

	void SetEdMode(const TWeakObjectPtr<UDungeonForgeEditorMode>& InEdMode);

protected:
	TSubclassOf<UDungeonForgeToolBase> ToolClass;
	TWeakObjectPtr<UDungeonForgeEditorMode> EdMode;
};