//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Common/Utils/AssetThumbnailCache.h"

#include "AssetRegistry/AssetData.h"
#include "AssetThumbnail.h"
#include "ThumbnailRendering/ThumbnailManager.h"

//////////////////////////////// FDAAssetThumbnailCache ////////////////////////////////
FDAAssetThumbnailCache::FDAAssetThumbnailCache(const UObject* InAssetObject, const FIntPoint& InSize, const FAssetThumbnailConfig& InConfig)
	: AssetObject(InAssetObject)
{
	TSharedPtr<FAssetThumbnailPool> AssetThumbnailPool = UThumbnailManager::Get().GetSharedThumbnailPool();
	CachedAssetThumbnail = MakeShareable(new FAssetThumbnail(AssetObject.Get(), InSize.X, InSize.Y, AssetThumbnailPool));
	CachedAssetThumbnailWidget = CachedAssetThumbnail->MakeThumbnailWidget();
}

bool FDAAssetThumbnailCache::IsValid() const {
	return AssetObject.IsValid() && CachedAssetThumbnail.IsValid() && CachedAssetThumbnailWidget.IsValid();
}


