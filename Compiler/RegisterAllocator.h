#pragma once
#ifndef _REGISTER_ALLOCATOR
#define _REGISTER_ALLOCATOR

#include "Common.h"
#include "Parser.h"
#include <set>
#include <queue>
#include <iostream>
#endif

class RegisterAllocator
{
private:
	BasicBlock *outerMostBlock;

	bool registerInUse[8];
	map<string, set<string>> interferenceGraph;
	vector<Result> aliveValues;
	Parser parser = NULL;

	void generateInterferenceGraph(BasicBlock *root);
	set<string> calculateLiveRange(BasicBlock* node, set<string> alive);
	set<string> calculateLiveRangeForBlock(BasicBlock *node, set<string> alive, bool liveRangeGenerated = true);
	void generateEdgeBetween(string variable, set<string> alive);
	BasicBlock *getOuterMostBlock(BasicBlock *root);
	
	void fillParentBlocks(BasicBlock *root);
	void printParents(BasicBlock* root, set<BasicBlock*> visited);
	void printInterferenceGraph();
public:
	RegisterAllocator(const Parser &parser);

	void start(BasicBlock *root);
};