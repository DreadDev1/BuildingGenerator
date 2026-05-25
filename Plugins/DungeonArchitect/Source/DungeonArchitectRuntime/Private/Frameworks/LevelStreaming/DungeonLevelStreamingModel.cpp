//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/LevelStreaming/DungeonLevelStreamingModel.h"

#include "Core/Dungeon.h"
#include "Core/Utils/DungeonConstants.h"
#include "Core/Utils/DungeonUtils.h"
#include "Frameworks/LevelStreaming/DungeonLevelStreamingNavigation.h"
#include "Frameworks/LevelStreaming/Interfaces/DungeonStreamingChunkEvent.h"
#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommand.h"

#include "Engine/Engine.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "LevelInstance/LevelInstanceLevelStreaming.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY(LogLevelStreamingModel);


ADungeonStreamingChunkLevelInstance::ADungeonStreamingChunkLevelInstance() {
    
}

///////////////////////////////// UDungeonStreamingChunk ///////////////////////////////// 
void ADungeonStreamingChunkLevelInstance::OnLevelInstanceLoaded() {
    Super::OnLevelInstanceLoaded();

    CacheOriginalHiddenStates();
    if (const ULevelInstanceSubsystem* LevelInstanceSubsystem = UWorld::GetSubsystem<ULevelInstanceSubsystem>(GetWorld())) {
        LevelStreaming = LevelInstanceSubsystem->GetLevelInstanceLevelStreaming(this);
        if (LevelStreaming.IsValid()) {
            RegisterStreamingCallbacks();
        }
    }
}

void ADungeonStreamingChunkLevelInstance::CacheOriginalHiddenStates() {
    ChildActorHiddenStates.Reset();
    if (ULevelInstanceSubsystem* LevelInstanceSubsystem = GetLevelInstanceSubsystem()) {
        if (ULevel* LevelInstanceLevel = LevelInstanceSubsystem->GetLevelInstanceLevel(this)) {
            for (AActor* LevelActor : LevelInstanceLevel->Actors) {
                if (IsValid(LevelActor)) {
                    bool& StateRef = ChildActorHiddenStates.FindOrAdd(LevelActor->GetUniqueID());
                    StateRef = LevelActor->IsHidden();
                }
            }
        }
    }
}

void ADungeonStreamingChunkLevelInstance::RegisterStreamingCallbacks() {
    if (LevelStreaming.IsValid() && Chunk.IsValid()) {
        UnregisterStreamingCallbacks();
        LevelStreaming->OnLevelLoaded.AddDynamic(Chunk.Get(), &UDungeonStreamingChunk::HandleChunkLoaded);
        LevelStreaming->OnLevelUnloaded.AddDynamic(Chunk.Get(), &UDungeonStreamingChunk::HandleChunkUnloaded);
        LevelStreaming->OnLevelShown.AddDynamic(Chunk.Get(), &UDungeonStreamingChunk::HandleChunkVisible);
        LevelStreaming->OnLevelHidden.AddDynamic(Chunk.Get(), &UDungeonStreamingChunk::HandleChunkHidden);
    }
}

void ADungeonStreamingChunkLevelInstance::UnregisterStreamingCallbacks() const {
    if (LevelStreaming.IsValid()) {
        LevelStreaming->OnLevelLoaded.RemoveAll(this);
        LevelStreaming->OnLevelUnloaded.RemoveAll(this);
        LevelStreaming->OnLevelShown.RemoveAll(this);
        LevelStreaming->OnLevelHidden.RemoveAll(this);
    }
}

bool ADungeonStreamingChunkLevelInstance::SetWorldAssetImpl(const TSoftObjectPtr<UWorld>& NewWorldAsset) {
#if WITH_EDITOR
    return SetWorldAsset(NewWorldAsset);
#else // WITH_EDITOR
    return SetWorldAssetImplRuntime(NewWorldAsset);
#endif // WITH_EDITOR

}

