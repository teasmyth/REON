// Fill out your copyright notice in the Description page of Project Settings.


#include "Pathfinding/OctreeGraph.h"

OctreeGraph::OctreeGraph()
{
	
}

OctreeGraph::~OctreeGraph()
{
}

void OctreeGraph::AddNode(OctreeGraphNode* Node)
{
	Nodes.Add(Node);
}

void OctreeGraph::AddEdge(OctreeNode* From, OctreeNode* To)
{
	
}

void OctreeGraph::OctreeAStar(OctreeNode* StartNode, OctreeNode* EndNode, TArray<OctreeNode*>& OutPathList)
{
	
}

void OctreeGraph::ReconstructPath(OctreeGraphNode Start, OctreeGraphNode End, TArray<OctreeGraphNode*> OutPathList)
{
	
}



