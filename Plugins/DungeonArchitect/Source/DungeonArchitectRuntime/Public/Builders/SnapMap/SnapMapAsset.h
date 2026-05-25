//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/GraphGrammar/GraphGrammar.h"
#include "SnapMapAsset.generated.h"

/** This class represents the data model for the dungeon flow graph */
UCLASS(Blueprintable, DisplayName="SnapMap Flow Graph")
class DUNGEONARCHITECTRUNTIME_API USnapMapAsset : public UObject {
    GENERATED_UCLASS_BODY()

public:
    UPROPERTY()
    TObjectPtr<UGraphGrammar> MissionGrammar;

#if WITH_EDITORONLY_DATA
    // Save a copy of the editor's preview dungeon so when it is reopened, the config is restored
    UPROPERTY()
    TObjectPtr<UObject> PreviewDungeonConfigCopy;
#endif // WITH_EDITORONLY_DATA

};

