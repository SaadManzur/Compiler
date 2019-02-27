#pragma once
#ifndef _PARSER_H_
#define _PARSER_H_

#include <iostream>
#include "Common.h"
#include "Scanner.h"
#include <stack>
#include <set>
using namespace std;

#endif

class Parser
{
private:
	vector<IntermediateCode> intermediateCodelist;
	int symbol;
	int currentCodeAddress;
	Scanner *scanner;

	void Next();

	bool letter();
	bool digit();
	int relOp();
	set<BasicBlock *> visitedNodes;
	Result ident();
	Result number();

	Result designator();
	Result factor();
	Result term();
	Result expression();
	IntermediateCode relation();

	std::vector<string> cachedIdentifierList;
	vector<int> cachedVersionTable;
	unordered_map<string, int> cachedIdentifierHashMap;
	//indicates whether current statement is in else block or not
	int phiFlag;   // 1 means ifBlock, 2 elseBlock, 3 whileBlock, 0 none
	int whileStartAddr; 
	

	void assignment();
	void funcCall();
	void ifStatement();
	void whileStatement();
	void returnStatement();

	void statement();
	void statSequence();

	void typeDecl();
	void varDecl();
	void funcDecl();
	void formalParam();
	void funcBody();

	void computation();

	void InputNum();
	void OutputNum(Result x);

	void OutputNewLine();
	Result compute(int op, Result x, Result y);
	BasicBlock *root;
	BasicBlock *currentBlock;
	stack<BasicBlock *> joinBlockStack;
	
public:
	Parser(Scanner *scanner);
	void printIntermediateCode(IntermediateCode instr);
	void Parse();
	IntermediateCode createIntermediateCode(int op, Result x, Result y);
	IntermediateCode createIntermediateCode(string opcode, Result x, Result y);
	void printAllIntermediateCode();
	void updatePhi(Result x);
	void cacheVersionTable();
	void storeOldCachedVersion(vector<string> & identifierList, unordered_map<string, int> &identifierHashMap, vector<int> &versionTable);
	void loadOldCachedVersion(vector<string> & identifierList, unordered_map<string, int> &identifierHashMap, vector<int> &versionTable);
	void restoreVersionTableFromCache();
	void renameLoopOccurances(Result x, int newVersion);
	void commitPhi(BasicBlock *joinBlock);
	void printCodesByBlocks(BasicBlock *cfgNode=NULL);
	void outputVCGFile(BasicBlock *cfgNode = NULL);
	vector<IntermediateCode>& getIntermediateCodelist();
	BasicBlock *getCFGTreeRoot();
};