//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "Frameworks/Forge/DungeonForgeElement.h"

#include "Frameworks/Forge/DungeonForgeContext.h"
#include "Frameworks/Forge/Graph/DungeonForgeGraphNode.h"

FDungeonForgeContextPtr IDungeonForgeElement::Initialize(const FDungeonForgeDataCollection& InputData, const UDungeonForgeGraphNode* Node, const TWeakPtr<const FDungeonForgeGraphExecutionState>& InExecutionState) {
	FDungeonForgeContextPtr Context = CreateContext();
	check (Context.IsValid());
	Context->InputData = InputData;
	Context->ExecutionState = InExecutionState;
	Context->Node = Node;

	return Context; 
}

bool IDungeonForgeElement::Execute(const FDungeonForgeContextPtr& Context) const {
	return ExecuteImpl(Context);
}

bool IDungeonForgeElement::Tick(const FDungeonForgeContextPtr& Context, float DeltaTime) const {
	return TickImpl(Context, DeltaTime);
}

void IDungeonForgeElement::Abort(const FDungeonForgeContextPtr& Context) const {
	AbortImpl(Context);
}

FDungeonForgeContextPtr IDungeonForgeElement::CreateContext() {
	return MakeShared<FDungeonForgeContext, ESPMode::ThreadSafe>();
}

bool IDungeonForgeElement::ExecuteImpl(const FDungeonForgeContextPtr& Context) const {
	return true;
}

bool IDungeonForgeElement::TickImpl(const FDungeonForgeContextPtr& Context, float DeltaTime) const {
	return true;
}

void IDungeonForgeElement::AbortImpl(const FDungeonForgeContextPtr& Context) const {
}
