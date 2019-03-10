#pragma once
#ifndef _REGISTER_ALLOCATOR
#define _REGISTER_ALLOCATOR

#include "Common.h"
#include "Parser.h"
#include <set>
#include <algorithm>
#include <iterator>
#include <queue>
#include <iostream>
#endif

class RegisterAllocator
{
private:
	BasicBlock *outerMostBlock;

	bool registerInUse[8];
	map<string, set<string>> interferenceGraph;
	map<string, map<string, bool>> adjacencyGraph;
	vector<Result> aliveValues;
	Parser parser = NULL;

	void generateInterferenceGraph(BasicBlock *root);
	void calculateLiveRange(BasicBlock* node, set<string> alive);
	void calculateLiveRangeForBlock(BasicBlock *node, bool liveRangeGenerated = true);
	void generateEdgeBetween(string variable, set<string> alive);
	
	void fillParentBlocks(BasicBlock *root);
	void printParents(BasicBlock* root, set<BasicBlock*> visited);
	void printInterferenceGraph();
	bool aExistsInBDominatorTree(BasicBlock *nodeA, BasicBlock *nodeB);
public:
	RegisterAllocator(const Parser &parser);

	void start(BasicBlock *root);
};