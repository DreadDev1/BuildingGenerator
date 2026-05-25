//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/ThemeEngine/SceneProviders/DungeonSceneProvider.h"

#include "Core/Utils/DungeonModelHelper.h"
#include "Frameworks/ThemeEngine/SceneProviders/Spawners/DungeonSceneProviderSpawnStrategy.h"

DEFINE_LOG_CATEGORY(DungeonSceneProvider);

static TAutoConsoleVariable<int32> CVarDAMaxLatentTasks(
    TEXT("da.MaxLatentTasks"),
    96,
    TEXT("Maximum concurrent latent tasks (GPU voxel generation, etc). Higher values improve pipeline utilization but use more GPU memory."),
    ECVF_Default);

/////////////////////////////// FDungeonSceneProviderBase ///////////////////////////////
void FDungeonSceneProvider::OnDungeonBuildStart(ADungeon* InDungeon, const FGuid& InChunkTag) {
    SpawnStrategy = CreateSpawnStrategy();
    
    // Clear all command tracking
    CommandList = {};
    PendingCommands.Empty();
    ActiveCommands.Empty();
    TotalCommandCount = 0;
    CompletedCommandCount = 0;

    if (SpawnStrategy.IsValid()) {
        SpawnStrategy->Initialize(CommandList, InDungeon, InChunkTag);
    }
}

void FDungeonSceneProvider::OnDungeonBuildStop(ADungeon* InDungeon, const FGuid& InChunkTag) {
    if (SpawnStrategy.IsValid()) {
        SpawnStrategy->Finalize(CommandList, InDungeon, InChunkTag);
    }
    
    // Sort the game commands based on priority
    for (const TSharedPtr<FDungeonSceneProviderCommand>& Command : CommandList.GetCommands()) {
        Command->UpdateExecutionPriority(BuildPriorityLocation);
    }
    CommandList.GetMutableCommands().Sort(FDungeonSceneProviderCommand::WeightSortPredicate);
}

void FDungeonSceneProvider::AddStaticMesh(const FDungeonSceneProviderContext& Context, UDungeonMesh* Mesh) {
    if (SpawnStrategy.IsValid()) {
        SpawnStrategy->AddStaticMesh(Context, CommandList, Mesh);
    }
}

void FDungeonSceneProvider::AddStaticMesh(const FDungeonSceneProviderContext& Context, UStaticMesh* Mesh, bool bCanAffectNavigation) {
    if (SpawnStrategy.IsValid()) {
        SpawnStrategy->AddStaticMesh(Context, CommandList, Mesh, bCanAffectNavigation);
    }
}

void FDungeonSceneProvider::AddLight(const FDungeonSceneProviderContext& Context, UPointLightComponent* LightTemplate) {
    if (SpawnStrategy.IsValid()) {
        SpawnStrategy->AddLight(Context, CommandList, LightTemplate);
    }
}

void FDungeonSceneProvider::AddParticleSystem(const FDungeonSceneProviderContext& Context, UParticleSystem* ParticleTemplate) {
    if (SpawnStrategy.IsValid()) {
        SpawnStrategy->AddParticleSystem(Context, CommandList, ParticleTemplate);
    }
}

void FDungeonSceneProvider::AddActorFromTemplate(const FDungeonSceneProviderContext& Context, UClass* ClassTemplate, bool bCanAffectNavigation) {
    if (SpawnStrategy.IsValid()) {
        SpawnStrategy->AddActorFromTemplate(Context, CommandList, ClassTemplate, bCanAffectNavigation);
    }
}

void FDungeonSceneProvider::AddClonedActor(const FDungeonSceneProviderContext& Context, UDungeonActorTemplate* ActorTemplate) {
    if (SpawnStrategy.IsValid()) {
        SpawnStrategy->AddClonedActor(Context, CommandList, ActorTemplate);
    }
}

void FDungeonSceneProvider::AddGroupActor(const FDungeonSceneProviderContext& Context, const TArray<FName>& ActorNodeIds) {
    if (SpawnStrategy.IsValid()) {
        SpawnStrategy->AddGroupActor(Context, CommandList, ActorNodeIds);
    }
}

void FDungeonSceneProvider::AddCustomCommand(TSharedPtr<FDungeonSceneProviderCommand> SceneCommand) {
    CommandList.Add(SceneCommand);
}

void FDungeonSceneProvider::ExecuteAllCommands(UWorld* InWorld) {
    RunGameThreadCommands(InWorld, 0.0f);
}

