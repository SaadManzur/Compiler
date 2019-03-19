#pragma once
#ifndef _CODE_GENERATOR_H_
#define _CODE_GENERATOR_H_

#define SP 29
#define FP 28
#define RP1 9
#define RP2 10
#define SP_OFFSET 4

#include <iostream>
#include <vector>
#include <stack>
#include <set>
#include <algorithm>
#include "Common.h"
#include "DLXProcessor.h"

#endif

class CodeGenerator
{
private:
	DLXProcessor dlxProcessor;

	Scope *mainScope;
	vector<Scope *> functions;
	vector<BasicBlock*> controlFlow;
	vector<IntermediateCode> intermediateCodeList;
	vector<IntermediateCode> intermediateTargetCodeCandidates;
	map<int, int> blockFirstInstructionAddress;
	map<string, int> scopeStartingAddressInMemory;
	map<string, Scope*> scopeNameMap;
	map<int, IntermediateCode> branchInstructionsPending;
	map<string, int> arrayBaseAddress;
	vector<unsigned int> targetCodes;
	int currentCodeAddress;
	bool dummyPushed = false;

	void generateControlFlow(Scope *currentScope);
	void processBranchingInstructions(int fromAddress=0);
	void processBlockLastInstructionIfBranching(int address, int offset);
	void integrateRegisterWithIntermediateCodes(Scope *currentScope);
	void generateCode();
	void generateCodeForInstruction(IntermediateCode instruction);
	void initialize();
	void prologue(int N = 4);
	void epilogue(int M);
	void backupRegisters(Scope *currentScope);
	void restoreRegisters(Scope *currentScope);

	int getAssignmentCode(IntermediateCode instruction);
	int getMathCode(IntermediateCode instruction, int opcode);
	int getBranchCode(IntermediateCode instruction);
	int getBranchCode(IntermediateCode instruction, int opcode);

	void printControlFlow(Scope *currentScope);
	void addIntermediateCodes(BasicBlock *block, Scope *currentScope);
	void printIntermediateCode(IntermediateCode instruction, Scope *currentScope);
	void printIntermediateTargetCode(IntermediateCode instruction, Scope *currentScope);
	void printTargetCode(bool verbose = true);
	string getRegisterIfAssigned(string operand, Scope *currentScope);
	IntermediateCode createInstruction(string opcode, string operands[MAXOPERANDLENGTH], int versions[MAXOPERANDLENGTH]);
public:
	CodeGenerator(Scope *mainScope, vector<Scope *> functions, vector<IntermediateCode> intermediateCodeList, int currentCodeAddress=0);

	void generate();
	void execute();
};