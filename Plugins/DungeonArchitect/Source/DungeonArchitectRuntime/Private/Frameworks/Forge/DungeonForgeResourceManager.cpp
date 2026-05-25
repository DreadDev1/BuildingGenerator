//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/DungeonForgeResourceManager.h"

#include "Core/Utils/DungeonLog.h"

#include "Engine/Texture.h"

////////////////////////////////// UDungeonForgeResourceManager //////////////////////////////////
void UDungeonForgeResourceManager::RegisterManagedTexture(UTexture* InTexture) {
	if (InTexture) {
		ManagedTextures.AddUnique(InTexture);
	}
}

void UDungeonForgeResourceManager::ReleaseResources() {
	for (TObjectPtr<UTexture> ManagedTexture : ManagedTextures) {
		if (ManagedTexture) {
			ManagedTexture->ReleaseResource();
			UE_LOG(LogDungeonForge, Log, TEXT("Released Managed Texture: %s"), *ManagedTexture->GetName());
		}
	}
	ManagedTextures.Reset();
}

