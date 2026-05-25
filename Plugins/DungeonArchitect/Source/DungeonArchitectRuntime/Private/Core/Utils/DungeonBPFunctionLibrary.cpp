//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Utils/DungeonBPFunctionLibrary.h"

#include "Core/Dungeon.h"
#include "Core/Utils/DungeonUtils.h"
#include "Frameworks/ThemeEngine/SceneProviders/Commands/DungeonSceneProviderCommand.h"

#include "Engine/CanvasRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogDungeonBPLibrary, Log, All);

AActor* UDungeonBPFunctionLibrary::SpawnDungeonOwnedActor(ADungeon* Dungeon, TSubclassOf<AActor> ActorClass,
                                                          const FTransform& Transform) {
    if (!Dungeon) {
        UE_LOG(LogDungeonBPLibrary, Error, TEXT("Invalid Dungeon reference passed to SpawnDungeonActor"));
        return nullptr;
    }

    UWorld* World = Dungeon->GetWorld();
    AActor* Actor = World->SpawnActor<AActor>(ActorClass, Transform);
    if (Actor) {
#if WITH_EDITOR
        if (World->WorldType == EWorldType::Editor) {
            Actor->RerunConstructionScripts();
        }
#endif // WITH_EDITOR
        const FName DungeonIdTag = FDungeonUtils::GetDungeonIdTag(Dungeon);
        Actor->Tags.Add(DungeonIdTag);

        FDungeonSceneProviderCommand::MoveToFolder(Actor, Dungeon);
        FDungeonSceneProviderCommand::TagAsComplexObject(Actor);
    }

    return Actor;
}

bool UDungeonBPFunctionLibrary::ActorBelongsToDungeon(ADungeon* Dungeon, AActor* ActorToCheck) {
    if (!Dungeon || !ActorToCheck) return false;
    const FName DungeonIdTag = FDungeonUtils::GetDungeonIdTag(Dungeon);
    return ActorToCheck->Tags.Contains(DungeonIdTag);
}

TArray<AActor*> UDungeonBPFunctionLibrary::GetAllDungeonSpawnedActors(ADungeon* Dungeon) {
    if (!Dungeon) return {};
    
    const FName DungeonIdTag = FDungeonUtils::GetDungeonIdTag(Dungeon);
    TArray<AActor*> TrackedActors;
    
    for (TActorIterator<AActor> ActorIt(Dungeon->GetWorld()); ActorIt; ++ActorIt) {
        AActor* Actor = *ActorIt;
        if (Actor && Actor->Tags.Contains(DungeonIdTag)) {
            TrackedActors.Add(Actor);
        }
    }
    return TrackedActors;
}

bool UDungeonBPFunctionLibrary::DungeonObjectHasAuthority(UObject* Object) {
    AActor* Actor = nullptr;
    while (Object) {
        Actor = Cast<AActor>(Object);
        if (Actor) break;
        Object = Object->GetOuter();
    }
    
    if (!Actor) return false;
    return Actor->HasAuthority();
}

void UDungeonBPFunctionLibrary::DAGetAllActorsOfClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors) {
    UGameplayStatics::GetAllActorsOfClass(WorldContextObject, ActorClass, OutActors);
}

AActor* UDungeonBPFunctionLibrary::DAGetActorOfClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass) {
    return UGameplayStatics::GetActorOfClass(WorldContextObject, ActorClass);
}

void UDungeonBPFunctionLibrary::DARecreateActorInLevel(AActor* InActor, ULevel* TargetLevel, AActor*& NewTargetActor) {
    if (!InActor || !TargetLevel) {
        return;
    }
    
    UWorld* World = TargetLevel->GetWorld();
    FActorSpawnParameters SpawnParams;
    SpawnParams.Template = InActor;
    SpawnParams.bDeferConstruction = true;
    SpawnParams.OverrideLevel = TargetLevel;
        
    NewTargetActor = World->SpawnActor<AActor>(InActor->GetClass(), SpawnParams);
    const FTransform TargetTransform = InActor->GetActorTransform();
    InActor->Destroy();
    
    NewTargetActor->FinishSpawning(TargetTransform);
}

int32 UDungeonBPFunctionLibrary::DAHashLocation(const FVector& Location) {
    return GetTypeHash(Location);
}

int32 UDungeonBPFunctionLibrary::DAHashTransform(const FTransform& Transform) {
    return GetTypeHash(Transform);
}

int32 UDungeonBPFunctionLibrary::DAHashCombine(int32 A, int32 B) {
    return HashCombine(A, B);
}

FDungeonLayoutData UDungeonBPFunctionLibrary::GetDungeonLayout(ADungeon* Dungeon) {
    return (Dungeon && Dungeon->DungeonModel)
        ? Dungeon->DungeonModel->DungeonLayout
        : FDungeonLayoutData();
}

FBox UDungeonBPFunctionLibrary::GetDungeonBounds(ADungeon* Dungeon) {
    return GetDungeonLayout(Dungeon).Bounds;
}

FBox UDungeonBPFunctionLibrary::GenerateDungeonLayoutBounds(const FDungeonLayoutData& DungeonLayout, float MarginPercent) {
    const FVector WorldSize = DungeonLayout.Bounds.GetSize();
    const FVector Margin = WorldSize * MarginPercent / 100.0f;
    const float MarginF = FMath::Max(Margin.X, Margin.Y);
    return DungeonLayout.Bounds.ExpandBy(MarginF);
}

UCanvasRenderTarget2D* UDungeonBPFunctionLibrary::DACreateCanvasRenderTexture(UObject* Owner, TEnumAsByte<ETextureRenderTargetFormat> Format, int Width, int Height, FLinearColor ClearColor) {
    if (Width == 0 || Height == 0) {
        UE_LOG(LogDungeonBPLibrary, Error, TEXT("Invalid texture size"));
        return nullptr;
    }
    
    UCanvasRenderTarget2D* RenderTarget2D = NewObject<UCanvasRenderTarget2D>(Owner);
    RenderTarget2D->RenderTargetFormat = Format;
    RenderTarget2D->InitAutoFormat(Width, Height);
    RenderTarget2D->ClearColor = ClearColor;
    RenderTarget2D->UpdateResourceImmediate(true);
    return RenderTarget2D; 
}

bool UDungeonBPFunctionLibrary::TextureNeedsRecreation(UTextureRenderTarget2D* Texture, TEnumAsByte<ETextureRenderTargetFormat> Format, int Width, int Height) {
    return !Texture || Texture->GetFormat() != GetPixelFormatFromRenderTargetFormat(Format) || Texture->SizeX != Width || Texture->SizeY != Height; 
}

