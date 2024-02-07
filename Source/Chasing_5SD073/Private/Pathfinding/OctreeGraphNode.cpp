// Fill out your copyright notice in the Description page of Project Settings.


#include "Pathfinding/OctreeGraphNode.h"

LLM_DEFINE_TAG(OctreeGraphNode);

OctreeGraphNode::OctreeGraphNode()
{
	LLM_SCOPE_BYTAG(OctreeGraphNode);
	Bounds = FBox();
	F = FLT_MAX;
	G = FLT_MAX;
	H = FLT_MAX;
	CameFrom = nullptr;
	Neighbors.Empty();
}

OctreeGraphNode::OctreeGraphNode(const FBox& Bounds)
{
	LLM_SCOPE_BYTAG(OctreeGraphNode);
	this->Bounds = Bounds;
	F = FLT_MAX;
	G = FLT_MAX;
	H = FLT_MAX;
	CameFrom = nullptr;
	Neighbors.Empty();
	
}

OctreeGraphNode::~OctreeGraphNode()
{
	CameFrom = nullptr;
	//Neighbors.Empty();
}
