//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Core/Dungeon.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SnapFunctionLibrary.generated.h"

UCLASS(Blueprintable)
class USnapDungeonFunctionLibrary : public UBlueprintFunctionLibrary {
	GENERATED_BODY()
public:
	/**
	 * @brief Builds the themed snap based dungeon.  This assumes that the dungeon is already built (so the layout is saved in the model) 
	 * @param Dungeon The dungeon actor
	 * @param World The target world to build the dungeon on. Leave it blank to build on the main world. You may generate this in a new world using CreateDungeonPreviewScene
	 * @param ParentActor The actor to attach the spawned dungeon actors to.   You can move/rotate this actor to rotate the entire dungeon.  Leave it blank to build it normally 
	 * @param ThemeId The theme you've provided in your module database.  E.g. you might register lower resolution modules under the theme id "minimap". Provide that here
	 * @param ConnectionMarkerPrefix In prefix to added to the marker name while spawning the connections.   You might specify "Minimap" here and create equivalent markers in the connection theme editor (MinimapDoor, MinimapWall to correspond with Door and Wall)  
	 * @return If the build was successful
	 */
	UFUNCTION(BlueprintCallable, Category="Dungeon Architect")
	static bool BuildSnapThemedDungeon(ADungeon* Dungeon, UWorld* World, AActor* ParentActor, FName ThemeId, const FString& ConnectionMarkerPrefix);
};
 

