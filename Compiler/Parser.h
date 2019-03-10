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

//	std::vector<string> cachedIdentifierList;
	vector<int> cachedVersionTable;
//	unordered_map<string, int> cachedIdentifierHashMap;

	Scope *global;
	vector<Scope *> functions;
	Scope *currentScope;

	vector<string> functionCalls;
	//indicates whether current statement is in else block or not
	int phiFlag;   // 1 means ifBlock, 2 elseBlock, 3 whileBlock, 0 none
	int whileStartAddr; 
	
	int getInScopeID(int id);    //get the id number of current identifier token in current scope
	void assignment();
	Result funcCall();
	void ifStatement();
	void whileStatement();
	void returnStatement();

	void statement();
	void statSequence();

	vector<int> typeDecl();
	void varDecl();
	void funcDecl();
	void formalParam();
	void funcBody();

	void computation();

	Result createAndAddCode(int op, Result x, Result y);
	Result createAndAddCode(string opcode, string x, string y);
	Result createAndAddCode(string opcode, Result x, Result y);

	Result InputNum();
	void OutputNum(Result x);
	void updateScope(vector<int> &dimension, Result &x);
	Result accessArray(vector<Result> &dimension, Result x);
	void determineType(Result &x);  // updateType if array

	void OutputNewLine();
	Result compute(int op, Result x, Result y);
	BasicBlock *root;
	BasicBlock *currentBlock;
	stack<BasicBlock *> joinBlockStack;

	int getVersion(int id);
	void updateVersion(int id, int version);
	string id2String(int id);

	void storeOldCachedVersion(vector<int> &versionTable);
	void loadOldCachedVersion(vector<int> &versionTable);

	void cacheVersionTable();
	void restoreVersionTableFromCache();
//	int isFunction(string identifier);
	int functionNametoScopeId(string func);
	void secondPass(BasicBlock *cfgNode = NULL);
public:
	Parser(Scanner *scanner);
	void Parse();
	IntermediateCode createIntermediateCode(int op, Result x, Result y);
	IntermediateCode createIntermediateCode(string opcode, Result x, Result y);
	void printAllIntermediateCode();
	void updatePhi(Result x);
	void renameLoopOccurances(Result x, int newVersion);
	void commitPhi(BasicBlock *joinBlock);
	void printCodesByBlocks(BasicBlock *cfgNode=NULL);
	void outputVCGFile(BasicBlock *cfgNode = NULL);
	void outputDominatorTree(BasicBlock *cfgNode = NULL);
	vector<IntermediateCode>& getIntermediateCodelist();
	BasicBlock *getCFGTreeRoot();
};