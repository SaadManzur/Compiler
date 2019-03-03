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
			if (intermediateCodelist[addr].operand[j].size() > 0 && (kind.compare("IntermediateCode")==0||kind.compare("var")==0))
			{
				int newVersion= getPointedInstructionAddr(intermediateCodelist[addr].version[j]);
				if (intermediateCodelist[addr].version[j] != newVersion)
				{
					intermediateCodelist[addr].version[j] = newVersion;
					intermediateCodelist[addr].operandType[j] = "IntermediateCode";
				}
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
		if (intermediateCodelist[addr].operand[j].size() > 0 && (kind.compare("IntermediateCode") == 0 || kind.compare("var") == 0))
		{
			//intermediateCodelist[addr].version[j] = getPointedInstructionAddr(intermediateCodelist[addr].version[j]);
			int newVersion = getPointedInstructionAddr(intermediateCodelist[addr].version[j]);
			if (intermediateCodelist[addr].version[j] != newVersion)
			{
				intermediateCodelist[addr].version[j] = newVersion;
				intermediateCodelist[addr].operandType[j] = "IntermediateCode";
			}
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
	else
		return;
	pointedAddr = subexpressionPointer[subexprType];

	tempAddr = pointedAddr;
	while (tempAddr != -1)
	{
		if ((intermediateCodelist[tempAddr].version[0] == intermediateCodelist[addr].version[0] &&
			intermediateCodelist[tempAddr].version[1] == intermediateCodelist[addr].version[1])
			|| ( (subexprType==add||subexprType==mul)&& 
				  intermediateCodelist[tempAddr].version[0] == intermediateCodelist[addr].version[1] &&
				   intermediateCodelist[tempAddr].version[1] == intermediateCodelist[addr].version[0] ) ) //match found
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
	if (addr==-1 || intermediateCodelist[addr].address == addr)
		return addr;
	else
		return intermediateCodelist[addr].address=getPointedInstructionAddr(intermediateCodelist[addr].address);
}

void EliminateRedundency::copyCFG(BasicBlock * root)
{
	this->root = root;
}

void EliminateRedundency::printCodesByBlocks(BasicBlock * cfgNode)
{
	if (cfgNode == NULL)
	{
		cfgNode = root;
		visitedNodes.clear();
	}

	visitedNodes.insert(cfgNode);
	cout << "Node : " << cfgNode->id << endl;
	for (int i = 0; i < cfgNode->instructionAddrList.size(); i++)
	{
		printIntermediateCode(intermediateCodelist[cfgNode->instructionAddrList[i]]);
	}
	for (int i = 0; i < cfgNode->next.size(); i++)
	{
		if (visitedNodes.find(cfgNode->next[i]) == visitedNodes.end())
		{
			printCodesByBlocks(cfgNode->next[i]);
		}

	}
}

void EliminateRedundency::outputVCGFile(BasicBlock * cfgNode)
{
	if (cfgNode == NULL)
	{
		cfgNode = root;
		visitedNodes.clear();
		cout << "graph: { title: \"control flow graph\"" << endl;
		cout << "manhattan_edges: yes" << endl;
		cout << "smanhattan_edges: yes" << endl;
	}
	cout << "node: {" << endl;
	cout << "title: \"" << cfgNode->id << "\"" << endl;
	cout << "label: \"" << cfgNode->id << "[" << endl;
	visitedNodes.insert(cfgNode);
	IntermediateCode instr;
	for (int i = 0; i < cfgNode->instructionAddrList.size(); i++)
	{
		instr = intermediateCodelist[cfgNode->instructionAddrList[i]];
		if(instr.opcode.compare("eliminated")!=0)
			printIntermediateCode(instr);
	}
	cout << "]\"" << endl << "}";
	for (int i = 0; i < cfgNode->next.size(); i++)
	{
		cout << "edge: { sourcename: \"" << cfgNode->id << "\"  targetname: \"" << cfgNode->next[i]->id;
		cout << "\" color: blue }" << endl;
		if (visitedNodes.find(cfgNode->next[i]) == visitedNodes.end())
		{
			outputVCGFile(cfgNode->next[i]);
		}

	}
	if (cfgNode == root)
	{
		//	cout << "}";
		parser->outputDominatorTree();
	}
}


EliminateRedundency::~EliminateRedundency()
{
}
