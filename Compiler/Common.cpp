#include "Common.h"

SyntaxException::SyntaxException()
{
	this->message = "Syntax error";
}

SyntaxException::SyntaxException(string message)
{
	this->message = message;
}

SyntaxException::SyntaxException(int lineNumber, int colNumber, string message)
{
	this->message = message;
	this->lineNumber = lineNumber;
	this->colNumber = colNumber;
}

string SyntaxException::getMessage()
{
	string message = this->message + " at[" + to_string(lineNumber) + "," + to_string(colNumber) + "]";

	return message;
}

void BasicBlock::addInstruction(IntermediateCode instr)
{
	instructionAddrList.push_back(instr.address);
}

void BasicBlock::addInstructionInBegining(IntermediateCode instr)
{
	instructionAddrList.insert(instructionAddrList.begin(), instr.address);
}

int BasicBlock::blockSerialNumber = 0;

BasicBlock::BasicBlock()
{
	id = BasicBlock::blockSerialNumber++;
}