void ADungeonStreamingChunkLevelInstance::SetActorHiddenInGame(bool bNewHidden) {
    bool bOldHidden = IsHidden();
    Super::SetActorHiddenInGame(bNewHidden);

    if (bOldHidden != bNewHidden) {
        if (ULevelInstanceSubsystem* LevelInstanceSubsystem = GetLevelInstanceSubsystem()) {
            if (ULevel* LevelInstanceLevel = LevelInstanceSubsystem->GetLevelInstanceLevel(this)) {
                for (AActor* LevelActor : LevelInstanceLevel->Actors) {
                    if (IsValid(LevelActor)) {
                        auto bActorHiddenInGame = ChildActorHiddenStates.FindRef(LevelActor->GetUniqueID());
                        if (!bActorHiddenInGame) {
                            LevelActor->SetActorHiddenInGame(bNewHidden);
                        }
                    }
                }
            }
        }
    }
}

void ADungeonStreamingChunkLevelInstance::SetNetworkGuid(const FGuid& InGuid) {
    NetworkGuid = InGuid;
    LevelInstanceActorGuid.ActorGuid = InGuid;
}

bool ADungeonStreamingChunkLevelInstance::SetWorldAssetImplRuntime(const TSoftObjectPtr<UWorld>& NewWorldAsset) {
#if WITH_EDITORONLY_DATA
    WorldAsset = NewWorldAsset;
#endif // WITH_EDITORONLY_DATA
    CookedWorldAsset = NewWorldAsset;
    return true;
}

///////////////////////////////// UDungeonStreamingChunk ///////////////////////////////// 
UDungeonStreamingChunk::UDungeonStreamingChunk(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
}

ADungeon* UDungeonStreamingChunk::GetOwningDungeon() const {
    if (const UDungeonLevelStreamingModel* LevelStreamingModel = Cast<UDungeonLevelStreamingModel>(GetOuter())) {
        return Cast<ADungeon>(LevelStreamingModel->GetOuter());
    }
    return nullptr;
}

FRandomStream& UDungeonStreamingChunk::GetRandomStream(ADungeon* InDungeon) {
    if (InDungeon && InDungeon->GetBuilder()) {
        return InDungeon->GetBuilder()->GetRandomStreamRef();
    }
    return FallbackRandomStream;
}

ADungeon* UDungeonStreamingChunk::GetDungeon() const {
    if (const UDungeonLevelStreamingModel* LevelStreamingModel = GetLevelStreamingModel()) {
        return Cast<ADungeon>(LevelStreamingModel->GetOuter());
    }
    return nullptr;
}

UDungeonLevelStreamingModel* UDungeonStreamingChunk::GetLevelStreamingModel() const {
    return Cast<UDungeonLevelStreamingModel>(GetOuter());
}


void UDungeonStreamingChunk::NotifyEventListener_OnVisible() {
    // Notify all the chunk event listener objects
    for (TWeakObjectPtr<AActor> EventListener : ChunkListeners) {
        if (EventListener.IsValid()) {
            IDungeonStreamingChunkEventInterface::Execute_OnChunkVisible(EventListener.Get(), GetOwningDungeon());
        }
    }

    if (UDungeonLevelStreamingModel* LevelStreamingModel = Cast<UDungeonLevelStreamingModel>(GetOuter())) {
        LevelStreamingModel->NotifyChunkVisible(this);
    } 
}

void UDungeonStreamingChunk::NotifyEventListener_OnHidden() {
    // Notify all the chunk event listener objects
    for (TWeakObjectPtr<AActor> EventListener : ChunkListeners) {
        if (EventListener.IsValid()) {
            IDungeonStreamingChunkEventInterface::Execute_OnChunkHidden(EventListener.Get(), GetOwningDungeon());
        }
    }
    
    if (UDungeonLevelStreamingModel* LevelStreamingModel = Cast<UDungeonLevelStreamingModel>(GetOuter())) {
        LevelStreamingModel->NotifyChunkHidden(this);
    }
}

void UDungeonStreamingChunk::NotifyEventListener_OnLoaded() {
    
    // Notify all the chunk event listener objects
    for (TWeakObjectPtr<AActor> EventListener : ChunkListeners) {
        if (EventListener.IsValid()) {
            IDungeonStreamingChunkEventInterface::Execute_OnChunkLoaded(EventListener.Get(), GetOwningDungeon());
        }
    }

    if (UDungeonLevelStreamingModel* LevelStreamingModel = Cast<UDungeonLevelStreamingModel>(GetOuter())) {
        LevelStreamingModel->NotifyChunkLoaded(this);
    }
}