void FDungeonSceneProvider::RunGameThreadCommands(UWorld* InWorld, float MaxBuildTimePerFrameMs) {
    const int32 MaxLatentTasks = CVarDAMaxLatentTasks.GetValueOnGameThread();
    
    // Use more efficient platform time
    const double FrameStartTime = FPlatformTime::Seconds();
    const bool bHasTimeBudget = MaxBuildTimePerFrameMs > 0;
    const double MaxBuildTimeSeconds = MaxBuildTimePerFrameMs * 0.001;
    constexpr float DeltaTime = 0.016f; // Fixed 16ms frame time for ticking

    auto FinaliseCommand = [&](const TSharedPtr<FDungeonSceneProviderCommand>& Command) {
        if (Command.IsValid() && Command->HasAdditionalEmittedCommands()) {
            for (const TSharedPtr<FDungeonSceneProviderCommand>& NewCommand : Command->GetAdditionalEmittedCommands()) {
                if (NewCommand.IsValid()) {
                    PendingCommands.HeapPush(NewCommand, FDungeonSceneProviderCommand::WeightSortPredicate);
                    TotalCommandCount++;
                }
            }
        }
    };
    
    // First, tick all active multi-frame commands (they have priority)
    if (!ActiveCommands.IsEmpty()) {
        for (int32 i = ActiveCommands.Num() - 1; i >= 0; i--) {
            TSharedPtr<FDungeonSceneProviderCommand> Command = ActiveCommands[i];
            
            // Tick the command
            Command->Tick(InWorld, DeltaTime);
            
            // Remove if finished
            if (Command->IsFinished()) {
                FinaliseCommand(Command);
                CompletedCommandCount++;
                ActiveCommands.RemoveAt(i);
            }
            
            // Early exit if we have a time budget and already exceeded it
            if (bHasTimeBudget) {
                const double ElapsedTime = FPlatformTime::Seconds() - FrameStartTime;
                if (ElapsedTime >= MaxBuildTimeSeconds) {
                    return;
                }
            }
        }
    }

    // Process the command list
    if (!CommandList.GetCommands().IsEmpty()) {
        for (const TSharedPtr<FDungeonSceneProviderCommand>& Command : CommandList.GetCommands()) {
            if (Command.IsValid()) {
                PendingCommands.HeapPush(Command, FDungeonSceneProviderCommand::WeightSortPredicate);
                TotalCommandCount++;
            }
        }
        CommandList = {};
    }

    TArray<TSharedPtr<FDungeonSceneProviderCommand>> PendingLatentCommands;
    
    // Process pending commands with remaining time budget
    while (PendingCommands.Num() > 0) {
        // Check time budget before dequeuing
        if (bHasTimeBudget) {
            const double ElapsedTime = FPlatformTime::Seconds() - FrameStartTime;
            if (ElapsedTime >= MaxBuildTimeSeconds) {
                break; // Out of time, leave command in queue
            }
        }
        
        TSharedPtr<FDungeonSceneProviderCommand> Command;
        PendingCommands.HeapPop(Command, FDungeonSceneProviderCommand::WeightSortPredicate);

        if (ActiveCommands.Num() < MaxLatentTasks || !Command->GetHints().bIsLatent) {
            // Execute the command
            Command->Execute(InWorld);
        
            // Check if command finished immediately
            if (Command->IsFinished()) {
                FinaliseCommand(Command);
                CompletedCommandCount++;
            } else {
                // Add to active commands for multi-frame processing
                ActiveCommands.Add(Command);
            }
        }
        else {
            // Command is latent, and we need to put this back in somehow after this loop finishes, but continue executing others in the pending command list
            PendingLatentCommands.Add(Command);
        }
    }
    
    if (PendingLatentCommands.Num() > 0) {
        for (const TSharedPtr<FDungeonSceneProviderCommand>& Command : PendingLatentCommands) {
            PendingCommands.HeapPush(Command, FDungeonSceneProviderCommand::WeightSortPredicate);
        }
        PendingLatentCommands.Reset();
    }
}

void FDungeonSceneProvider::ApplyExecutionWeights() const {
    for (const TSharedPtr<FDungeonSceneProviderCommand>& Command : CommandList.GetCommands()) {
        Command->UpdateExecutionPriority(BuildPriorityLocation);
    }
}

bool FDungeonSceneProvider::IsRunningGameThreadCommands() const {
    return !PendingCommands.IsEmpty() || ActiveCommands.Num() > 0 || !CommandList.GetCommands().IsEmpty();
}

void FDungeonSceneProvider::SetBuildPriorityLocation(const FVector& Location) {
    this->BuildPriorityLocation = Location;
}

