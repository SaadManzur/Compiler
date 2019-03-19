#include "Common.h"
#include<iostream>

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

void BasicBlock::addInstructionAtPosition(IntermediateCode instr, int i)
{
	instructionAddrList.insert(instructionAddrList.begin() + i, instr.address);
}

void BasicBlock::removeInstruction(IntermediateCode instruction)
{
	vector<int>::iterator it = find(instructionAddrList.begin(), instructionAddrList.end(), instruction.address);

	if (it != instructionAddrList.end())
	{
		instructionAddrList.erase(it);
	}
}

int BasicBlock::blockSerialNumber = 0;

BasicBlock::BasicBlock()
{
	id = BasicBlock::blockSerialNumber++;
}

void printIntermediateCode(IntermediateCode instr)
{
	cout << instr.address << ' ' << ':' << ' ';
	cout << instr.opcode;
	for (int i = 0; i < MAXOPERANDLENGTH && (instr.operand[i].length() > 0 || instr.operandType->length() > 0); i++)
	{
		if (instr.operandType[i].compare("JumpAddr") == 0)
		{
			BasicBlock * nextBlock = (BasicBlock *)(instr.version[i]);
			instr.operand[i] = to_string(nextBlock->id);
		}
		if(instr.operandType[i].compare("IntermediateCode")==0)
			instr.operand[i]= "(" + to_string(instr.version[i]) + ")";
		cout << ' ' << instr.operand[i];
		if (instr.operandType[i].compare("var") == 0)
			cout << '_' << instr.version[i];
	}
	cout << endl;
}

string IntermediateCode::getOperandRepresentation(int index)
{
	if (operandType[index] == "var")
		return operand[index] + "_" + to_string(version[index]);
	else
		return "(" + to_string(version[index]) + ")";
}

string IntermediateCode::getImmediateAddressRepresentation()
{
	return "(" + to_string(address) + ")";
}

void Scope::setRegisters(map<string, int> registers)
{
	assignedRegisters.insert(registers.begin(), registers.end());
}
