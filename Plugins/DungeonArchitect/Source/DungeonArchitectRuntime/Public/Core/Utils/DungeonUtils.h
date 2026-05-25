//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "EngineUtils.h"

class UEdGraphNode;
class UDungeonModel;
struct FDungeonMarkerInstance;
class ADungeon;
class ULevel;

class DUNGEONARCHITECTRUNTIME_API FDungeonUtils {
public:
	template<typename TActor>
	static void DestroyManagedActor(const UWorld* InWorld, const FGuid& InDungeonId) {
		for (TActorIterator<TActor> It(InWorld); It; ++It) {
			TActor* ManagedActor = *It;
			if (ManagedActor && ManagedActor->DungeonID == InDungeonId) {
				ManagedActor->Destroy();
			}
		}
	}

	static FName GetDungeonIdTag(const ADungeon* Dungeon);
	static FName GetNodeID(const UEdGraphNode* InNode);
	
	static bool ActorBelongsToDungeon(const AActor* InActor, const ADungeon* InDungeon);

	static void DestroyAllDungeonOwnedActors(ADungeon* InDungeon);
	static void DestroyAllDungeonOwnedActors(ADungeon* InDungeon, ULevel* InLevel);
	static FVector GetPlayerViewportLocation(const UWorld* World);
	static bool GetSpawnedActorMarkerInfo(const AActor* InSpawnedActor, const UDungeonModel* InDungeonModel, FDungeonMarkerInstance& OutWorldMarker);
	
	static const FName GenericDungeonIdTag;

	template<typename F>
	void ExecuteNextTick(F&& Lambda) {
		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda([Lambda = Forward<F>(Lambda)](float) {
				Lambda();
				return false;
			})
		);
	}
	
	template <typename T>
	static void CloneUObjectArray(UObject* Outer, const TArray<TObjectPtr<T>>& SourceList, TArray<TObjectPtr<T>>& DestList) {
		DestList.Reset();
		for (T* Source : SourceList) {
			if (!Source) continue;
			TObjectPtr<T> Clone = NewObject<T>(Outer, Source->GetClass(), NAME_None, RF_NoFlags, Source);
			DestList.Add(Clone);
		}
	}
};

