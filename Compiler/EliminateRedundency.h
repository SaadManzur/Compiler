#pragma once
#include "common.h"
#include "Parser.h"

#define NUMOFSUBEXPRESSION 6

enum Subexpression
{
	add,
	sub,
	mul,
	divide,
	cmp,
	load
};

class EliminateRedundency
{
	vector<IntermediateCode> intermediateCodelist;
	Parser *parser;
	//BasicBlock *root;
	Scope *global;
	vector<Scope *> functions;
	std::set<BasicBlock *> visitedNodes;
	vector<int> subexpressionPointer;
	void eliminateCopies(BasicBlock *cfgNode);
	int checkMatch(int a, int b, int subexprType);
	int matchOperands(int adr1, int opr1, int adr2, int opr2);
public:
	EliminateRedundency(Parser *parser);
	void copyPropagation(BasicBlock *cfgNode =NULL);
	void updateVersion(BasicBlock *cfgNode = NULL);
	void updateVersion(int addr);
	void CSE(BasicBlock *cfgNode = NULL);
	void searchCommonSubexpression(int addr);
	int getPointedInstructionAddr(int addr);
//	void copyCFG(BasicBlock *root);
	void printCodesByBlocks(BasicBlock *cfgNode = NULL);
	void outputVCGFile(BasicBlock *cfgNode = NULL);
	IntermediateCode getIntermediateCode(int address);
	vector<IntermediateCode> getIntermediateCodeList();
	~EliminateRedundency();
	vector<Scope *> getFunctions();
	Scope* getGlobalScope();
};

