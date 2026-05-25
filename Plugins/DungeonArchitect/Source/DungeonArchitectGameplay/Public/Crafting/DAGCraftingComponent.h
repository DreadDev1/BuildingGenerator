//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DAGCraftingComponent.generated.h"

class UDAG_CraftingRecipe;
class UDAG_InventoryComponent;
struct FDAG_CraftingRecipeItem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDAG_CraftingProgressDelegate, float, Progress, float, TotalTime);

UENUM(BlueprintType)
enum class EDAG_CraftingSystemState : uint8 {
	/** Not crafting */
	NotCrafting,
	
	/** Crafting is in progress */
	InProgress,

	/** Crafting is paused */
	Paused
};

UENUM(BlueprintType)
enum class EDAG_CraftingSystemPauseReason : uint8 {
	None,
	
	/** Output inventory has no space to put the crafted result */
	WaitTillOutputFree,

	/** Not enough input resources */
	WaitTillInputAvailable,
	
	/** Manually invoked by the user or gameplay code */
	Manual
};

UCLASS(ClassGroup=(Crafting), meta=(BlueprintSpawnableComponent))
class DUNGEONARCHITECTGAMEPLAY_API UDAG_CraftingSystemComponent : public UActorComponent {
	GENERATED_BODY()

public:
	UDAG_CraftingSystemComponent();

	/** Starts crafting the recepie if the inventory has the necessary recipe input items. The items will be consumed */
	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Crafting")
	void StartCrafting(UDAG_CraftingRecipe* InRecipe, UDAG_InventoryComponent* InInputInventory);

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool CanCraft(UDAG_CraftingRecipe* InRecipe, UDAG_InventoryComponent* InInputInventory) const;
	
	/** Cancels the crafting. The caller needs to take care of refunding the used input assets back to the owner */
	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void CancelCrafting();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Crafting")
	EDAG_CraftingSystemState GetCraftingState() const { return CraftingState; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Crafting")
	bool IsInState(EDAG_CraftingSystemState State) const { return CraftingState == State; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Crafting")
	EDAG_CraftingSystemPauseReason GetCraftingPauseReason() const;

	UPROPERTY(BlueprintAssignable, Category = "Crafting")
	FDAG_CraftingProgressDelegate OnCraftingProgress;

public:
	UPROPERTY(Replicated)
	bool bAutoCraftWhenInputReady = false;

	
protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	bool HasAuthority() const;
	void SetCraftingState(EDAG_CraftingSystemState InState);
	void ServerCompleteCrafting();

	static bool CanAddItemsToInventory(const TArray<FDAG_CraftingRecipeItem>& InItems, UDAG_InventoryComponent* InInventory);
	void NotifyCraftingProgress() const;
	
private:
	UFUNCTION()
	void OnRep_CraftingProgress();

	UPROPERTY()
	TObjectPtr<UDAG_CraftingRecipe> CurrentRecipe = nullptr;;

private:
	/** The inventory to grab the input from */
	UPROPERTY()
	TWeakObjectPtr<UDAG_InventoryComponent> InputInventory = nullptr;

	/** The target inventory to place the target on */
	UPROPERTY()
	TWeakObjectPtr<UDAG_InventoryComponent> OutputInventory = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_CraftingProgress)
	float CraftingProgress;

	UPROPERTY(Replicated)
	EDAG_CraftingSystemState CraftingState;

	UPROPERTY(Replicated)
	EDAG_CraftingSystemPauseReason PauseReason = EDAG_CraftingSystemPauseReason::None;
	
	UPROPERTY(Replicated)
	float TotalCraftingTime;
};

