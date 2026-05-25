//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Canvas/DungeonCanvasTrackedObject.h"

#include "Core/Dungeon.h"
#include "Core/DungeonWorldSubsystem.h"
#include "Frameworks/Canvas/DungeonCanvas.h"

#include "Engine/World.h"

UDungeonCanvasTrackedObject::UDungeonCanvasTrackedObject() {
	bWantsInitializeComponent = true;
}

void UDungeonCanvasTrackedObject::BeginPlay() {
	Super::BeginPlay();

	
}

void UDungeonCanvasTrackedObject::InitializeComponent() {
	Super::InitializeComponent();
	
	if (UWorld* World = GetWorld()) {
		if (UDungeonWorldSubsystem* DungeonSubsystem = World->GetSubsystem<UDungeonWorldSubsystem>()) {
			for (ADungeon* Dungeon : DungeonSubsystem->GetAllActiveDungeons()) {
				if (UDungeonCanvasComponent* DungeonCanvasComponent = Dungeon->GetComponentByClass<UDungeonCanvasComponent>()) {
					DungeonCanvasComponent->AddTrackedOverlayActor(this);
				}
			}
		}
	}
}


