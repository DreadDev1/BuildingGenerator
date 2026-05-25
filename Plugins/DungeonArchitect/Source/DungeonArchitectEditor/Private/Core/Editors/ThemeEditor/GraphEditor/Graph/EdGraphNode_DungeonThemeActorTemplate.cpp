//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/ThemeEditor/GraphEditor/Graph/EdGraphNode_DungeonThemeActorTemplate.h"

#include "Core/Actors/DungeonActorTemplate.h"

#include "ActorFactories/ActorFactory.h"
#include "AssetRegistry/AssetData.h"
#include "ComponentAssetBroker.h"
#include "Engine/Engine.h"

void UEdGraphNode_DungeonThemeActorTemplate::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
    Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UEdGraphNode_DungeonThemeActorTemplate::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) {
    if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(
        UEdGraphNode_DungeonThemeActorTemplate, ClassTemplate)) {
        SetTemplateClass(ClassTemplate);
    }
    Super::PostEditChangeChainProperty(PropertyChangedEvent);
}

void UEdGraphNode_DungeonThemeActorTemplate::OnThemeEditorLoaded() {
    if (!ActorTemplate) {
        SetTemplateClass(ClassTemplate);
    }
}

void UEdGraphNode_DungeonThemeActorTemplate::SetTemplateClass(TSubclassOf<AActor> InClass, AActor* InTemplate) {
    ClassTemplate = InClass;

    if (ClassTemplate) {
        if (ActorTemplate == nullptr || ActorTemplate->GetClass() != ClassTemplate) {
            Modify();

            AActor* NewActorTemplate = NewObject<AActor>(GetTransientPackage(), ClassTemplate, NAME_None,
                                                         RF_ArchetypeObject | RF_Transactional | RF_Public,
                                                         InTemplate);

            if (ActorTemplate) {
                UEngine::CopyPropertiesForUnrelatedObjects(ActorTemplate, NewActorTemplate);

                ActorTemplate->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors);
            }

            ActorTemplate = NewActorTemplate;

            // Record initial object state in case we're in a transaction context.
            ActorTemplate->Modify();

            // Now set the actual name and outer to the BPGC.
            const FString TemplateName = FString::Printf(TEXT("%s_%s_CAT"), *GetName(), *ClassTemplate->GetName());
            FName UniqueTemplateName = MakeUniqueObjectName(this, ActorTemplate->GetClass(), *TemplateName);
            
            ActorTemplate->Rename(*UniqueTemplateName.ToString(), this,
                                  REN_DoNotDirty | REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
        }
    }
    else {
        if (ActorTemplate) {
            Modify();
            ActorTemplate->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors);
            ActorTemplate = nullptr;
        }
    }
}

void UEdGraphNode_DungeonThemeActorTemplate::SetTemplateFromAsset(UObject* AssetObject, UActorFactory* ActorFactory) {
    if (AssetObject && ActorFactory) {
        const FAssetData AssetData(AssetObject);
        SetTemplateClass(ActorFactory->GetDefaultActorClass(AssetData));
        if (ActorTemplate) {
            const TSubclassOf<UActorComponent> AssetPrimaryComponentClass = FComponentAssetBrokerage::GetPrimaryComponentForAsset(AssetObject->GetClass());
            if (UActorComponent* AssetPrimaryComponent = ActorTemplate->GetComponentByClass(AssetPrimaryComponentClass)) {
                FComponentAssetBrokerage::AssignAssetToComponent(AssetPrimaryComponent, AssetObject);
            }
        }
    }
    else {
        ActorTemplate = nullptr;
        ClassTemplate = nullptr;
    }
}

UObject* UEdGraphNode_DungeonThemeActorTemplate::GetNodeAssetObject(UObject* Outer) const {
    UDungeonActorTemplate* TemplateObject = NewObject<UDungeonActorTemplate>(Outer);

    AActor* NewActorTemplate = nullptr;
    if (ClassTemplate) {
        // Clone the actor template 
        NewActorTemplate = NewObject<AActor>(TemplateObject, ClassTemplate, NAME_None,
                                             RF_ArchetypeObject | RF_Transactional | RF_Public);

        if (ActorTemplate) {
            UEngine::CopyPropertiesForUnrelatedObjects(ActorTemplate, NewActorTemplate);
        }
    }

    TemplateObject->ActorTemplate = NewActorTemplate;
    TemplateObject->ClassTemplate = ClassTemplate;
    return TemplateObject;
}
 
TArray<UObject*> UEdGraphNode_DungeonThemeActorTemplate::GetThumbnailAssetObjects() const {
    if (ActorTemplate) {
        UActorComponent* RootComponent = ActorTemplate->GetRootComponent();
        if (RootComponent) {
            UObject* AssetObject = FComponentAssetBrokerage::GetAssetFromComponent(RootComponent);
            if (AssetObject) {
                return { AssetObject };
            }
        }
        return { ActorTemplate->GetClass() };
    }
    return { ClassTemplate };
}