void UDungeonStreamingChunk::NotifyEventListener_OnUnloaded() {
    // Notify all the chunk event listener objects
    for (TWeakObjectPtr<AActor> EventListener : ChunkListeners) {
        if (EventListener.IsValid()) {
            IDungeonStreamingChunkEventInterface::Execute_OnChunkUnloaded(EventListener.Get(), GetOwningDungeon());
        }
    }

    if (UDungeonLevelStreamingModel* LevelStreamingModel = Cast<UDungeonLevelStreamingModel>(GetOuter())) {
        LevelStreamingModel->NotifyChunkUnloaded(this);
    }
}

ULevelStreamingDynamic* FLevelStreamLoader::LoadLevelInstanceBySoftObjectPtr(
    const UObject* WorldContextObject, const TSoftObjectPtr<UWorld>& Level, uint32 InstanceId, const FTransform& Transform,
    bool& bOutSuccess, UPackage*& OutLevelPackage) {
    bOutSuccess = false;
    OutLevelPackage = nullptr;

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World) {
        return nullptr;
    }

    // Check whether requested map exists, this could be very slow if LevelName is a short package name
    if (Level.IsNull()) {
        return nullptr;
    }

    bOutSuccess = true;
    return LoadLevelInstance_Internal(World, Level.GetLongPackageName(), InstanceId, Transform,
                                      OutLevelPackage);
}

ULevelStreamingDynamic* FLevelStreamLoader::LoadLevelInstance_Internal(UWorld* World, const FString& LongPackageName,
                                                                       uint32 InstanceId, const FTransform& Transform, UPackage*& OutLevelPackage) {
    if (!World) return nullptr;

    // Create Unique Name for sub-level package
    const FString PackagePath = FPackageName::GetLongPackagePath(LongPackageName);
    FString ShortPackageName = FPackageName::GetShortName(LongPackageName);
    
    if (ShortPackageName.StartsWith(World->StreamingLevelsPrefix))
    {
        ShortPackageName.RightChopInline(World->StreamingLevelsPrefix.Len(), EAllowShrinking::No);
    }

    // Remove PIE prefix if it's there before we actually load the level
    const FString OnDiskPackageName = PackagePath + TEXT("/") + ShortPackageName;
    
    // Create Unique Name for sub-level package
    //const uint32 Hash = HashCombine(GetTypeHash(Location), GetTypeHash(Rotation.Euler()));
    const uint32 Hash = GetTypeHash(Transform);
    FString UniqueLevelPackageName = FString::Printf(TEXT("%s/%s_LI_%s_%d_%u"), *PackagePath, 
            *ShortPackageName, *World->GetName(), InstanceId, Hash);

    if (!World->IsGameWorld()) {
        UPackage* LevelPackage = CreatePackage(*UniqueLevelPackageName);
        LevelPackage->SetFlags(RF_Transient | RF_DuplicateTransient);
        LevelPackage->SetDirtyFlag(false);
        OutLevelPackage = LevelPackage;
    }
    else {
        UniqueLevelPackageName += TEXT("G");
    }
    
#if WITH_EDITOR
    const bool bIsPlayInEditor = World->IsPlayInEditor();
    int32 PIEInstance = INDEX_NONE;
    if (bIsPlayInEditor)
    {
        const FWorldContext& WorldContext = GEngine->GetWorldContextFromWorldChecked(World);
        PIEInstance = WorldContext.PIEInstance;
        UniqueLevelPackageName = UWorld::ConvertToPIEPackageName(UniqueLevelPackageName, PIEInstance);
    }
#endif

    ULevelStreamingDynamic* StreamingLevel = NewObject<ULevelStreamingDynamic>(
        World, ULevelStreamingDynamic::StaticClass(), NAME_None, RF_Transient | RF_DuplicateTransient, nullptr);
    StreamingLevel->SetWorldAssetByPackageName(FName(*UniqueLevelPackageName));

#if WITH_EDITOR
    if (bIsPlayInEditor)
    {
        // Necessary for networking in PIE
        StreamingLevel->RenameForPIE(PIEInstance);
    }
#endif // WITH_EDITOR
    
    StreamingLevel->LevelColor = FColor::MakeRandomColor();
    StreamingLevel->SetShouldBeLoaded(false);
    StreamingLevel->SetShouldBeVisible(false);
    StreamingLevel->bShouldBlockOnLoad = false;
    StreamingLevel->bShouldBlockOnUnload = false;
    StreamingLevel->bInitiallyLoaded = false;
    StreamingLevel->bInitiallyVisible = false;
    // Transform
    StreamingLevel->LevelTransform = Transform;
    // Map to Load
    StreamingLevel->PackageNameToLoad = FName(*OnDiskPackageName);

    // Add the new level to world.
    World->AddStreamingLevel(StreamingLevel);

    return StreamingLevel;
}

