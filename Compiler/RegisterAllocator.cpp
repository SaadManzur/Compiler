#include "RegisterAllocator.h"

RegisterAllocator::RegisterAllocator(const Parser &parser)
{
	this->parser = parser;
}

void RegisterAllocator::generateInterferenceGraph(BasicBlock* root)
{
	fillParentBlocks(root);
}

set<string> RegisterAllocator::calculateLiveRange(BasicBlock *node, set<string> alive)
{	
	if (node->isLoopHeader)
	{
		for (auto dominatedNode : node->dominates)
		{
			if (!dominatedNode->liveRangeGenerated)
			{
				alive = calculateLiveRangeForBlock(node, alive, false);
				alive = calculateLiveRange(getOuterMostBlock(dominatedNode), alive);

				return alive;
			}
		}
	}
	else if (!node->isLoopHeader)
	{
		for (auto dominatedNode : node->dominates)
		{
			if (!dominatedNode->liveRangeGenerated)
			{
				return alive;
			}
		}
	}

	if (!node->liveRangeGenerated)
	{
		calculateLiveRangeForBlock(node, alive);

		for (auto parentNode : node->back)
		{
			calculateLiveRange(parentNode, alive);
		}

		for (auto dominatorNode : node->dominatedBy)
		{
			calculateLiveRange(dominatorNode, alive);
		}
	}

	return alive;
}

set<string> RegisterAllocator::calculateLiveRangeForBlock(BasicBlock *node, set<string> alive, bool liveRangeGenerated)
{
	cout << node->id << endl;

	for (int i = node->instructionAddrList.size() - 1; i >= 0; i--) {

		int instructionAddress = node->instructionAddrList[i];

		IntermediateCode instruction = parser.getIntermediateCode(instructionAddress);

		if (instruction.opcode == "mov")
		{
			if (instruction.operandType[0] == "var")
			{
				alive.insert(instruction.getOperandRepresentation(0));
			}
			else if (instruction.operandType[0] == "IntermediateCode")
			{
				alive.insert(instruction.operand[0]);
			}

			if (alive.find(instruction.getOperandRepresentation(1)) != alive.end())
			{
				alive.erase(instruction.getOperandRepresentation(1));
			}

			generateEdgeBetween(instruction.getOperandRepresentation(1), alive);
		}
		else if (instruction.opcode == "phi")
		{
			if (alive.find(instruction.getOperandRepresentation(0)) != alive.end())
			{
				alive.erase(instruction.getOperandRepresentation(0));
			}

			alive.insert(instruction.getOperandRepresentation(1));
			alive.insert(instruction.getOperandRepresentation(2));

			generateEdgeBetween(instruction.getOperandRepresentation(0), alive);
		}
		else
		{
			for (int i = 0; i < 3; i++)
			{
				if (instruction.operandType[i] == "var")
					alive.insert(instruction.getOperandRepresentation(i));
			}

			if (alive.find(instruction.getImmediateAddressRepresentation()) != alive.end())
			{
				alive.erase(instruction.getImmediateAddressRepresentation());

				generateEdgeBetween(instruction.getImmediateAddressRepresentation(), alive);
			}
		}
	}

	node->liveRangeGenerated = liveRangeGenerated;

	return alive;
}

void RegisterAllocator::generateEdgeBetween(string variable, set<string> alive)
{
	for (string element : alive)
	{
		interferenceGraph[variable].insert(element);
	}
}



void RegisterAllocator::fillParentBlocks(BasicBlock *root)
{
	if (root == NULL)
		return;

	set<BasicBlock*> visited;

	queue<BasicBlock*> blocks;
	blocks.push(root);

	while (!blocks.empty())
	{
		BasicBlock *currentBlock = blocks.front();
		blocks.pop();

		if (visited.find(currentBlock) != visited.end())
		{
			continue;
		}
		
		visited.insert(currentBlock);

		for (auto childBlock : currentBlock->next)
		{
			childBlock->back.push_back(currentBlock);

			if (visited.find(childBlock) == visited.end())
			{
				blocks.push(childBlock);
			}
		}

		for (auto dominatedBlock : currentBlock->dominates)
		{
			dominatedBlock->dominatedBy.push_back(currentBlock);
		}
	}
}

void RegisterAllocator::start(BasicBlock *root)
{
	fillParentBlocks(root);

	set<string> alive;
	set<BasicBlock *> visited;

	//printParents(root, visited);

	calculateLiveRange(getOuterMostBlock(root), alive);

	printInterferenceGraph();
}

void RegisterAllocator::printParents(BasicBlock *root, set<BasicBlock*> visited)
{
	if (root == NULL)
		return;

	if (visited.find(root) != visited.end())
		return;

	visited.insert(root);

	cout << root->id << ": ";

	for (auto parentBlock : root->back)
	{
		cout << parentBlock->id << " ";
	}

	cout << "dominates ";
	for (auto dominatedBlock : root->dominates)
	{
		cout << dominatedBlock->id << " ";
	}

	cout << "dominated by ";
	for (auto dominatorBlock : root->dominatedBy)
	{
		cout << dominatorBlock->id << " ";
	}

	cout << endl;

	for (auto childBlock : root->next)
	{
		printParents(childBlock, visited);
	}
}

void RegisterAllocator::printInterferenceGraph()
{
	for (auto element : interferenceGraph)
	{
		cout << element.first << ": ";

		for (string value : element.second)
		{
			cout << value << " ";
		}
		
		cout << endl;
	}
}

BasicBlock *RegisterAllocator::getOuterMostBlock(BasicBlock *root)
{
	if (root == NULL)
		return NULL;

	BasicBlock *outerMostBlock = root;

	for (auto dominatedBlock : root->dominates)
	{
		BasicBlock *outerMostFromDominatedBlock = getOuterMostBlock(dominatedBlock);

		outerMostBlock = (outerMostFromDominatedBlock != NULL && outerMostFromDominatedBlock->id > outerMostBlock->id) 
									? outerMostFromDominatedBlock : outerMostBlock;
	}

	return outerMostBlock;
}