//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/MiniMap/LevelMinimapGenerator.h"

#include "Components/StaticMeshComponent.h"
#include "EditorAssetLibrary.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GeometryScript/CreateNewAssetUtilityFunctions.h"
#include "GeometryScript/MeshAssetFunctions.h"
#include "GeometryScript/MeshBasicEditFunctions.h"
#include "GeometryScript/MeshNormalsFunctions.h"
#include "GeometryScript/MeshPolygroupFunctions.h"
#include "GeometryScript/MeshSimplifyFunctions.h"
#include "GeometryScript/MeshVoxelFunctions.h"
#include "Interfaces/IMainFrameModule.h"
#include "PropertyEditorModule.h"
#include "UDynamicMesh.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "LevelMinimapGenSettingsWindow"

void SLevelMinimapGenSettingsWindow::Construct(const FArguments& InArgs) {
	
	Settings = InArgs._Settings;
	WidgetWindow = InArgs._WidgetWindow;

	check (Settings);
	
	TSharedPtr<SBox> InspectorBox;
	this->ChildSlot
	[
		SNew(SBox)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2.0f)
			[
				SNew(SBorder)
				.Padding(FMargin(3))
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(STextBlock)
						.Font(FAppStyle::GetFontStyle("CurveEd.LabelFont"))
						.Text(LOCTEXT("Export_CurrentFileTitle", "Current File: "))
					]
					+SHorizontalBox::Slot()
					.Padding(5, 0, 0, 0)
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Font(FAppStyle::GetFontStyle("CurveEd.InfoFont"))
						.Text(InArgs._FullPath)
					]
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			[
				SAssignNew(InspectorBox, SBox)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(2.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("DAMiniMapGen_Generate", "Generate"))
					.ToolTipText(LOCTEXT("DAMiniMapGen_Generate_ToolTip", "Generate the minimap mesh"))
					.OnClicked(this, &SLevelMinimapGenSettingsWindow::OnGenerate)
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("DAMiniMapGen_Cancel", "Cancel"))
					.OnClicked(this, &SLevelMinimapGenSettingsWindow::OnCancel)
				]
			]
		]
	];

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	InspectorBox->SetContent(DetailsView->AsShared());
	DetailsView->SetObject(Settings);
}