TArray<AActor*> UDungeonStreamingChunk::GetLoadedChunkActors() {
    if (const ULevelStreamingDynamic* LevelStreaming = GetStreamingLevel()) {
        if (const ULevel* LoadedLevel = LevelStreaming->GetLoadedLevel()) {
            return LoadedLevel->Actors;
        }
    }

    return TArray<AActor*>();
}

TArray<AActor*> UDungeonStreamingChunk::GetLoadedChunkActorsOfType(const UClass* ActorClass)
{
    return GetLoadedChunkActors().FilterByPredicate([ActorClass](const AActor* Actor) -> bool
    {
        return Actor && Actor->IsA(ActorClass);
    });
}

AActor* UDungeonStreamingChunk::GetLoadedChunkActorOfType(const UClass* ActorClass)
{
    for (AActor* Actor : GetLoadedChunkActors())
    {
        if (Actor && Actor->IsA(ActorClass))
        {
            return Actor;
        }
    }
    return nullptr;
}

void UDungeonStreamingChunk::CreateLevelStreaming(UObject* WorldContextObject, TSoftObjectPtr<UWorld> Level,
                                                  const FGuid& InstanceId, const FTransform& Transform, bool& bOutSuccess) {
    const FString NetworkStableId = FString("Module_") + InstanceId.ToString();
    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = FName(NetworkStableId);
    SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
    SpawnParams.OverrideLevel = World->PersistentLevel;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    LevelInstanceActor = World->SpawnActor<ADungeonStreamingChunkLevelInstance>(ADungeonStreamingChunkLevelInstance::StaticClass(), Transform,  SpawnParams);
    LevelInstanceActor->SetNetworkGuid(InstanceId);
    LevelInstanceActor->Chunk = this;

#if WITH_EDITOR
    if (const ADungeon* Dungeon = GetDungeon()) {
        const FString FolderName = Dungeon->ItemFolderPath.ToString() + FDungeonArchitectConstants::DungeonFolderPathSuffix_SnapInstances;
        LevelInstanceActor->SetFolderPath(FName(FolderName));
    }
#endif // WITH_EDITOR

    // TODO: Check if this is needed
    //LevelInstanceActor->DesiredRuntimeBehavior = ELevelInstanceRuntimeBehavior::LevelStreaming;
    
    LevelInstanceActor->SetWorldAssetImpl(Level);
    ADungeon* DungeonOwner{};
    UObject* Outer = this;
    while (Outer) {
        if (Outer->IsA<ADungeon>()) {
            DungeonOwner = Cast<ADungeon>(Outer);
            break;
        }
        Outer = Outer->GetOuter();
    }

    if (DungeonOwner) {
        const FName DungeonTag = FDungeonUtils::GetDungeonIdTag(DungeonOwner);
        LevelInstanceActor->Tags.Add(DungeonTag);
        LevelInstanceActor->Tags.Add(FDungeonSceneProviderCommand::TagComplexActor);
    }
    
    ManagedActors.Add(LevelInstanceActor);
    bOutSuccess = true;
}

void UDungeonStreamingChunk::SetStreamingLevelState(bool bInVisible, EDungeonLevelStreamUnloadMethod UnloadMethod) {
    if (LevelInstanceActor.IsValid() && RequiresStateUpdate(bInVisible)) {
        bShouldBeVisible = bInVisible;

        if (bInVisible) {
            if (LevelInstanceActor->IsLoaded()) {
                if (LevelInstanceActor->IsHidden()) {
                    // Loaded, but hidden. Make it visible
                    LevelInstanceActor->SetActorHiddenInGame(false);
                    HandleChunkVisible();
                }
            }
            else {
                LevelInstanceActor->SetActorHiddenInGame(false);
                LevelInstanceActor->LoadLevelInstance();
            }
        }
        else {
            if (UnloadMethod == EDungeonLevelStreamUnloadMethod::UnloadHiddenChunks) {
                LevelInstanceActor->UnloadLevelInstance();
            }
            else {
                LevelInstanceActor->SetActorHiddenInGame(true);
                HandleChunkHidden();
            }
        }
    }
}

