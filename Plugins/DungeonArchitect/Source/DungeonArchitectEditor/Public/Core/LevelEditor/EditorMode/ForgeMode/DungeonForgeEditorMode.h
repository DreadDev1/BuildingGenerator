//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Tools/UEdMode.h"
#include "DungeonForgeEditorMode.generated.h"

class UDungeonForgePaintBrushToolBuilder;
class ADungeon;
class UDungeonForgeGraph;
class ADungeonForgeEditorDebugVisualization;
class UDungeonForgeTransformToolBuilder;
class UDungeonForgeNodeSettings;
class UModelingSceneSnappingManager;

UCLASS()
class UDungeonForgeEditorMode : public UEdMode {
	GENERATED_BODY()
	
public:
	const static FEditorModeID EM_DungeonForgeEditorModeId;
	
	static FString GridToolName;
	static FString TextureToolName;
	static FString TransformToolName;
	static FString TexturePaintToolName;
	
	UDungeonForgeEditorMode();

	/** UEdMode interface */
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void ActorSelectionChangeNotify() override;
	virtual void CreateToolkit() override;
	virtual TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetModeCommands() const override;

	ADungeon* GetDungeonActor();

	void SpawnDungeonForgeActor(const FAssetData& InForgeAssetPath);
	
	void ActiveTransformTool(UDungeonForgeNodeSettings* InNodeSettings) const;
	void ActivePaintBrushTool(UDungeonForgeNodeSettings* InNodeSettings) const;
	void BuildDungeon() const;

	void SetSelectedObject(UObject* InObject) const;
	void SetSelectedObjects(TArray<UObject*> InObjects) const;
	
private:
	void FindAndCacheDungeonForgeActor();
	TWeakObjectPtr<UDungeonForgeGraph> GetForgeGraph() const; 

	UFUNCTION()
	void HandleDungeonBuildComplete(ADungeon* InDungeon, bool bInSuccess);
	
private:
	TWeakObjectPtr<ADungeon> Dungeon;

	// Tool Builders
	UPROPERTY()
	TObjectPtr<UDungeonForgeTransformToolBuilder> TransformToolBuilder;
	
	UPROPERTY()
	TObjectPtr<UDungeonForgePaintBrushToolBuilder> PaintBrushToolBuilder;

	UPROPERTY()
	TObjectPtr<UModelingSceneSnappingManager> SceneSnappingManager;
	
	friend class FDungeonForgeEditorModeToolkit;
};

