//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "DetailLayoutBuilder.h"

class ADungeon;
class UDungeonConfig;
struct FDungeonLevelStreamingConfig;

namespace DADetailsCustomizationHelpers {
	ADungeon* GetDungeon(IDetailLayoutBuilder& DetailBuilder);
    void ShowDungeonConfigProperties(IDetailLayoutBuilder& DetailBuilder, UDungeonConfig* Config);
    void NotifyPropertyChanged(UObject* InObject, const FName& InFieldName);
    void ShowLevelStreamingProperties(IDetailLayoutBuilder& DetailBuilder, FDungeonLevelStreamingConfig* StreamingConfig);

	template <typename T>
	static TObjectPtr<T> GetEditedObject(IDetailLayoutBuilder& DetailBuilder) {
		TArray<TWeakObjectPtr<UObject>> OutObjects;
		DetailBuilder.GetObjectsBeingCustomized(OutObjects);
		T* Obj = nullptr;
		if (OutObjects.Num() > 0) {
			Obj = Cast<T>(OutObjects[0].Get());
		}
		return Obj;
	}
};

