#include "RegisterAllocator.h"

RegisterAllocator::RegisterAllocator(const Parser &parser)
{
	this->parser = parser;
}

void RegisterAllocator::generateInterferenceGraph(BasicBlock* root)
{
	fillParentBlocks(root);
}

void RegisterAllocator::calculateLiveRange(BasicBlock *node, set<string> alive)
{	
	node->alive.insert(alive.begin(), alive.end());

	if (node->isLoopHeader)
	{
		for (auto backNode : node->back)
		{
			if (node->dominatedBy != backNode && !backNode->liveRangeGenerated)
			{
				calculateLiveRangeForBlock(node, false);

				calculateLiveRange(backNode, node->alive);

				return;
			}
		}
	}
	else if (!node->isLoopHeader)
	{
		for (auto childNode : node->next)
		{
			if (!childNode->liveRangeGenerated && !aExistsInBDominatorTree(childNode, node))
			{
				return;
			}
		}
	}

	calculateLiveRangeForBlock(node);

	if (find(node->back.begin(), node->back.end(), node->dominatedBy) == node->back.end())
	{
		int i = 0;
		for (auto parentNode : node->back)
		{
			set<string> aliveForParentNode;
			set<string> setToSubtract = (i == 0) ? node->phiAliveFromRight : node->phiAliveFromLeft;

			set_difference(node->alive.begin(), node->alive.end(), 
						setToSubtract.begin(), setToSubtract.end(), 
						inserter(aliveForParentNode, aliveForParentNode.end()));

			i++;

			calculateLiveRange(parentNode, aliveForParentNode);
		}
	}
	else if(node->dominatedBy != NULL)
	{
		calculateLiveRange(node->dominatedBy, node->alive);
	}
}

void RegisterAllocator::calculateLiveRangeForBlock(BasicBlock *node, bool liveRangeGenerated)
{
	cout << node->id << endl;

	for (int i = node->instructionAddrList.size() - 1; i >= 0; i--) {

		int instructionAddress = node->instructionAddrList[i];

		IntermediateCode instruction = parser.getIntermediateCode(instructionAddress);

		if (instruction.opcode == "mov")
		{
			if (instruction.operandType[0] == "var")
			{
				node->alive.insert(instruction.getOperandRepresentation(0));
			}
			else if (instruction.operandType[0] == "IntermediateCode")
			{
				node->alive.insert(instruction.operand[0]);
			}

			if (node->alive.find(instruction.getOperandRepresentation(1)) != node->alive.end())
			{
				node->alive.erase(instruction.getOperandRepresentation(1));
			}

			generateEdgeBetween(instruction.getOperandRepresentation(1), node->alive);
		}
		else if (instruction.opcode == "phi")
		{
			if (node->alive.find(instruction.getOperandRepresentation(0)) != node->alive.end())
			{
				node->alive.erase(instruction.getOperandRepresentation(0));
			}
			
			string leftOperand = instruction.getOperandRepresentation(1);
			string rightOperand = instruction.getOperandRepresentation(2);

			node->alive.insert(leftOperand);
			node->alive.insert(rightOperand);

			node->phiAliveFromLeft.insert(leftOperand);
			node->phiAliveFromRight.insert(rightOperand);

			generateEdgeBetween(instruction.getOperandRepresentation(0), node->alive);

			node->loopPhiProcessed = true;
		}
		else
		{
			for (int i = 0; i < 3; i++)
			{
				if (instruction.operandType[i] == "var")
				{
					node->alive.insert(instruction.getOperandRepresentation(i));
				}
				else if (instruction.operandType[i] == "IntermediateCode")
				{
					node->alive.insert(instruction.operand[i]);
				}
			}

			if (node->alive.find(instruction.getImmediateAddressRepresentation()) != node->alive.end())
			{
				node->alive.erase(instruction.getImmediateAddressRepresentation());

				generateEdgeBetween(instruction.getImmediateAddressRepresentation(), node->alive);
			}
		}
	}

	node->liveRangeGenerated = liveRangeGenerated;
}

void RegisterAllocator::generateEdgeBetween(string variable, set<string> alive)
{
	for (string element : alive)
	{
		interferenceGraph[variable].insert(element);
		interferenceGraph[element].insert(variable);
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
		
		if (currentBlock->next.size() == 0)
			outerMostBlock = currentBlock;

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
			dominatedBlock->dominatedBy = currentBlock;
		}
	}
}

void RegisterAllocator::start(BasicBlock *root)
{
	fillParentBlocks(root);

	set<string> alive;
	set<BasicBlock *> visited;

	//printParents(root, visited);

	cout << "-----------Node Traverse Order--------------" << endl;
	calculateLiveRange(outerMostBlock, alive);

	cout << "Still alive : " << root->alive.size() << endl;

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
	if (root->dominatedBy != NULL)
		cout << root->dominatedBy->id;
	else
		cout << "null";

	cout << endl;

	for (auto childBlock : root->next)
	{
		printParents(childBlock, visited);
	}
}

void RegisterAllocator::printInterferenceGraph()
{
	cout << "-----------Interference Graph---------------" << endl;

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

bool RegisterAllocator::aExistsInBDominatorTree(BasicBlock *nodeA, BasicBlock *nodeB)
{
	if (nodeB == NULL)
		return false;

	if (nodeA == nodeB->dominatedBy)
		return true;

	return aExistsInBDominatorTree(nodeA, nodeB->dominatedBy);

}