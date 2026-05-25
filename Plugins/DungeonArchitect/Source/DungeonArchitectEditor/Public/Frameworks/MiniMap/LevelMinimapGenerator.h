//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "GeometryScript/MeshNormalsFunctions.h"
#include "GeometryScript/MeshSimplifyFunctions.h"
#include "GeometryScript/MeshVoxelFunctions.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"
#include "LevelMinimapGenerator.generated.h"

UCLASS(Blueprintable)
class ULevelMinimapGeneratorSettings : public UAssetUserData {
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	FString MinimapMeshPostfix = "_MinimapMesh";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	bool bEnableNanite{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	bool bVoxelize = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect", meta=(EditCondition="bVoxelize"))
	FGeometryScriptSolidifyOptions VoxelizeOptions;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	FGeometryScriptSimplifyMeshOptions SimplifyOptions{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	FGeometryScriptCalculateNormalsOptions NormalGenOptions;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	int32 LODIndex{10};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Architect")
	int32 TargetTriangleCount = 2000;
};


class SLevelMinimapGenSettingsWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLevelMinimapGenSettingsWindow)
		: _Settings(nullptr)
		, _WidgetWindow()
		, _FullPath()
	{}

	SLATE_ARGUMENT( ULevelMinimapGeneratorSettings*, Settings )
	SLATE_ARGUMENT( TSharedPtr<SWindow>, WidgetWindow )
	SLATE_ARGUMENT( FText, FullPath )
SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	virtual bool SupportsKeyboardFocus() const override { return true; }

	SLevelMinimapGenSettingsWindow()
		: Settings(nullptr)
		, bShouldExport(false)
	{}
	
	FReply OnGenerate() {
		bShouldExport = true;
		if (WidgetWindow.IsValid()) {
			WidgetWindow.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}


	FReply OnCancel() {
		bShouldExport = false;
		if ( WidgetWindow.IsValid() )
		{
			WidgetWindow.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override {
		if (InKeyEvent.GetKey() == EKeys::Escape) {
			return OnCancel();
		}

		return FReply::Unhandled();
	}

	bool ShouldExport() const {
		return bShouldExport;
	}

private:
	ULevelMinimapGeneratorSettings* Settings;
	TSharedPtr<class IDetailsView> DetailsView;
	TWeakPtr<SWindow> WidgetWindow;
	bool bShouldExport;
};


UCLASS()
class ULevelMinimapGenerator : public UBlueprintFunctionLibrary {
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	static void GenerateMinimapMeshFromWorld(UWorld* InWorld);
	
};