void ULevelMinimapGenerator::GenerateMinimapMeshFromWorld(UWorld* InWorld) {
	ULevel* Level = InWorld ? InWorld->PersistentLevel : nullptr;
	if (!Level) {
		return;
	}

	ULevelMinimapGeneratorSettings* MeshGenSettings = Level->GetAssetUserData<ULevelMinimapGeneratorSettings>();
	if (!MeshGenSettings) {
		MeshGenSettings = NewObject<ULevelMinimapGeneratorSettings>(Level);
		Level->AddAssetUserData(MeshGenSettings);
	}
	
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("MinimapOpionsTitle", "Dungeon Architect Minimap Mesh"))
		.SizingRule(ESizingRule::UserSized)
		.AutoCenter(EAutoCenter::PrimaryWorkArea)
		.ClientSize(FVector2D(500, 445));
	
	FString FullPathWithExtension = UEditorAssetLibrary::GetPathNameForLoadedAsset(InWorld);
	TSharedPtr<SLevelMinimapGenSettingsWindow> MeshGetOptionWidget;
	Window->SetContent
		(
			SAssignNew(MeshGetOptionWidget, SLevelMinimapGenSettingsWindow)
			.Settings(MeshGenSettings)
			.WidgetWindow(Window)
			.FullPath(FText::FromString(FullPathWithExtension))
		);
	
	TSharedPtr<SWindow> ParentWindow;
	if (FModuleManager::Get().IsModuleLoaded("MainFrame")) {
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}
	FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);
	if (!MeshGetOptionWidget->ShouldExport()) {
		return;
	}
		
	UDynamicMeshPool* MeshPool = NewObject<UDynamicMeshPool>();
	TMap<UStaticMesh*, TArray<FTransform>> MeshTransforms;

	static const FName IgnoreTag = "ignore_minimap"; 

	for (const AActor* Actor : Level->Actors) {
		if (Actor->Tags.Contains(IgnoreTag)) {
			continue;
		}

		TArray<UStaticMeshComponent*> MeshComponents;
		if (const AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(Actor)) {
			MeshComponents = { StaticMeshActor->GetStaticMeshComponent() };
		}
		else {
			constexpr bool bIncludeFromChildActors = false; 	// Check if child actors are needed
			Actor->GetComponents(MeshComponents, bIncludeFromChildActors);
		}

		for (UStaticMeshComponent* MeshComponent : MeshComponents) {
			UStaticMesh* StaticMesh = MeshComponent->GetStaticMesh();
			const FTransform MeshTransform = MeshComponent->GetComponentTransform();

			// Make sure we don't include very large objects (like skyboxes)
			{
				FVector MeshExtents = StaticMesh->GetBounds().BoxExtent;
				FVector MeshWorldSize = MeshExtents * 2 * MeshTransform.GetScale3D();
				static constexpr float KM = 1024 * 100; 
				static constexpr float MaxAllowedSize = 0.5f * KM;
				if (FMath::Max3(MeshWorldSize.X, MeshWorldSize.Y, MeshWorldSize.Z) > MaxAllowedSize) {
					continue;
				}
			}
			
			TArray<FTransform>& Transforms = MeshTransforms.FindOrAdd(StaticMesh);
			Transforms.Add(MeshTransform);
		}
	}
	
	UDynamicMesh* MinimapMesh = MeshPool->RequestMesh();
	for (const auto& Entry : MeshTransforms) {
		UStaticMesh* StaticMesh = Entry.Key;
		const TArray<FTransform>& Transforms = Entry.Value;

		// Generate a simplified version of this static mesh
		UDynamicMesh* SimplifiedMesh = MeshPool->RequestMesh(); 
		{
			// Copy over the static mesh to the dynamic mesh, so we can voxelize it in the next step
			constexpr FGeometryScriptCopyMeshFromAssetOptions AssetOptions;
			EGeometryScriptOutcomePins Outcome;
			FGeometryScriptMeshReadLOD MeshCopyOptions;
			MeshCopyOptions.LODIndex = MeshGenSettings->LODIndex;
			SimplifiedMesh = UGeometryScriptLibrary_StaticMeshFunctions::CopyMeshFromStaticMesh(StaticMesh, SimplifiedMesh, AssetOptions, MeshCopyOptions, Outcome);

			// Voxelize the mesh
			//FGeometryScriptSolidifyOptions VoxelizeOptions;
			//VoxelizeOptions.GridParameters.SizeMethod = EGeometryScriptGridSizingMethod::GridCellSize;
			//VoxelizeOptions.GridParameters.GridCellSize = 30;
			//SimplifiedMesh = UGeometryScriptLibrary_MeshVoxelFunctions::ApplyMeshSolidify(SimplifiedMesh, VoxelizeOptions);
		}

		// Copy over this mesh to the minimap mesh in all the places it occurs in the world
		MinimapMesh = UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMeshTransformed(MinimapMesh, SimplifiedMesh, Transforms, FTransform::Identity);
		
		MeshPool->ReturnMesh(SimplifiedMesh);
	}

	// Voxelize the minimap
	{
		if (MeshGenSettings->bVoxelize) {
			MinimapMesh = UGeometryScriptLibrary_MeshVoxelFunctions::ApplyMeshSolidify(MinimapMesh, MeshGenSettings->VoxelizeOptions);
		}

		MinimapMesh = UGeometryScriptLibrary_MeshPolygroupFunctions::EnablePolygroups(MinimapMesh);
		MinimapMesh = UGeometryScriptLibrary_MeshPolygroupFunctions::ComputePolygroupsFromAngleThreshold(MinimapMesh, {});
		MinimapMesh = UGeometryScriptLibrary_MeshSimplifyFunctions::ApplySimplifyToTriangleCount(MinimapMesh, MeshGenSettings->TargetTriangleCount, MeshGenSettings->SimplifyOptions);
		MinimapMesh = UGeometryScriptLibrary_MeshNormalsFunctions::RecomputeNormals(MinimapMesh, MeshGenSettings->NormalGenOptions);
	}
	
	// Save the minimap to disk
	{
		FString SourceAssetPath = FPaths::GetBaseFilename(FullPathWithExtension, false);
		FString SourceAssetFolder = FPaths::GetPath(SourceAssetPath);
		FString SourceAssetName = FPaths::GetBaseFilename(FullPathWithExtension, true);

		FString MinimapMeshPath = SourceAssetFolder / (SourceAssetName + MeshGenSettings->MinimapMeshPostfix);
		FGeometryScriptCreateNewStaticMeshAssetOptions CreateMeshOptions;
		CreateMeshOptions.bEnableRecomputeNormals = false;
		CreateMeshOptions.bEnableRecomputeTangents = false;
		CreateMeshOptions.bEnableCollision = false;
		CreateMeshOptions.bEnableNanite = MeshGenSettings->bEnableNanite;
		
		EGeometryScriptOutcomePins Outcome;
		UGeometryScriptLibrary_CreateNewAssetFunctions::CreateNewStaticMeshAssetFromMesh(MinimapMesh, MinimapMeshPath, CreateMeshOptions,Outcome);
	}
	
	MeshPool->ReturnAllMeshes();
}



#undef LOCTEXT_NAMESPACE

