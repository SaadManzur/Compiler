#pragma once
#include "common.h"
#include "Parser.h"

#define NUMOFSUBEXPRESSION 5

enum Subexpression
{
	add,
	sub,
	mul,
	divide,
	cmp
};

class EliminateRedundency
{
	vector<IntermediateCode> intermediateCodelist;
	Parser *parser;
	BasicBlock *root;
	std::set<BasicBlock *> visitedNodes;
	vector<int> subexpressionPointer;
	void eliminateCopies(BasicBlock *cfgNode);
public:
	EliminateRedundency(Parser *parser);
	void copyPropagation(BasicBlock *cfgNode =NULL);
	void updateVersion(BasicBlock *cfgNode = NULL);
	void updateVersion(int addr);
	void CSE(BasicBlock *cfgNode = NULL);
	void searchCommonSubexpression(int addr);
	int getPointedInstructionAddr(int addr);
	void copyCFG(BasicBlock *root);
	void printCodesByBlocks(BasicBlock *cfgNode = NULL);
	void outputVCGFile(BasicBlock *cfgNode = NULL);
	~EliminateRedundency();
};

