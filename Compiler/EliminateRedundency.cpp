#include "EliminateRedundency.h"

EliminateRedundency::EliminateRedundency(Parser *parser)
{
	this->parser = parser;
	this->intermediateCodelist = parser->getIntermediateCodelist();
	copyCFG(parser->getCFGTreeRoot());
	subexpressionPointer = vector<int >(NUMOFSUBEXPRESSION, -1);
}

void EliminateRedundency::copyPropagation(BasicBlock *cfgNode)
{
	if (cfgNode == NULL)
	{
		cfgNode = root;
		visitedNodes.clear();
	}

	visitedNodes.insert(cfgNode);
	eliminateCopies(cfgNode);
	

	for (int i = 0; i < cfgNode->next.size(); i++)
	{
		if (visitedNodes.find(cfgNode->next[i]) == visitedNodes.end())
		{
			copyPropagation(cfgNode->next[i]);
		}

	}
}

void EliminateRedundency::eliminateCopies(BasicBlock * cfgNode)
{
//	IntermediateCode instr;
	int addr;
	string opcode, operandType0;
	for (int i = 0; i < cfgNode->instructionAddrList.size(); i++)
	{
		addr = cfgNode->instructionAddrList[i];
		opcode = intermediateCodelist[addr].opcode;
		operandType0 = intermediateCodelist[addr].operandType[0];
		if (opcode.compare("mov") == 0 && operandType0.compare("const")!=0)
		{
			intermediateCodelist[addr].opcode = "eliminated";
			intermediateCodelist[addr].address = getPointedInstructionAddr(intermediateCodelist[addr].version[0]);
		}
		else
		{
			
		}
	}
}

void EliminateRedundency::updateVersion(BasicBlock * cfgNode)
{
	if (cfgNode == NULL)
	{
		cfgNode = root;
		visitedNodes.clear();
	}

	visitedNodes.insert(cfgNode);

	int addr;
	string kind;
	for (int i = 0; i < cfgNode->instructionAddrList.size(); i++)
	{
		addr = cfgNode->instructionAddrList[i];

		//update versions of operands
		for (int j = 0; j < MAXOPERANDLENGTH; j++)
		{
			kind = intermediateCodelist[addr].operandType[j];
			if (intermediateCodelist[addr].operand[j].size() > 0 && kind.compare("const")!=0) //kind==var or intermediateCode
			{
				intermediateCodelist[addr].version[j] = getPointedInstructionAddr(intermediateCodelist[addr].version[j]);
			}
		}
	}

	for (int i = 0; i < cfgNode->next.size(); i++)
	{
		if (visitedNodes.find(cfgNode->next[i]) == visitedNodes.end())
		{
			updateVersion(cfgNode->next[i]);
		}

	}
}

void EliminateRedundency::updateVersion(int addr)
{
	string kind;
	for (int j = 0; j < MAXOPERANDLENGTH; j++)
	{
		kind = intermediateCodelist[addr].operandType[j];
		if (intermediateCodelist[addr].operand[j].size() > 0 && kind.compare("const") != 0) //kind==var or intermediateCode
		{
			intermediateCodelist[addr].version[j] = getPointedInstructionAddr(intermediateCodelist[addr].version[j]);
		}
	}
}

void EliminateRedundency::CSE(BasicBlock * cfgNode)
{

	if (cfgNode == NULL)
	{
		cfgNode = root;
		visitedNodes.clear();
	}
	vector<int > oldSubexpressinPointer = subexpressionPointer;
	visitedNodes.insert(cfgNode);

	int addr;
	string kind;
	for (int i = 0; i < cfgNode->instructionAddrList.size(); i++)
	{
		addr = cfgNode->instructionAddrList[i];

		//update versions of operands
		updateVersion(addr);
		searchCommonSubexpression(addr);
	}

	for (int i = 0; i < cfgNode->dominates.size(); i++)
	{
		if (visitedNodes.find(cfgNode->dominates[i]) == visitedNodes.end())
		{
			CSE(cfgNode->dominates[i]);
		}
	}
	subexpressionPointer = oldSubexpressinPointer;
}

void EliminateRedundency::searchCommonSubexpression(int addr)
{
	int pointedAddr, tempAddr, subexprType;
	string opcode = intermediateCodelist[addr].opcode;
	if (opcode.compare("add") == 0)
		subexprType = add;
	else if (opcode.compare("sub") == 0)
		subexprType = sub;
	else if (opcode.compare("mul") == 0)
		subexprType = mul;
	else if (opcode.compare("div") == 0)
		subexprType = divide;
	else if (opcode.compare("cmp") == 0)
		subexprType = cmp;
	pointedAddr = subexpressionPointer[subexprType];

	tempAddr = pointedAddr;
	while (tempAddr != -1)
	{
		if (intermediateCodelist[tempAddr].version[0] == intermediateCodelist[addr].version[0] &&
			intermediateCodelist[tempAddr].version[1] == intermediateCodelist[addr].version[1]) //match found
		{
			intermediateCodelist[addr].opcode = "eliminated";
			intermediateCodelist[addr].address= getPointedInstructionAddr(tempAddr);
			break;
		}
		tempAddr = intermediateCodelist[tempAddr].previousSameOp;
	}
	if (tempAddr == -1) //if no match found then update subexpression pointer
	{
		intermediateCodelist[addr].previousSameOp = pointedAddr;
		subexpressionPointer[subexprType] = addr;
	}
}

int EliminateRedundency::getPointedInstructionAddr(int addr)
{
	if (intermediateCodelist[addr].address == addr)
		return addr;
	else
		return intermediateCodelist[addr].address=getPointedInstructionAddr(intermediateCodelist[addr].address);
}

void EliminateRedundency::copyCFG(BasicBlock * root)
{

}

EliminateRedundency::~EliminateRedundency()
{
}
