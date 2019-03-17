#include "RegisterAllocator.h"

RegisterAllocator::RegisterAllocator(const Parser &parser)
{
	this->parser = parser;

	for (int i = 0; i < NUMBER_OF_REGISTERS; i++)
	{
		registers[i] = "R" + to_string(i + 1);
	}
}

void RegisterAllocator::generateInterferenceGraph(BasicBlock* root)
{
	fillParentBlocks(root);
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

void RegisterAllocator::calculateLiveRangeForBlock(BasicBlock *node, int depth, bool liveRangeGenerated)
{
	cout << node->id << endl;

	for (int i = node->instructionAddrList.size() - 1; i >= 0; i--) {

		int instructionAddress = node->instructionAddrList[i];

		IntermediateCode instruction = parser.getIntermediateCode(instructionAddress);

		if (instruction.opcode == "mov")
		{
			if (node->alive.find(instruction.getOperandRepresentation(1)) != node->alive.end())
			{
				node->alive.erase(instruction.getOperandRepresentation(1));
			}

			cost[instruction.getOperandRepresentation(1)] += (depth * 0.5);

			generateEdgeBetween(instruction.getOperandRepresentation(1), node->alive);

			if (instruction.operandType[0] == "var")
			{
				node->alive.insert(instruction.getOperandRepresentation(0));
				cost[instruction.getOperandRepresentation(0)] += depth;
			}
			else if (instruction.operandType[0] == "IntermediateCode")
			{
				node->alive.insert(instruction.operand[0]);
				cost[instruction.operand[0]] += depth;
			}
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
		else
		{
			for (int j = 0; j < 3; j++)
			{
				if (instruction.operandType[j] == "var")
				{
					node->alive.insert(instruction.getOperandRepresentation(j));
					cost[instruction.getOperandRepresentation(j)] += depth;
				}
				else if (instruction.operandType[j] == "IntermediateCode")
				{
					node->alive.insert(instruction.operand[j]);
					cost[instruction.operand[j]] += depth;
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
		registerInUse[assignedColors[adjacentNode]] = true;
	}

	for (int i = 0; i < NUMBER_OF_REGISTERS; i++)
	{
		if (!registerInUse[i])
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

void RegisterAllocator::eliminatePhi()
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

			for (string clusterMember : clusters[clusterId])
			{
				if (interferenceGraph[x].find(clusterMember) == interferenceGraph[x].end())
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

		for (string clusterMember : clusters[clusterId])
		{
			assignedColors[clusterMember] = assignedColors[clusterId];
		}

		assignedColors.erase(clusterId);
	}
}

void RegisterAllocator::eliminateInstructions()
{
	for (auto instruction : instructionsToBeEliminated)
	{
		instructionBlocks[instruction.address]->removeInstruction(instruction);
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

void RegisterAllocator::start(BasicBlock *root)
{
	fillParentBlocks(root);

	set<string> alive;

	cout << endl << "-----------Node Traverse Order--------------" << endl;
	calculateLiveRange(outerMostBlock, alive);

	cout << endl << "Still alive : " << root->alive.size() << endl << endl;
	printInterferenceGraph();

	eliminatePhi();
	eliminateInstructions();
	printInterferenceGraph();
	printClusters();

	cout << endl << "-----------------Coloring-------------------" << endl;
	colorGraph();
	applyClusterColor();
	printAssignedRegisters();
}