bool UDungeonStreamingChunk::RequiresStateUpdate(bool bInVisible) const {
    return bInVisible != bShouldBeVisible;
}

void UDungeonStreamingChunk::RegisterManagedActor(TWeakObjectPtr<AActor> InActor) {
    ManagedActors.Add(InActor);
}

void UDungeonStreamingChunk::DestroyChunk(UWorld* InWorld) {
    // Destroy the managed actors
    for (TWeakObjectPtr<AActor> ManagedActor : ManagedActors) {
        if (ManagedActor.IsValid()) {
            ManagedActor->Destroy();
        }
    }
    ManagedActors.Reset();
    
    NotifyEventListener_OnUnloaded();
    if (LevelInstanceActor.IsValid()) {
        LevelInstanceActor->Destroy();
        LevelInstanceActor = nullptr;
    }
    LevelPackage = nullptr;
}

UPackage* UDungeonStreamingChunk::GetLevelPackage() const {
    return LevelPackage;
}

ULevel* UDungeonStreamingChunk::GetLoadedLevel() const {
    const ULevelStreamingDynamic* LevelStreaming = GetStreamingLevel();
    return LevelStreaming ? LevelStreaming->GetLoadedLevel() : nullptr;
}

ULevelStreamingDynamic* UDungeonStreamingChunk::GetStreamingLevel() const {
    return LevelInstanceActor.IsValid() ? LevelInstanceActor->GetCachedLevelStreaming().Get() : nullptr;
}

FTransform UDungeonStreamingChunk::GetLevelTransform() const {
    return LevelInstanceActor.IsValid() ? LevelInstanceActor->GetActorTransform() : FTransform::Identity;
}

void UDungeonStreamingChunk::HandleChunkVisible() {
    NotifyEventListener_OnVisible();
    
    const ULevelStreamingDynamic* LevelStreaming = GetStreamingLevel();
    const UDungeonLevelStreamingModel* StreamingModel = Cast<UDungeonLevelStreamingModel>(GetOuter());
    if (StreamingModel && StreamingModel->StreamingNavigation && LevelStreaming) {
        if (ULevel* LoadedLevel = LevelStreaming->GetLoadedLevel()) {
            StreamingModel->StreamingNavigation->AddLevelNavigation(LoadedLevel, Bounds);
        }
    }
    bIsVisible = true;
}

void UDungeonStreamingChunk::HandleChunkHidden() {
    NotifyEventListener_OnHidden();
    
    const ULevelStreamingDynamic* LevelStreaming = GetStreamingLevel();
    const UDungeonLevelStreamingModel* StreamingModel = Cast<UDungeonLevelStreamingModel>(GetOuter());
    if (LevelStreaming && StreamingModel && StreamingModel->StreamingNavigation) {
        StreamingModel->StreamingNavigation->RemoveLevelNavigation(LevelStreaming->GetLoadedLevel());
    }
    bIsVisible = false;
}

void UDungeonStreamingChunk::HandleChunkLoaded() {
    ChunkListeners.Reset();
    if (ULevel* Level = GetLoadedLevel()) {
        for (AActor* Actor : Level->Actors) {
            if (Actor && Actor->Implements<UDungeonStreamingChunkEventInterface>()) {
                ChunkListeners.Add(Actor);
            }
        }
    }
    
    NotifyEventListener_OnLoaded();
    
    bIsLoaded = true;
}

void UDungeonStreamingChunk::HandleChunkUnloaded() {
    ChunkListeners.Reset();
    
    NotifyEventListener_OnUnloaded();
    
    bIsLoaded = false;
}

///////////////////////////////// UDungeonLevelStreamingModel /////////////////////////////////

