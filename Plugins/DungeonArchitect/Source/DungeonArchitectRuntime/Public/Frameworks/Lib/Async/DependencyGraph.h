//$ Copyright 2015-25, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#pragma once
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Tickable.h"

enum class EDADependencyGraphTaskState : uint8 {
	Waiting,
	Running,
	Complete
};

class UWorld;
class FDADependencyGraph;

class DUNGEONARCHITECTRUNTIME_API FDADependencyGraphTask : public TSharedFromThis<FDADependencyGraphTask> {
public:
	virtual ~FDADependencyGraphTask() {}
	virtual void Run() = 0;
	virtual void Abort() {}
	virtual void Tick(float DeltaTime) {}
	
	void AddDependency(const TSharedPtr<FDADependencyGraphTask>& InDependency);
	bool CanStart() const;
	void RequestAbortGraph() const;
	FORCEINLINE bool IsComplete() const { return State == EDADependencyGraphTaskState::Complete; }
	FORCEINLINE EDADependencyGraphTaskState GetState() const { return State; }


private:
	TArray<TWeakPtr<FDADependencyGraphTask>> DependentTasks;

protected:
	EDADependencyGraphTaskState State = EDADependencyGraphTaskState::Waiting;
	FDADependencyGraph* Graph{};
	FString DebugName;
	
	friend FDADependencyGraph;
}; 

class DUNGEONARCHITECTRUNTIME_API FDADependencyGraph : public FTickableGameObject {
public:
	void RegisterTask(const TSharedPtr<FDADependencyGraphTask>& InTask);
	void Start(UWorld* InWorld);
	void Abort();
	void Clear();
	bool IsRunning() const;

	//~ Begin FTickableGameObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return true;}
	virtual bool IsTickableInEditor() const override { return true; }
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual TStatId GetStatId() const;
	//~ End FTickableGameObject Interface

	void GetTasks(EDADependencyGraphTaskState InState, TArray<TSharedPtr<FDADependencyGraphTask>>& OutTasks) const;

private:
	TArray<TSharedPtr<FDADependencyGraphTask>> Tasks;
	TWeakObjectPtr<UWorld> WeakWorld;
};

