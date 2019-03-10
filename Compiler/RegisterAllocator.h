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

#define NUMBER_OF_REGISTERS 8

class RegisterAllocator
{
private:
	BasicBlock *outerMostBlock;
	map<string, set<string>> interferenceGraph;
	map<int, string> registers;
	map<string, int> assignedColors;
	vector<Result> aliveValues;
	Parser parser = NULL;

	void generateInterferenceGraph(BasicBlock *root);
	void calculateLiveRange(BasicBlock* node, set<string> alive);
	void calculateLiveRangeForBlock(BasicBlock *node, bool liveRangeGenerated = true);
	void generateEdgeBetween(string variable, set<string> alive);
	void colorGraph();
	set<string> removeNodeFromInterferenceGraph(string node);
	void insertNodeIntoInterferenceGraph(string node, set<string> adjacency);
	string getNodeWithDegreeLessThanN(int n);
	void assignColor(string node);
	
	void fillParentBlocks(BasicBlock *root);
	void printParents(BasicBlock* root, set<BasicBlock*> visited);
	void printInterferenceGraph();
	void printAssignedRegisters();
	bool aExistsInBDominatorTree(BasicBlock *nodeA, BasicBlock *nodeB);
public:
	RegisterAllocator(const Parser &parser);

	void start(BasicBlock *root);
	string getAssignedRegister(string operand);
};