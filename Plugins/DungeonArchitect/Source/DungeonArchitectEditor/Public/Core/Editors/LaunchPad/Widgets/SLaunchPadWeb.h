//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FWebNavigationRequest;
class SWebBrowser;

class SLaunchPadWeb : public SCompoundWidget {
public:
    SLATE_BEGIN_ARGS(SLaunchPadWeb) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);
    static bool HandleBeforeNavigation(const FString& Url, const FWebNavigationRequest& Request);
    
private:
    TSharedPtr<SWebBrowser> WebBrowser;
};