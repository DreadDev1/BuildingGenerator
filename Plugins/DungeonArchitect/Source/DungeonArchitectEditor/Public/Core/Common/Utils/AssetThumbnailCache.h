//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "AssetThumbnail.h"

class FAssetThumbnail;
class SWidget;
struct FAssetThumbnailConfig;

class FDAAssetThumbnailCache {
public:
	FDAAssetThumbnailCache(const UObject* InAssetObject, const FIntPoint& InSize, const FAssetThumbnailConfig& InConfig = FAssetThumbnailConfig());
	bool IsValid() const;
	TWeakObjectPtr<const UObject> GetAssetObject() const { return AssetObject; }
	TSharedPtr<SWidget> GetWidget() const { return CachedAssetThumbnailWidget; }
	
private:
	TWeakObjectPtr<const UObject> AssetObject;
	TSharedPtr<FAssetThumbnail> CachedAssetThumbnail;
	TSharedPtr<SWidget> CachedAssetThumbnailWidget;
};


class FDAAssetThumbnailCacheManager {
public:
	FDAAssetThumbnailCacheManager(const FIntPoint& InSize, const FAssetThumbnailConfig& InConfig = FAssetThumbnailConfig()) 
		: ThumbnailSize(InSize)
		, ThumbnailConfig(InConfig) {}

	TSharedPtr<SWidget> RequestThumbnailWidget(const UObject* AssetObject) {
		if (!AssetObject) {
			return nullptr;
		}
            
		for (const auto& Cache : CacheList) {
			if (Cache.GetAssetObject().Get() == AssetObject) {
				return Cache.GetWidget();
			}
		}

		CacheList.Add(FDAAssetThumbnailCache(AssetObject, ThumbnailSize, ThumbnailConfig));
		return CacheList.Last().GetWidget();
	}

	void PurgeStaleEntries(const TArray<UObject*>& InValidAssets) {
		// Remove nulls from valid assets
		TArray<UObject*> ValidAssets = InValidAssets;
		ValidAssets.RemoveAll([](const UObject* Asset) { return Asset == nullptr; });
   
		CacheList.RemoveAll([&ValidAssets](const FDAAssetThumbnailCache& Cache) {
			return !Cache.IsValid() || !ValidAssets.Contains(Cache.GetAssetObject().Get());
		});
	}

	FORCEINLINE FIntPoint GetThumbnailSize() const { return ThumbnailSize; }
	
private:
	TArray<FDAAssetThumbnailCache> CacheList;
	FIntPoint ThumbnailSize;
	FAssetThumbnailConfig ThumbnailConfig;
};
