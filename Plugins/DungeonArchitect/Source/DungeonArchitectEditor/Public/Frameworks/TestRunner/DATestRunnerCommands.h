//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class DUNGEONARCHITECTEDITOR_API FDATestRunnerCommands : public TCommands<FDATestRunnerCommands> {
public:
	FDATestRunnerCommands();
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> StartStopPerfRunner;
};

