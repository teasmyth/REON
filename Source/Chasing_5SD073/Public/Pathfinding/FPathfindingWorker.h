// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OctreeNode.h"

/**
 * 
 */
class CHASING_5SD073_API FPathfindingWorker : public FRunnable
{
public:

	FPathfindingWorker(const TWeakPtr<OctreeNode>& InOctreeNode, bool& InDebug, const TArray<FBox>& InActorBoxes, const float InMinSize) : OctreeRootNode(InOctreeNode), ActorBoxes(InActorBoxes), MinSize(InMinSize), Debug(InDebug)
	{
		Thread = FRunnableThread::Create(this, TEXT("PathfindingThread"));
	}

	virtual ~FPathfindingWorker() override
	{
		bRunThread = false;
		
		if (Thread)
		{
			// Kill() is a blocking call, it waits for the thread to finish.
			Thread->WaitForCompletion();
			Thread->Kill();
			delete Thread;
		}
	}

	virtual uint32 Run() override;

	TArray<FVector> GetPathPoints() const
	{
		return PathPoints;
	}


	bool GetFoundPath() const
	{
		return PathFound;
	}

	bool IsItWorking() const
	{
		return IsWorking && bRunThread;
	}

	virtual void Stop() override;

	void ContinueThread();
	void PauseThread();


	/// @param Task of FVector, FVector where the first FVector is the start location and the second is the end location.
	/// @param MoveOnToNextTask if true, the thread will start working on the task immediately.
	void AddToQueue(const TPair<FVector, FVector>& Task, const bool MoveOnToNextTask = false);
	//Returns the oldest task's results.
	TArray<FVector> GetOutQueue();

private:
	bool ThreadIsPaused = false;
	FRunnableThread* Thread;
	TWeakPtr<OctreeNode> OctreeRootNode;
	TQueue<TPair<FVector, FVector>> TaskQueue;
	TArray<FVector> PathPoints;
	
	TArray<FBox> ActorBoxes;
	float MinSize;
	
	bool bRunThread = true;
	bool PathFound = false;
	bool IsWorking = false;
	bool& Debug;
};
