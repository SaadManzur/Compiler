#pragma once
#ifndef _COMMON_H_
#define _COMMON_H_

#include <vector>
#include <map>
#include <string>
#include<unordered_map>
#include <set>
#include "Logger.h"
#include <set>
#endif

#define MAXOPERANDLENGTH 3

enum Tokens
{
	errorToken = 0,
	timesToken = 1,
	divToken = 2,

	plusToken = 11,
	minusToken = 12,

	eqlToken = 20,
	neqToken = 21,
	lssToken = 22,
	geqToken = 23,
	leqToken = 24,
	gtrToken = 25,

	periodToken = 30,
	commaToken = 31,
	openBracketToken = 32,
	closeBracketToken = 34,
	closeParenToken = 35,

	becomesToken = 40,
	thenToken = 41,
	doToken = 42,
	
	openParenToken = 50,
	numberToken = 60,
	identToken = 61,
	
	semiToken = 70,

	endToken = 80,
	odToken = 81,
	fiToken = 82,

	elseToken = 90,

	letToken = 100,
	callToken = 101,
	ifToken = 102,
	whileToken = 103,
	returnToken = 104,

	varToken = 110,
	arrToken = 111,
	funcToken = 112,
	procToken = 113,

	beginToken = 150,
	mainToken = 200,
	eofToken = 255
};

class SyntaxException : public std::exception
{
private:
	string message;
	int lineNumber;
	int colNumber;

public:
	SyntaxException();
	SyntaxException(string message);
	SyntaxException(int lineNumber, int colNumber, string message = "Syntax error");

	string getMessage();
};

class IntermediateCode {
public:
	int address;
	string opcode;
	int iOpcode;
	string operand[MAXOPERANDLENGTH];
	long long version[MAXOPERANDLENGTH];
	string operandType[MAXOPERANDLENGTH];
	int registers[MAXOPERANDLENGTH];
	int addressRegister;
	int previousSameOp;
	int blockId;

	string getOperandRepresentation(int index);
	string getImmediateAddressRepresentation();
};

class Result {
public: 
	string kind;  //const, var, reg, condition , IntermediateCode, array
	int value;   // value if it is a constant
	int address;  //address if it is a variable/IntermediateCode
	int regno;    // register number if it is a reg or a condition
	int cond, fixupLocation;  // if it is a condition
	int isGlobal=-1;  
};

class BasicBlock
{
	static int blockSerialNumber;
public:
	int id;
	bool liveRangeGenerated = false;
	bool isLoopHeader = false;
	bool isJoinBlock = false;
	bool loopPhiProcessed = false;
	int loopCounter = 0;
	vector<int> instructionAddrList;
	set<string> alive;
	set<string> phiAliveFromLeft;
	set<string> phiAliveFromRight;
	vector<BasicBlock *> dominates;
	BasicBlock* dominatedBy;
	vector<BasicBlock *> next;
	vector<BasicBlock *> back;
	void addInstruction(IntermediateCode instr);
	void addInstructionInBegining(IntermediateCode instr);
	void addInstructionAtPosition(IntermediateCode instr, int i);
	void removeInstruction(IntermediateCode instruction);
	BasicBlock();
};

class Scope
{
public:
	BasicBlock * root;
	string functionName;
	int functionType;   // 0 for proc and 1 for func
	int numOfArg=0;  
	std::vector<std::string> variableList;
	std::vector<int> versionTable;
	std::vector<std::string> arrayList;
	std::vector< std::vector<int> > arrayDimensions;
	std::unordered_map<std::string, int> identifierHashMap;
	std::set<int> globalVarsModifies;
	std::set<int> globalVarsUses;
	std::vector<int> arguments;
	std::map<string, int> assignedRegisters;

	void setRegisters(map<string, int> registers);
};

void printIntermediateCode(IntermediateCode instr);

