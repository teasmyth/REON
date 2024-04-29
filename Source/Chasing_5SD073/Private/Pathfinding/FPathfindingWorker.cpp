// Fill out your copyright notice in the Description page of Project Settings.

#include "Pathfinding/FPathfindingWorker.h"
#include "Pathfinding/OctreeGraph.h"


uint32 FPathfindingWorker::Run()
{
	while (bRunThread)
	{
		if (ThreadIsPaused) continue;
		
		TPair<FVector, FVector> Task;
		//Dequeue will return false if the queue is empty.
		while (IsWorking && TaskQueue.Dequeue(Task))
		{
			PathFound = OctreeGraph::LazyOctreeAStar(ThreadIsPaused, Debug, ActorBoxes, MinSize, Task.Key, Task.Value, OctreeRootNode.Pin(), PathPoints);
			FPlatformProcess::Sleep(0.01f); //I lost the source but read somewhere that a small sleep can help with the flip-flopping of threads.
			IsWorking = false;
		}
	}
	return 0;
}

void FPathfindingWorker::Stop()
{
	FRunnable::Stop();
	bRunThread = false;
	ThreadIsPaused = true;

	TaskQueue.Empty();
	PathPoints.Empty();
	IsWorking = false;
}

void FPathfindingWorker::ContinueThread()
{
	TaskQueue.Empty();
	PathPoints.Empty();
	IsWorking = false;
	ThreadIsPaused = false;
}

void FPathfindingWorker::PauseThread()
{
	ThreadIsPaused = true;
	TaskQueue.Empty();
	PathPoints.Empty();
	IsWorking = false;
	
}

void FPathfindingWorker::AddToQueue(const TPair<FVector, FVector>& Task, const bool MoveOnToNextTask)
{
	TaskQueue.Enqueue(Task);
	
	if (MoveOnToNextTask)
	{
		//IsPathfindingInProgress = false;
		IsWorking = true;
	}
}

TArray<FVector> FPathfindingWorker::GetOutQueue()
{
	TArray<FVector> OutPathPoints = PathPoints;
	PathPoints.Empty();
	
	return OutPathPoints;
}
