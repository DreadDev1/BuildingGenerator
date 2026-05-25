//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Layout/DungeonLayoutData.h"

#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Actor.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DungeonBPFunctionLibrary.generated.h"

class UCanvasRenderTarget2D;
class AActor;
class ADungeon;

UCLASS()
class DUNGEONARCHITECTRUNTIME_API UDungeonBPFunctionLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
	
    UFUNCTION(BlueprintCallable, Category = "Dungeon")
    static AActor* SpawnDungeonOwnedActor(ADungeon* Dungeon, TSubclassOf<AActor> ActorClass, const FTransform& Transform);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dungeon")
    static bool ActorBelongsToDungeon(ADungeon* Dungeon, AActor* ActorToCheck);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dungeon")
	static TArray<AActor*> GetAllDungeonSpawnedActors(ADungeon* Dungeon);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dungeon")
	static bool DungeonObjectHasAuthority(UObject* Object);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dungeon", meta=(DeterminesOutputType="ActorClass", DynamicOutputParam="OutActors"))
	static void DAGetAllActorsOfClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors);

	UFUNCTION(BlueprintCallable, Category="Dungeon", meta=(DeterminesOutputType="ActorClass"))
	static AActor* DAGetActorOfClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass);

	UFUNCTION(BlueprintCallable, Category="Dungeon")
	static void DARecreateActorInLevel(AActor* InActor, ULevel* TargetLevel, AActor*& NewTargetActor);

	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dungeon")
	static int32 DAHashLocation(const FVector& Location);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dungeon")
	static int32 DAHashTransform(const FTransform& Transform);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Dungeon")
	static int32 DAHashCombine(int32 A, int32 B);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonCanvas")
	static FDungeonLayoutData GetDungeonLayout(ADungeon* Dungeon);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonCanvas")
	static FBox GetDungeonBounds(ADungeon* Dungeon);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonCanvas")
	static FBox GenerateDungeonLayoutBounds(const FDungeonLayoutData& DungeonLayout, float Margin = 1000);

	UFUNCTION(BlueprintCallable, Category = "DungeonCanvas", meta=(DefaultToSelf="Owner"))
	static UCanvasRenderTarget2D* DACreateCanvasRenderTexture(UObject* Owner, TEnumAsByte<ETextureRenderTargetFormat> Format, int Width, int Height, FLinearColor ClearColor);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonCanvas", meta=(DefaultToSelf="Owner"))
	static bool TextureNeedsRecreation(UTextureRenderTarget2D* Texture, TEnumAsByte<ETextureRenderTargetFormat> Format, int Width, int Height);
	
};

