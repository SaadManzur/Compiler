#include "RegisterAllocator.h"

RegisterAllocator::RegisterAllocator(EliminateRedundency *redundancyEliminator, int currentCodeAddress, int proxyRegisterStart)
{
	this->currentCodeAddress = currentCodeAddress;
	this->redundancyEliminator = redundancyEliminator;

	for (int i = 0; i < NUMBER_OF_REGISTERS; i++)
	{
		registers[i] = "R" + to_string(i + 1);
	}

	lastVirtualRegisterNumber = proxyRegisterStart;
}

void RegisterAllocator::generateInterferenceGraph(BasicBlock* root)
{
//	fillParentBlocks(root);
	visited.clear();
	fillParentBlocksDFS(root);
}

void RegisterAllocator::calculateLiveRange(BasicBlock *node, set<string> alive, int depth)
{	
	node->alive.insert(alive.begin(), alive.end());

	if (node->isLoopHeader && node->loopCounter < 2)
	{
		for (auto backNode : node->back)
		{
			if (node->dominatedBy != backNode)
			{
				node->loopCounter++;

				calculateLiveRangeForBlock(node, false);

				set<string> aliveForLoopBody;

				set_difference(node->alive.begin(), node->alive.end(),
					node->phiAliveFromLeft.begin(), node->phiAliveFromLeft.end(),
					inserter(aliveForLoopBody, aliveForLoopBody.end()));

				calculateLiveRange(backNode, aliveForLoopBody);

				return;
			}
		}
	}
	else if (node->isLoopHeader)
	{
		calculateLiveRangeForBlock(node);

		set<string> aliveForDominator;

		set_difference(node->alive.begin(), node->alive.end(),
			node->phiAliveFromRight.begin(), node->phiAliveFromRight.end(),
			inserter(aliveForDominator, aliveForDominator.end()));

		calculateLiveRange(node->dominatedBy, aliveForDominator);

		return;
	}
	else
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
	node->loopCounter = 0;

	if (node->isJoinBlock)
	{
		int i = 0;
		for (auto parentNode : node->back)
		{
			if (parentNode->id == node->dominatedBy->id)
				continue;

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

void RegisterAllocator::calculateLiveRangeForBlock(BasicBlock *node, int depth, bool liveRangeGenerated)
{
	cout << node->id << endl;

	for (int i = node->instructionAddrList.size() - 1; i >= 0; i--) {

		int instructionAddress = node->instructionAddrList[i];

		IntermediateCode instruction = redundancyEliminator->getIntermediateCode(instructionAddress);
		transform(instruction.opcode.begin(), instruction.opcode.end(), instruction.opcode.begin(), ::tolower);

		if ( instruction.opcode == "mov")
		{
			if (node->alive.find(instruction.getOperandRepresentation(1)) != node->alive.end())
			{
				node->alive.erase(instruction.getOperandRepresentation(1));
			}

			cost[instruction.getOperandRepresentation(1)] += (depth * 0.5);

			generateEdgeBetween(instruction.getOperandRepresentation(1), node->alive);

			if (instruction.operandType[0] == "var" || instruction.operandType[0] == "IntermediateCode")
			{
				node->alive.insert(instruction.getOperandRepresentation(0));
				cost[instruction.getOperandRepresentation(0)] += depth;
			}
		}
		else if (instruction.opcode == "pop")
		{
			if (node->alive.find(instruction.getOperandRepresentation(0)) != node->alive.end())
			{
				node->alive.erase(instruction.getOperandRepresentation(0));
			}

			cost[instruction.getOperandRepresentation(0)] += (depth * 0.5);

			generateEdgeBetween(instruction.getOperandRepresentation(0), node->alive);
		}
		else if (instruction.opcode == "phi")
		{
			if (find_if(phiInstructions.begin(), phiInstructions.end(), [&](const IntermediateCode &a) { return a.address == instruction.address; }) == phiInstructions.end())
				phiInstructions.push_back(instruction);

			if (node->alive.find(instruction.getOperandRepresentation(0)) != node->alive.end())
			{
				node->alive.erase(instruction.getOperandRepresentation(0));
			}
			generateEdgeBetween(instruction.getOperandRepresentation(0), node->alive);
			cost[instruction.getOperandRepresentation(0)] += (depth * 0.5);
			
			string leftOperand = instruction.getOperandRepresentation(1);
			string rightOperand = instruction.getOperandRepresentation(2);

			cost[leftOperand] += depth;
			cost[rightOperand] += depth;

			node->alive.insert(leftOperand);
			node->alive.insert(rightOperand);

			node->phiAliveFromLeft.insert(leftOperand);
			node->phiAliveFromRight.insert(rightOperand);

			node->loopPhiProcessed = true;
		}
		else if(instruction.opcode != "eliminated")
		{
			for (int j = 0; j < 3; j++)
			{
				if (instruction.operandType[j] == "var" || instruction.operandType[j] == "IntermediateCode")
				{
					node->alive.insert(instruction.getOperandRepresentation(j));
					cost[instruction.getOperandRepresentation(j)] += depth;
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

void RegisterAllocator::colorGraph()
{
	if (interferenceGraph.size() == 0)
		return;
	
	string selectedNode = getNodeWithDegreeLessThanN();

	if (selectedNode.empty())
		selectedNode = spillRegisterAndGetNode();

	set<string> adjacency = removeNodeFromInterferenceGraph(selectedNode);

	colorGraph();

	insertNodeIntoInterferenceGraph(selectedNode, adjacency);

	assignColor(selectedNode);
}

set<string> RegisterAllocator::removeNodeFromInterferenceGraph(string node)
{
	set<string> nodeAdjacency = interferenceGraph[node];
	
	interferenceGraph.erase(node);

	for (string element : nodeAdjacency)
	{
		interferenceGraph[element].erase(node);
	}

	return nodeAdjacency;
}

void RegisterAllocator::insertNodeIntoInterferenceGraph(string node, set<string> adjacency)
{
	for (string neighborNode : adjacency)
	{
		interferenceGraph[neighborNode].insert(node);
	}

	interferenceGraph[node] = adjacency;
}

string RegisterAllocator::getNodeWithDegreeLessThanN()
{
	for (auto node : interferenceGraph)
	{
		if (interferenceGraph[node.first].size() <= NUMBER_OF_REGISTERS)
			return string(node.first);
	}

	return string();
}

void RegisterAllocator::assignColor(string node)
{
	int selectedRegister = -1;

	bool registerInUse[NUMBER_OF_REGISTERS];
	memset(registerInUse, false, NUMBER_OF_REGISTERS);

	for (string adjacentNode : interferenceGraph[node])
	{
		registerInUse[assignedColors[adjacentNode]-1] = true;
	}

	for (int i = 1; i <= NUMBER_OF_REGISTERS; i++)
	{
		if (!registerInUse[i-1])
		{
			selectedRegister = i;
			break;
		}
	}

	if(selectedRegister < 0)
		selectedRegister = lastVirtualRegisterNumber++;

	assignedColors[node] = selectedRegister;
}

string RegisterAllocator::spillRegisterAndGetNode()
{
	int minCost = 9999;
	string selectedNode;

	for (auto node : interferenceGraph)
	{
		if (cost[string(node.first)] < minCost)
		{
			selectedNode = string(node.first);
			minCost = cost[string(node.first)];
		}
	}

	return selectedNode;
}

void RegisterAllocator::coalsceLiveRanges()
{
	int clusterCount = 1;

	for (IntermediateCode currentPhi : phiInstructions)
	{
		string clusterId = "C" + to_string(clusterCount++);
		string phiFirstOperand = currentPhi.getOperandRepresentation(0);
		clusters[clusterId].insert(phiFirstOperand);
		
		set<string> clusterEdges;
		clusterEdges.insert(interferenceGraph[phiFirstOperand].begin(), interferenceGraph[phiFirstOperand].end());

		for (int i = 1; i < MAXOPERANDLENGTH; i++)
		{
			string x = currentPhi.getOperandRepresentation(i);
			string y = getClusterName(x);
			for (string clusterMember : clusters[clusterId])
			{
				if (interferenceGraph[y].find(getClusterName(clusterMember)) == interferenceGraph[y].end())
				{
					clusters[clusterId].insert(x);

					set<string> newEdges = removeNodeFromInterferenceGraph(x);

					clusterEdges.insert(newEdges.begin(), newEdges.end());
				}
			}
		}

		if(clusters[clusterId].size() > 1)
			replaceNodeWithCluster(phiFirstOperand, clusterId, clusterEdges);

		if (clusters[clusterId].size() == 3)
			instructionsToBeEliminated.push_back(currentPhi);
	//	while (mergeClusters());
	}
}

void RegisterAllocator::replaceNodeWithCluster(string node, string clusterId, set<string> edges)
{
	removeNodeFromInterferenceGraph(node);

	insertNodeIntoInterferenceGraph(clusterId, edges);
}

void RegisterAllocator::applyClusterColor()
{
	for (auto cluster : clusters)
	{
		string clusterId = cluster.first;

		if (assignedColors.find(clusterId) != assignedColors.end())
		{
			for (string clusterMember : clusters[clusterId])
			{
				assignedColors[clusterMember] = assignedColors[clusterId];
			}

			assignedColors.erase(clusterId);
		}
	}
}

void RegisterAllocator::eliminatePhi()
{
	set<int> eliminatedInstructions;

	for (auto instruction : instructionsToBeEliminated)
	{
		instructionBlocks[instruction.address]->removeInstruction(instruction);

		eliminatedInstructions.insert(instruction.address);
	}
	
	for (int i = 0; i < phiInstructions.size(); i++)
	{
		IntermediateCode *instruction = &phiInstructions[i];

		if (eliminatedInstructions.find(instruction->address) == eliminatedInstructions.end())
		{
			int phiRegister = assignedColors[instruction->operand[0] + "_" + to_string(instruction->version[0])];
			int firstRegister = assignedColors[instruction->operand[1] + "_" + to_string(instruction->version[1])];
			int secondRegister = assignedColors[instruction->operand[2] + "_" + to_string(instruction->version[2])];

			BasicBlock *phiBlock = instructionBlocks[instruction->address];

			if (phiRegister != firstRegister)
			{
				IntermediateCode movInstruction = createMoveInstruction(instruction, 1, 0);
				redundancyEliminator->insertIntermediateCode(movInstruction);

				if (phiBlock->isJoinBlock)
				{
					addInstructionToParent(movInstruction, phiBlock->back[0]);
				}
				else
				{
					addInstructionToParent(movInstruction, phiBlock->dominatedBy);
				}
			}
			
			if (phiRegister != secondRegister)
			{
				IntermediateCode movInstruction = createMoveInstruction(instruction, 2, 0);
				redundancyEliminator->insertIntermediateCode(movInstruction);

				if (phiBlock->isJoinBlock)
				{
					addInstructionToParent(movInstruction, phiBlock->back[1]);
				}
				else
				{
					addInstructionToParent(movInstruction, phiBlock->back[1]);
				}
			}
			
			phiBlock->removeInstruction(*instruction);
		}
	}
	
}

IntermediateCode RegisterAllocator::createMoveInstruction(IntermediateCode *instruction, int i, int j)
{

	IntermediateCode movInstruction;
	movInstruction.opcode = "mov";
	movInstruction.address = currentCodeAddress++;
	movInstruction.operand[0] = instruction->operand[i];
	movInstruction.operand[1] = instruction->operand[j];
	movInstruction.version[0] = instruction->version[i];
	movInstruction.version[1] = instruction->version[j];
	movInstruction.operandType[0] = instruction->operandType[i];
	movInstruction.operandType[1] = instruction->operandType[j];

	return movInstruction;
}

int RegisterAllocator::getFirstBranchingWithinBlock(BasicBlock * block)
{
	int i = block->instructionAddrList.size();

	for (int address : block->instructionAddrList)
	{
		if (redundancyEliminator->getIntermediateCode(address).opcode[0] == 'b')
		{
			i--;
		}
	}

	return i;
}

void RegisterAllocator::addInstructionToParent(IntermediateCode instruction, BasicBlock *parent)
{
	int branchingAddress = getFirstBranchingWithinBlock(parent);

	parent->addInstructionAtPosition(instruction, branchingAddress);
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

		for (int address : currentBlock->instructionAddrList)
		{
			instructionBlocks[address] = currentBlock;
		}
		
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
	cout << "------------Interference Graph--------------" << endl;
	cout << "Total nodes: " << interferenceGraph.size() << endl;

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

void RegisterAllocator::printAssignedRegisters()
{
	cout << "Total assigned nodes : " << assignedColors.size() << endl;
	for (auto value : assignedColors)
	{
		cout << value.first << " " << getAssignedRegister(value.first) << endl;
	}
}

void RegisterAllocator::printClusters()
{
	cout << "----------------Clusters------------------" << endl;

	for (auto cluster : clusters)
	{
		cout << cluster.first << ": ";

		for (string clusterMember : cluster.second)
		{
			cout << clusterMember << " ";
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

int RegisterAllocator::tryMerge(string c1, string c2)
{
	for (auto it1 = clusters[c1].begin(); it1 != clusters[c1].end(); it1++)
	{
		for (auto it2 = clusters[c2].begin(); it2 != clusters[c2].end(); it2++)
		{
			if (it1->compare(*it2) == 0)
			{
				clusters[c1].insert(clusters[c2].begin(), clusters[c2].end());
				if(interferenceGraph[c1].find(c2)!=interferenceGraph[c1].end())
					interferenceGraph[c1].erase(c2);
				if (interferenceGraph[c2].find(c1) != interferenceGraph[c2].end())
					interferenceGraph[c2].erase(c1);
				interferenceGraph[c1].insert(interferenceGraph[c2].begin(), interferenceGraph[c2].end());
				interferenceGraph.erase(c2);
				clusters.erase(c2);

				for (auto it = interferenceGraph.begin(); it != interferenceGraph.end(); it++)
				{
					if (it->second.find(c2) != it->second.end())
					{
						it->second.erase(c2);
						it->second.insert(c1);
					}
						
				}
			/**/	return 1;
			}
				
		}
	}
	return 0;
}

int RegisterAllocator::mergeClusters()
{
	for (auto it1 = clusters.begin(); it1!=clusters.end(); it1++)
	{
		auto it2 = it1;
		for (it2++; it2 != clusters.end(); it2++)
		{
			if (tryMerge(it1->first, it2->first))
				return 1;
		}
	}
	return 0;
}

string RegisterAllocator::getClusterName(string x)
{
	for (auto it = clusters.begin(); it != clusters.end(); it++)
	{
		for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++)
		{
			if (it2->compare(x) == 0)
				return it->first;
		}
	}
	return x;
}

string RegisterAllocator::getAssignedRegister(string operand)
{
	return "R" + to_string(assignedColors[operand]);
}

map<string, int> RegisterAllocator::getAllAssignedRegisters()
{
	return assignedColors;
}

vector<IntermediateCode> RegisterAllocator::getInstructionsToBeEliminated()
{
	return instructionsToBeEliminated;
}

int RegisterAllocator::getCurrentCodeAddress()
{
	return currentCodeAddress;
}

int RegisterAllocator::getLastVirtualRegisterNumber()
{
	return lastVirtualRegisterNumber;
}

void RegisterAllocator::start(BasicBlock *root)
{
//	fillParentBlocks(root);
	visited.clear();
	fillParentBlocksDFS(root);

	set<string> alive;

	cout << endl << "-----------Node Traverse Order--------------" << endl;
	calculateLiveRange(outerMostBlock, alive);

	cout << endl << "Still alive : " << root->alive.size() << endl << endl;


//	printInterferenceGraph();
	coalsceLiveRanges();
//	printInterferenceGraph();
//	printClusters();
	while (mergeClusters());
	printInterferenceGraph();
	printClusters();

	cout << endl << "-----------------Coloring-------------------" << endl;
	colorGraph();
	printAssignedRegisters();

	applyClusterColor();
	printAssignedRegisters();

	eliminatePhi();
}

void RegisterAllocator::fillParentBlocksDFS(BasicBlock * root)
{
	if (root == NULL)
		return;

	visited.insert(root);

	for (int address : root->instructionAddrList)
	{
		instructionBlocks[address] = root;
	}

	if (root->next.size() == 0)
		outerMostBlock = root;

	/*	for (auto childBlock : root->next)
		{
			childBlock->back.push_back(root);
		}
	*/
	for (auto dominatedBlock : root->dominates)
	{
		dominatedBlock->dominatedBy = root;
	}

	for (int i = 0; i < root->next.size(); i++)
	{
		root->next[i]->back.push_back(root);
		if (visited.find(root->next[i]) == visited.end())
		{
			fillParentBlocksDFS(root->next[i]);
		}
	}

	/*

		while (!blocks.empty())
		{
			BasicBlock *currentBlock = blocks.front();
			blocks.pop();

			if (visited.find(currentBlock) != visited.end())
			{
				continue;
			}

			visited.insert(currentBlock);

			for (int address : currentBlock->instructionAddrList)
			{
				instructionBlocks[address] = currentBlock;
			}

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
		}*/
}
