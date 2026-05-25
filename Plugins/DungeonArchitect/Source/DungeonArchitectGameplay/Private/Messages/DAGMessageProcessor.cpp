//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Messages/DAGMessageProcessor.h"

#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(DAGMessageProcessor)

void UDAG_MessageProcessor::BeginPlay()
{
	Super::BeginPlay();

	StartListening();
}

void UDAG_MessageProcessor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	StopListening();

	// Remove any listener handles
	UDAG_MessageSubsystem& MessageSubsystem = UDAG_MessageSubsystem::Get(this);
	for (FDAG_MessageListenerHandle& Handle : ListenerHandles)
	{
		MessageSubsystem.UnregisterListener(Handle);
	}
	ListenerHandles.Empty();
}

void UDAG_MessageProcessor::StartListening()
{

}

void UDAG_MessageProcessor::StopListening()
{
}

void UDAG_MessageProcessor::AddListenerHandle(FDAG_MessageListenerHandle&& Handle)
{
	ListenerHandles.Add(MoveTemp(Handle));
}

double UDAG_MessageProcessor::GetServerTime() const
{
	if (AGameStateBase* GameState = GetWorld()->GetGameState())
	{
		return GameState->GetServerWorldTimeSeconds();
	}
	else
	{
		return 0.0;
	}
}


