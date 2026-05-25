//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DAGCraftingRecipe.generated.h"

class UDAG_ItemDefinition;

USTRUCT(BlueprintType)
struct FDAG_CraftingRecipeItem {
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	TObjectPtr<UDAG_ItemDefinition> ItemDefinition = nullptr;;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	int32 Amount = 0;
};


UCLASS(BlueprintType)
class DUNGEONARCHITECTGAMEPLAY_API UDAG_CraftingRecipe : public UDataAsset {
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
	TArray<FDAG_CraftingRecipeItem> RequiredItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
	TArray<FDAG_CraftingRecipeItem> ResultItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
	FText RecipeName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
	FText RecipeDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crafting")
	float CraftingTime = 0.0f;
};

