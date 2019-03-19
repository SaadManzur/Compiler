#pragma once
#ifndef _REGISTER_ALLOCATOR
#define _REGISTER_ALLOCATOR

#include "Common.h"
#include "EliminateRedundency.h"
#include <set>
#include <algorithm>
#include <iterator>
#include <queue>
#include <iostream>
#endif

#define NUMBER_OF_REGISTERS 8
#define VIRTUAL_REGISTER_OFFSET 100

class RegisterAllocator
{
private:
	int lastVirtualRegisterNumber = VIRTUAL_REGISTER_OFFSET;
	BasicBlock *outerMostBlock;
	vector<IntermediateCode> phiInstructions;
	vector<IntermediateCode> instructionsToBeEliminated;
	map<string, set<string>> interferenceGraph;
	map<string, set<string>> clusters;
	map<string, int> cost;
	map<int, BasicBlock*> instructionBlocks;
	map<int, string> registers;
	map<string, int> assignedColors;
	vector<Result> aliveValues;
	EliminateRedundency *redundancyEliminator = NULL;
	int currentCodeAddress;
	set<BasicBlock*> visited;

	void generateInterferenceGraph(BasicBlock *root);
	void calculateLiveRange(BasicBlock* node, set<string> alive, int depth = 1);
	void calculateLiveRangeForBlock(BasicBlock *node, int depth = 1, bool liveRangeGenerated = true);
	void generateEdgeBetween(string variable, set<string> alive);
	void colorGraph();
	set<string> removeNodeFromInterferenceGraph(string node);
	void insertNodeIntoInterferenceGraph(string node, set<string> adjacency);
	string getNodeWithDegreeLessThanN();
	void assignColor(string node);
	string spillRegisterAndGetNode();
	void coalsceLiveRanges();
	void replaceNodeWithCluster(string node, string clusterId, set<string> edges);
	void applyClusterColor();
	void eliminatePhi();
	IntermediateCode createMoveInstruction(IntermediateCode *instruction, int i, int j);
	int getFirstBranchingWithinBlock(BasicBlock *block);
	void addInstructionToParent(IntermediateCode instruction, BasicBlock *parent);
	
	void fillParentBlocks(BasicBlock *root);
	void printParents(BasicBlock* root, set<BasicBlock*> visited);
	void printInterferenceGraph();
	void printAssignedRegisters();
	void printClusters();
	bool aExistsInBDominatorTree(BasicBlock *nodeA, BasicBlock *nodeB);
	int tryMerge(string c1, string c2);
	int mergeClusters();
	string getClusterName(string x);
	void fillParentBlocksDFS(BasicBlock *root);

public:
	RegisterAllocator(EliminateRedundency *redundancyEliminator, int currentCodeAddress=0);

	string getAssignedRegister(string operand);
	map<string, int> getAllAssignedRegisters();
	vector<IntermediateCode> getInstructionsToBeEliminated();
	int getCurrentCodeAddress();

	void start(BasicBlock *root);
};