UDungeonLevelStreamingModel::UDungeonLevelStreamingModel(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer) {
    StreamingNavigation = ObjectInitializer.CreateDefaultSubobject<UDungeonLevelStreamingNavigation>(
        this, "StreamingNavigation");
    StreamingNavigation->bAutoResizeNavVolume = true;
}

void UDungeonLevelStreamingModel::Initialize(UWorld* InWorld) {
    bNotifiedInitialChunkLoadEvent = false;
    StreamingNavigation->Initialize(InWorld);
}

void UDungeonLevelStreamingModel::Release(UWorld* InWorld) {
    StreamingNavigation->Release();

    for (UDungeonStreamingChunk* Chunk : Chunks) {
        if (Chunk) {
            // Destroy the chunk
            Chunk->DestroyChunk(InWorld);
        }
    }
    Chunks.Empty();

    if (!InWorld->IsGameWorld()) {
        // Unload the transient levels from memory
        CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
    }
}

void UDungeonLevelStreamingModel::NotifyInitialChunksLoaded() {
    ADungeon* Dungeon = Cast<ADungeon>(GetOuter());

    // Fire the deferred build complete event first (before OnInitialChunksLoaded)
    // This ensures OnDungeonBuildComplete fires before OnInitialChunksLoaded for backward compat
    if (Dungeon) {
        Dungeon->FireDeferredBuildCompleteEvent();
    }

    // Then fire OnInitialChunksLoaded (for backward compat, now redundant with OnDungeonBuildComplete)
    OnInitialChunksLoaded.Broadcast(Dungeon);
    bNotifiedInitialChunkLoadEvent = true;
}

void UDungeonLevelStreamingModel::NotifyChunkLoaded(UDungeonStreamingChunk* InChunk)
{
    ADungeon* Dungeon = Cast<ADungeon>(GetOuter());
    OnChunkLoaded.Broadcast(Dungeon, InChunk);
}

void UDungeonLevelStreamingModel::NotifyChunkUnloaded(UDungeonStreamingChunk* InChunk)
{
    ADungeon* Dungeon = Cast<ADungeon>(GetOuter());
    OnChunkUnloaded.Broadcast(Dungeon, InChunk);
}

void UDungeonLevelStreamingModel::NotifyChunkVisible(UDungeonStreamingChunk* InChunk)
{
    ADungeon* Dungeon = Cast<ADungeon>(GetOuter());
    OnChunkVisible.Broadcast(Dungeon, InChunk);
}

void UDungeonLevelStreamingModel::NotifyChunkHidden(UDungeonStreamingChunk* InChunk)
{
    ADungeon* Dungeon = Cast<ADungeon>(GetOuter());
    OnChunkHidden.Broadcast(Dungeon, InChunk);
}

namespace {
    void GetPlayerSourceLocations(UWorld* InWorld, TArray<FVector>& OutSourceLocations) {
        OutSourceLocations.Reset();
        for (TActorIterator<APlayerController> It(InWorld); It; ++It) {
            APlayerController* PlayerController = *It;
            if (PlayerController) {
                APawn* Pawn = PlayerController->GetPawnOrSpectator();
                if (Pawn) {
                    FVector PlayerLocation = Pawn->GetActorLocation();
                    OutSourceLocations.Add(PlayerLocation);
                }
            }
        }
    }

    void GetAlwaysResidentSourceLocations(TArray<UDungeonStreamingChunk*> InChunks, TArray<FVector>& OutSourceLocations) {
        OutSourceLocations.Reset();
        for (UDungeonStreamingChunk* Chunk : InChunks) {
            if (Chunk && Chunk->bAlwaysResident) {
                OutSourceLocations.Add(Chunk->Bounds.GetCenter());
            }
        }
    }
}

void UDungeonLevelStreamingModel::GetStreamingSourceLocations(UWorld* InWorld, EDungeonLevelStreamChunkSelection InChunkSelection, TArray<FVector>& OutSourceLocations) const {
    switch(InChunkSelection) {
        case EDungeonLevelStreamChunkSelection::AlwaysResidentLocations:
            GetAlwaysResidentSourceLocations(Chunks, OutSourceLocations);
            break;

        case EDungeonLevelStreamChunkSelection::PlayerLocations:
        default:
            GetPlayerSourceLocations(InWorld, OutSourceLocations);
            break;
    }
}

