// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/FPathfindingWorker.h"
#include "Pathfinding/OctreeGraph.h"


uint32 FPathfindingWorker::Run()
{
	while (bRunThread)
	{
		TPair<FVector, FVector> Task;
		//I used to check validity of OctreeRootNode here, but it's not necessary since it's a weak pointer.
		//Under normal circumstances, the OctreeRootNode should always be valid.
		//Dequeue will return false if the queue is empty.
		while (!FinishedWork && TaskQueue.Dequeue(Task))
		{
			//IsPathfindingInProgress = true;
			PathFound = OctreeGraph::OctreeAStar(Task.Key, Task.Value, OctreeRootNode.Pin(), PathPoints);
			FPlatformProcess::Sleep(0.01f);
			FinishedWork = true;
		}
	}
	return 0;
}

void FPathfindingWorker::Stop()
{
	FRunnable::Stop();

	bRunThread = false;
}

void FPathfindingWorker::AddToQueue(const TPair<FVector, FVector>& Task, const bool MoveOnToNextTask)
{
	TaskQueue.Enqueue(Task);
	
	if (MoveOnToNextTask)
	{
		//IsPathfindingInProgress = false;
		FinishedWork = false;
	}
}

TArray<FVector> FPathfindingWorker::GetOutQueue()
{
	TArray<FVector> OutPathPoints = PathPoints;
	PathPoints.Empty();
	
	return OutPathPoints;
}
