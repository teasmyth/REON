// Fill out your copyright notice in the Description page of Project Settings.


#include "Pathfinding/OctreeGraphNode.h"

OctreeGraphNode::OctreeGraphNode()
{
}

OctreeGraphNode::OctreeGraphNode(const FBox& Bounds)
{
	this->Bounds = Bounds;
	F = FLT_MAX;
	G = FLT_MAX;
	H = FLT_MAX;
	CameFrom = nullptr;
	
}

OctreeGraphNode::~OctreeGraphNode()
{
}
