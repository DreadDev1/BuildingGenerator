//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Core/Editors/LaunchPad/Widgets/SLaunchPadWeb.h"

#include "Core/Editors/LaunchPad/Actions/Impl/LaunchPadActionsImpl.h"

#include "GenericPlatform/GenericPlatformHttp.h"
#include "Interfaces/IPluginManager.h"
#include "SWebBrowser.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

DEFINE_LOG_CATEGORY_STATIC(LogLaunchPadWeb, Log, All);

namespace LaunchPadHelpers {

	FString GetLocalWebsiteURL() {
		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("DungeonArchitect"));
		if (!Plugin.IsValid()) {
			return TEXT(""); 
		}

		FString RelDir = Plugin->GetBaseDir();
		FString FullBaseDir = FPaths::ConvertRelativePathToFull(RelDir);
		FString WebsitePath = FPaths::Combine(FullBaseDir, TEXT("Resources/LaunchPad/website/index.html"));
		FPaths::NormalizeFilename(WebsitePath);
		FString FileURL = FString::Printf(TEXT("file:///%s"), *WebsitePath);
		return FileURL;
	}
}

void SLaunchPadWeb::Construct(const FArguments& Args) {
	const FString URL = LaunchPadHelpers::GetLocalWebsiteURL();
	UE_LOG(LogLaunchPadWeb, Log, TEXT("[v5] LaunchPad Web URL: %s"), *URL);
	
    ChildSlot
    [ 
        SAssignNew(WebBrowser, SWebBrowser) 
        .InitialURL(URL)
        .ShowControls(false)
        .ShowAddressBar(false)
        .ShowErrorMessage(true)
        .BrowserFrameRate(60)
        .SupportsTransparency(true)
		.PopupMenuMethod(EPopupMethod::UseCurrentWindow)
	    .OnBeforeNavigation_Static(&SLaunchPadWeb::HandleBeforeNavigation)
    ];
}

bool SLaunchPadWeb::HandleBeforeNavigation(const FString& Url, const FWebNavigationRequest& Request) {
	FString Command = FGenericPlatformHttp::GetUrlDomain(Url);
	UE_LOG(LogTemp, Log, TEXT("Command: %s"), *Command);

	if (TOptional<FString> PathParam = FGenericPlatformHttp::GetUrlParameter(Url, TEXT("path"))) {
		FString Path = MoveTemp(PathParam.GetValue());
		UE_LOG(LogTemp, Log, TEXT("Path: %s"), *Path);

		if (Command == TEXT("FolderView")) {
			FLaunchPadActions::Exec_OpenFolder(Path);
		}
		else if (Command == TEXT("Video")) {
			FLaunchPadActions::Exec_Video(Path);
		}
		else if (Command == TEXT("Documentation")) {
			FLaunchPadActions::Exec_Documentation(Path);
		}
		else if (Command == TEXT("CloneSceneAndBuild")) {
			FLaunchPadActions::Exec_CloneSceneAndBuild(Path);
		}
		else if (Command == TEXT("CloneScene")) {
			FLaunchPadActions::Exec_CloneScene(Path);
		}
		else if (Command == TEXT("LauncherURL")) {
			FLaunchPadActions::Exec_LauncherURL(Path);
		}
		else if (Command == TEXT("OpenURL")) {
			FLaunchPadActions::Exec_OpenURL(Path);
		}
		else if (Command == TEXT("News")) {
			FLaunchPadActions::Exec_OpenURL(Path);
		}
	}

	return true;
}

