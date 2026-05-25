//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/LevelStreaming/DungeonLevelStreamer.h"

#include "Frameworks/LevelStreaming/DungeonLevelStreamingModel.h"
#include "Frameworks/LevelStreaming/DungeonLevelStreamingNavigation.h"

#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY(LogLevelStreamer);

struct FDungeonLevelStreamingStateRequest {
    TWeakObjectPtr<UDungeonStreamingChunk> Chunk;
    bool bRequestVisible{};
};

void FDungeonLevelStreamer::GetPlayerLocations(UWorld* World, TArray<FVector>& OutPlayerLocations) {
    OutPlayerLocations.Reset();
    for (TActorIterator<APlayerController> It(World); It; ++It) {
        APlayerController* PlayerController = *It;
        if (PlayerController) {
            APawn* Pawn = PlayerController->GetPawnOrSpectator();
            if (Pawn) {
                FVector PlayerLocation = Pawn->GetActorLocation();
                OutPlayerLocations.Add(PlayerLocation);
            }
        }
    }
}

void FDungeonLevelStreamer::Process(UWorld* InWorld, const FDungeonLevelStreamingConfig& Config, UDungeonLevelStreamingModel* InModel) {
    TArray<UDungeonStreamingChunk*> Chunks = InModel->GetChunks();
    if (!InModel || Chunks.Num() == 0) return;
    InModel->StreamingNavigation->bEnabled = Config.bProcessStreamingNavigation;
    Chunks.RemoveAll([](const UDungeonStreamingChunk* Chunk) { return Chunk == nullptr; });

    TArray<FDungeonLevelStreamingStateRequest> UpdateRequests;
    TArray<UDungeonStreamingChunk*> ActiveChunks;

    const bool bEnabledLevelStreaming = (InWorld && InWorld->IsGameWorld()) ? Config.bEnabledLevelStreaming : false; 
    
    if (bEnabledLevelStreaming) {
        // Start by finding the source streaming points
        TArray<FVector> SourceLocations;
        EDungeonLevelStreamChunkSelection ChunkSelection = Config.InitialLoadLocation;
        if (InModel->HasNotifiedInitialChunkLoadEvent()) {
            // Default to streaming in the chunks closest to the player location if the initial chunks have already been loaded
            ChunkSelection = EDungeonLevelStreamChunkSelection::PlayerLocations;
        }
        InModel->GetStreamingSourceLocations(InWorld, ChunkSelection, SourceLocations);

        // Find the active chunks (near the source streaming points)
        {
            // Find the chunks that contain the source streaming points 
            TArray<TWeakObjectPtr<UDungeonStreamingChunk>> HostChunks;
            for (const FVector& SourceLocation : SourceLocations) {
                UDungeonStreamingChunk* StartChunk = nullptr;
                if (!GetNearestChunk(Chunks, SourceLocation, StartChunk)) {
                    continue;
                }
                HostChunks.AddUnique(StartChunk);
            }

            // Find the active chunks to stream (they usually are around the source host chunks)
            for (TWeakObjectPtr<UDungeonStreamingChunk> HostChunk : HostChunks) {
                GetVisibleChunks(Config, HostChunk.Get(), Chunks, ActiveChunks);
            }

            // Always include resident chunks - they should never be unloaded
            for (UDungeonStreamingChunk* Chunk : Chunks) {
                if (Chunk && Chunk->bAlwaysResident) {
                    ActiveChunks.AddUnique(Chunk);
                }
            }
        }

        for (UDungeonStreamingChunk* Chunk : Chunks) {
            const bool bShouldBeVisible = ActiveChunks.Contains(Chunk);
            if (Chunk->RequiresStateUpdate(bShouldBeVisible)) {
                FDungeonLevelStreamingStateRequest Request;
                Request.Chunk = Chunk;
                Request.bRequestVisible = bShouldBeVisible;
                UpdateRequests.Add(Request);
            }
        }
        
        // Sort the update requests based on the view location, so the nearby chunks are loaded first
        UpdateRequests.Sort([SourceLocations](const FDungeonLevelStreamingStateRequest& A,
                                            const FDungeonLevelStreamingStateRequest& B) -> bool {
            float BestDistA = MAX_flt;
            float BestDistB = MAX_flt;
            for (const FVector& ViewLocation : SourceLocations) {
                const float DistA = A.Chunk->Bounds.ComputeSquaredDistanceToPoint(ViewLocation);
                const float DistB = B.Chunk->Bounds.ComputeSquaredDistanceToPoint(ViewLocation);
                BestDistA = FMath::Min(BestDistA, DistA);
                BestDistB = FMath::Min(BestDistB, DistB);
            }
            return BestDistA < BestDistB;
        });
    }
    else {
        ActiveChunks.Append(Chunks);
        for (UDungeonStreamingChunk* Chunk : Chunks) {
            if (Chunk->RequiresStateUpdate(true)) {
                FDungeonLevelStreamingStateRequest Request;
                Request.Chunk = Chunk;
                Request.bRequestVisible = true;
                UpdateRequests.Add(Request);
            }
        }
    }
    
    // Process the update requests (show/hide chunks)
    for (const FDungeonLevelStreamingStateRequest& Request : UpdateRequests) {
        Request.Chunk->SetStreamingLevelState(Request.bRequestVisible, Config.UnloadMethod);
    }
    
    // Invoke the initial chunks loaded notification, if all the active nodes have been updated for the first time
    if (!InModel->HasNotifiedInitialChunkLoadEvent() && ActiveChunks.Num() > 0) {
        if (AreChunksVisible(ActiveChunks)) {
            InModel->NotifyInitialChunksLoaded();
        }
    }
}

