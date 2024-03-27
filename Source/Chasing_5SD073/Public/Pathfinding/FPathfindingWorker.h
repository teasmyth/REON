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
	FPathfindingWorker(const TWeakPtr<OctreeNode>& InOctreeRootNode) : OctreeRootNode(InOctreeRootNode)
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
		return  !FinishedWork;
	}

	virtual void Stop() override;


	/// @param Task of FVector, FVector where the first FVector is the start location and the second is the end location.
	void AddToQueue(const TPair<FVector, FVector>& Task, const bool MoveOnToNextTask = false);
	//Returns the oldest task's results.
	TArray<FVector> GetOutQueue();

private:
	FRunnableThread* Thread;
	TWeakPtr<OctreeNode> OctreeRootNode;
	TQueue<TPair<FVector, FVector>> TaskQueue;
	TArray<FVector> PathPoints;
	bool bRunThread = true;
	//bool IsPathfindingInProgress = false;
	bool PathFound = false;
	bool FinishedWork = true;
};
