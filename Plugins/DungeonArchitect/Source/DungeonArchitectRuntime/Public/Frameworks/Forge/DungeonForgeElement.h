//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Frameworks/Forge/Graph/Execution/DungeonForgeGraphTaskInstance.h"

class ADungeon;
struct FDungeonForgeContext;
class UDungeonForgeGraphNode;
struct FDungeonForgeDataCollection;
class UDungeonForgeNodeSettings;

typedef TSharedPtr<class IDungeonForgeElement, ESPMode::ThreadSafe> FDungeonForgeElementPtr;

namespace EDungeonForgeElementLogMode
{
	enum Type : uint8
	{
		/** Output to log. */
		LogOnly = 0,
		/** Display errors/warnings on graph as well as writing to log. */
		GraphAndLog,

		NumLogModes,

		// Used below in log macros to silence PVS by making the log mode comparison 'look' non-trivial by adding a trivial mask op (an identical
		// mechanism as the one employed in the macro UE_ASYNC_PACKAGE_LOG in AsyncLoading2.cpp).
		// Warning V501: There are identical sub-expressions 'EDungeonForgeElementLogMode::GraphAndLog' to the left and to the right of the '==' operator.
		// The warning disable comment can can't be used in a macro: //-V501 
		LogModeMask = 0xff
	};
};


#define DFE_LOG_BASE(Verbosity, CustomContext, Message) \
UE_LOG(LogDF, \
Verbosity, \
TEXT("[%s - %s]: %s"), \
(CustomContext) ? *((CustomContext)->GetComponentName()) : TEXT("UnknownComponent"), \
(CustomContext) ? *((CustomContext)->GetTaskName()) : TEXT("UnknownTask"), \
*Message.ToString())

#if WITH_EDITOR
// Output to Dungeon Forge log and optionally also display warnings/errors on graph.
#define DFE_LOG(Verbosity, LogMode, Message) do { \
if (((EDungeonForgeElementLogMode::LogMode) & EDungeonForgeElementLogMode::LogModeMask) == EDungeonForgeElementLogMode::GraphAndLog && (Context)) { (Context)->LogVisual(ELogVerbosity::Verbosity, Message); } \
if (ShouldLog()) { DFE_LOG_BASE(Verbosity, Context, Message); } \
} while (0)
// Output to DF log and optionally also display warnings/errors on graph. Takes context as argument.
#define DFE_LOG_C(Verbosity, LogMode, CustomContext, Message) do { \
if (((EDungeonForgeElementLogMode::LogMode) & EDungeonForgeElementLogMode::LogModeMask) == EDungeonForgeElementLogMode::GraphAndLog && (CustomContext)) { (CustomContext)->LogVisual(ELogVerbosity::Verbosity, Message); } \
DFE_LOG_BASE(Verbosity, CustomContext, Message); \
} while (0)
#else
#define DFE_LOG(Verbosity, LogMode, Message) DFE_LOG_BASE(Verbosity, Context, Message)
#define DFE_LOG_C(Verbosity, LogMode, CustomContext, Message) DFE_LOG_BASE(Verbosity, CustomContext, Message)
#endif


class IDungeonForgeElement {
public:
	virtual ~IDungeonForgeElement() = default;
	
	/** Creates a custom context object paired to this element */
	virtual FDungeonForgeContextPtr Initialize(const FDungeonForgeDataCollection& InputData, const UDungeonForgeGraphNode* Node, const TWeakPtr<const FDungeonForgeGraphExecutionState>& InExecutionState);
	
	/** Returns true if the node can be cached (e.g. does not create artifacts & does not depend on untracked data */
	virtual bool IsCacheable(const UDungeonForgeNodeSettings* InSettings) const { return true; }
	
	/**
	 * Public function that executes the element on the appropriately created context.
	 * The caller should call the Execute function until it returns true.
	 */
	bool Execute(const FDungeonForgeContextPtr& Context) const;
	
	/** The caller should call the Tick function until it returns true. */
	bool Tick(const FDungeonForgeContextPtr& Context, float DeltaTime) const;
	
	/** Public function called when an element is cancelled, passing its current context if any. */
	void Abort(const FDungeonForgeContextPtr& Context) const;

protected:
	/** Let each element optionally act as a concrete factory for its own context */
	virtual FDungeonForgeContextPtr CreateContext();
	virtual bool ExecuteImpl(const FDungeonForgeContextPtr& Context) const;
	virtual bool TickImpl(const FDungeonForgeContextPtr& Context, float DeltaTime) const;
	virtual void AbortImpl(const FDungeonForgeContextPtr& Context) const;
};