bool FDungeonLevelStreamer::AreChunksVisible(const TArray<UDungeonStreamingChunk*>& InChunks) {
    for (UDungeonStreamingChunk* const Chunk : InChunks) {
        if (!Chunk->IsLevelVisible()) {
            return false;
        }
    }
    return true;
}

/////////////// Level Streamer Visibility Strategy ///////////////
class DUNGEONARCHITECTRUNTIME_API FDAStreamerDepthVisibilityStrategy : public IDungeonLevelStreamerVisibilityStrategy {
public:
    FDAStreamerDepthVisibilityStrategy(int32 InVisibilityRoomDepth) : VisibilityRoomDepth(InVisibilityRoomDepth) {} 
    virtual void GetVisibleChunks(UDungeonStreamingChunk* StartChunk, const TArray<UDungeonStreamingChunk*>& AllChunks, TArray<UDungeonStreamingChunk*>& OutVisibleChunks) const override {
        TSet<UDungeonStreamingChunk*> Visited;
        GetVisibleChunksRecursive(StartChunk, 0, Visited, OutVisibleChunks);
    }

private:
    void GetVisibleChunksRecursive(UDungeonStreamingChunk* Chunk, int32 DepthFromStart,
                               TSet<UDungeonStreamingChunk*>& Visited,
                               TArray<UDungeonStreamingChunk*>& OutVisibleChunks) const {
        if (!Chunk) return;

        Visited.Add(Chunk);

        OutVisibleChunks.Add(Chunk);

        if (DepthFromStart + 1 <= VisibilityRoomDepth) {
            // Traverse the children
            for (UDungeonStreamingChunk* ChildChunk : Chunk->Neighbors) {
                if (!Visited.Contains(ChildChunk)) {
                    GetVisibleChunksRecursive(ChildChunk, DepthFromStart + 1, Visited, OutVisibleChunks);
                }
            }
        }
    }

private:
    int32 VisibilityRoomDepth;
};

class DUNGEONARCHITECTRUNTIME_API FDAStreamerDistanceVisibilityStrategy : public IDungeonLevelStreamerVisibilityStrategy {
public:
    FDAStreamerDistanceVisibilityStrategy(const FVector& InVisibilityDepth) : VisibilityDepth(InVisibilityDepth) {}
    virtual void GetVisibleChunks(UDungeonStreamingChunk* StartChunk, const TArray<UDungeonStreamingChunk*>& AllChunks, TArray<UDungeonStreamingChunk*>& OutVisibleChunks) const override {
        if (!StartChunk) return;
        
        const FVector Center = StartChunk->Bounds.GetCenter();
        FVector Extent = StartChunk->Bounds.GetExtent();
        Extent += VisibilityDepth;

        const FBox VisibilityBounds(Center - Extent, Center + Extent);
        for (UDungeonStreamingChunk* Chunk : AllChunks) {
            if (!Chunk) continue;
            if (Chunk->Bounds.Intersect(VisibilityBounds)) {
                OutVisibleChunks.Add(Chunk);
            }
        }
    }
    
private:
    FVector VisibilityDepth;
};

void FDungeonLevelStreamer::GetVisibleChunks(const FDungeonLevelStreamingConfig& Config, UDungeonStreamingChunk* StartChunk,
            const TArray<UDungeonStreamingChunk*>& AllChunks, TArray<UDungeonStreamingChunk*>& OutVisibleChunks) {
    TSharedPtr<IDungeonLevelStreamerVisibilityStrategy> Strategy;
    if (Config.StreamingStrategy == EDungeonLevelStreamingStrategy::LayoutDepth) {
        Strategy = MakeShareable(new FDAStreamerDepthVisibilityStrategy(Config.VisibilityRoomDepth));
    }
    else if (Config.StreamingStrategy == EDungeonLevelStreamingStrategy::Distance) {
        Strategy = MakeShareable(new FDAStreamerDistanceVisibilityStrategy(Config.VisibilityDistance));
    }

    Strategy->GetVisibleChunks(StartChunk, AllChunks, OutVisibleChunks);
}

bool FDungeonLevelStreamer::GetNearestChunk(const TArray<UDungeonStreamingChunk*>& Chunks, const FVector& ViewLocation,
                                            UDungeonStreamingChunk*& OutNearestChunk) {
    float NearestDistanceSq = MAX_flt;

    const FVector PlayerExtent(-40, -40, 90);
    FBox PlayerBounds(ViewLocation - PlayerExtent, ViewLocation + PlayerExtent);
    
    for (UDungeonStreamingChunk* Chunk : Chunks) {
        if (FDAShapeCollision::Intersects(PlayerBounds, Chunk->BoundShapes, 1e-4f)) {
            OutNearestChunk = Chunk;
            return true;
        }

        float DistanceSq = Chunk->Bounds.ComputeSquaredDistanceToPoint(ViewLocation);
        if (DistanceSq < NearestDistanceSq) {
            OutNearestChunk = Chunk;
            NearestDistanceSq = DistanceSq;
        }
    }


    return Chunks.Num() > 0;
}

