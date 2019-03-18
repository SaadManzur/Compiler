#pragma once
#ifndef _CODE_GENERATOR_H_
#define _CODE_GENERATOR_H_

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
	vector<unsigned int> targetCodes;

	void generateControlFlow();
	void processBranchingInstructions();
	void processBlockLastInstructionIfBranching(int address);
	void integrateRegisterWithIntermediateCodes(Scope *currentScope);
	void generateCode();
	void generateCodeForInstruction(IntermediateCode instruction);

	int getAssignmentCode(IntermediateCode instruction);
	int getMathCode(IntermediateCode instruction, int opcode);
	int getBranchCode(IntermediateCode instruction);
	int getBranchCode(IntermediateCode instruction, int opcode);

	void printControlFlow(Scope *currentScope);
	void printIntermediateCodes(BasicBlock *block, Scope *currentScope);
	void printIntermediateCode(IntermediateCode instruction, Scope *currentScope);
	void printIntermediateTargetCode(IntermediateCode instruction, Scope *currentScope);
	void printTargetCode(bool verbose = true);
	string getRegisterIfAssigned(string operand, Scope *currentScope);
public:
	CodeGenerator(Scope *mainScope, vector<Scope *> functions, vector<IntermediateCode> intermediateCodeList);

	void generate();
};