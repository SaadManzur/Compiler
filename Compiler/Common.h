#pragma once
#ifndef _COMMON_H_
#define _COMMON_H_

#include <vector>
#include <map>
#include <string>
#include "Logger.h"
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
	string operand[MAXOPERANDLENGTH];
	long long version[MAXOPERANDLENGTH];
	string operandType[MAXOPERANDLENGTH];
	int previousSameOp;
};

class Result {
public: 
	string kind;  //const, var, reg, condition , IntermediateCode
	int value;   // value if it is a constant
	int address;  //address if it is a variable/IntermediateCode
	int regno;    // register number if it is a reg or a condition
	int cond, fixupLocation;  // if it is a condition
};

class BasicBlock
{
	static int blockSerialNumber;
public:
	int id;
	vector<int> instructionAddrList;
	vector<BasicBlock *> dominates;
	vector<BasicBlock *> next;
	void addInstruction(IntermediateCode instr);
	void addInstructionInBegining(IntermediateCode instr);
	BasicBlock();
};

void printIntermediateCode(IntermediateCode instr);