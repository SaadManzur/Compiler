#include "CodeGenerator.h"

void CodeGenerator::generateControlFlow()
{
	stack<BasicBlock*> dominatedStack;
	set<BasicBlock *> visited;
	dominatedStack.push(mainScope->root);

	cout << "---------------Generating Control Flow----------------------" << endl;
	while (!dominatedStack.empty())
	{
		BasicBlock *currentBlock = dominatedStack.top();
		dominatedStack.pop();

		if (visited.find(currentBlock) != visited.end())
			continue;

		cout << currentBlock->id << endl;
		visited.insert(currentBlock);

		controlFlow.push_back(currentBlock);
		
		printIntermediateCodes(currentBlock, mainScope);

		for (int i = currentBlock->dominates.size() - 1; i >= 0; i--)
		{
			dominatedStack.push(currentBlock->dominates[i]);
		}
	}
}

void CodeGenerator::processBranchingInstructions()
{
	for (auto blockInstructionPair : blockFirstInstructionAddress)
	{
		if (blockInstructionPair.second != 0)
		{
			processBlockLastInstructionIfBranching(blockInstructionPair.second - 1);
		}
	}
}

void CodeGenerator::processBlockLastInstructionIfBranching(int address)
{
	if (intermediateTargetCodeCandidates[address].opcode[0] == 'b')
	{
		IntermediateCode *instruction = &intermediateTargetCodeCandidates[address];

		if (intermediateTargetCodeCandidates[address].opcode == "bra")
		{
			BasicBlock *branchingBlock = (BasicBlock *)(instruction->version[0]);
			instruction->operand[0] = to_string(blockFirstInstructionAddress[branchingBlock->id] - address);
		}
		else
		{
			BasicBlock *branchingBlock = (BasicBlock *)(instruction->version[1]);
			instruction->operand[1] = to_string(blockFirstInstructionAddress[branchingBlock->id] - address);
		}
	}
}

void CodeGenerator::integrateRegisterWithIntermediateCodes(Scope *currentScope)
{
	for (int i = 0; i < intermediateTargetCodeCandidates.size(); i++)
	{
		IntermediateCode *instruction = &intermediateTargetCodeCandidates[i];

		string intermediateAddress = instruction->getImmediateAddressRepresentation();

		if (currentScope->assignedRegisters.find(intermediateAddress) != currentScope->assignedRegisters.end())
		{
			instruction->addressRegister = currentScope->assignedRegisters[intermediateAddress];
		}

		for (int i = 0; i < MAXOPERANDLENGTH; i++)
		{
			if (instruction->operandType[i] == "var" || instruction->operandType[i] == "IntermediateCode")
			{
				string operand = instruction->getOperandRepresentation(i);

				instruction->registers[i] = currentScope->assignedRegisters[operand];
			}
		}
	}
}

void CodeGenerator::generateCode()
{
	for (int i = 0; i < intermediateTargetCodeCandidates.size(); i++)
	{
		generateCodeForInstruction(intermediateTargetCodeCandidates[i]);
	}
}

void CodeGenerator::generateCodeForInstruction(IntermediateCode instruction)
{
	transform(instruction.opcode.begin(), instruction.opcode.end(), instruction.opcode.begin(), ::tolower);

	unsigned int code = 0;

	if (instruction.opcode == "mov")
	{
		code = getAssignmentCode(instruction);
	}
	else if (instruction.opcode == "add")
	{
		code = getMathCode(instruction, dlxProcessor.ADD);
	}
	else if (instruction.opcode == "sub")
	{
		code = getMathCode(instruction, dlxProcessor.SUB);
	}
	else if (instruction.opcode == "div")
	{
		code = getMathCode(instruction, dlxProcessor.DIV);
	}
	else if (instruction.opcode == "mul")
	{
		code = getMathCode(instruction, dlxProcessor.MUL);
	}
	else if (instruction.opcode == "mod")
	{
		code = getMathCode(instruction, dlxProcessor.MOD);
	}
	else if (instruction.opcode == "cmp")
	{
		code = getMathCode(instruction, dlxProcessor.CMP);
	}
	else if (instruction.opcode[0] == 'b')
	{
		code = getBranchCode(instruction);
	}
	else if (instruction.opcode == "write")
	{
		code = dlxProcessor.assemble(dlxProcessor.WRD, instruction.registers[0]);
	}

	if (code > 0)
		targetCodes.push_back(code);

}

int CodeGenerator::getAssignmentCode(IntermediateCode instruction)
{
	if (instruction.operandType[0] == "const")
	{
		return dlxProcessor.assemble(dlxProcessor.ADDI, instruction.registers[1], 0, stoi(instruction.operand[0]));
	}
	else
	{
		return dlxProcessor.assemble(dlxProcessor.ADD, instruction.registers[1], instruction.registers[0], 0);
	}
}

int CodeGenerator::getMathCode(IntermediateCode instruction, int opcode)
{
	if (instruction.operandType[0] == "const")
	{
		return dlxProcessor.assemble(opcode + 16, instruction.addressRegister, instruction.registers[1], stoi(instruction.operand[0]));
	}
	else if (instruction.operandType[1] == "const")
	{
		return dlxProcessor.assemble(opcode + 16, instruction.addressRegister, instruction.registers[0], stoi(instruction.operand[1]));
	}
	else
	{
		return dlxProcessor.assemble(opcode + 16, instruction.addressRegister, instruction.registers[0], instruction.registers[1]);
	}
}

int CodeGenerator::getBranchCode(IntermediateCode instruction)
{
	if (instruction.opcode == "bra")
	{
		int wordOffset = stoi(instruction.operand[0]);

		return dlxProcessor.assemble(dlxProcessor.BSR, wordOffset);
	}
	else if(instruction.opcode == "bne")
	{
		return getBranchCode(instruction, dlxProcessor.BNE);
	}
	else if (instruction.opcode == "beq")
	{
		return getBranchCode(instruction, dlxProcessor.BEQ);
	}
	else if (instruction.opcode == "ble")
	{
		return getBranchCode(instruction, dlxProcessor.BLE);
	}
	else if (instruction.opcode == "blt")
	{
		return getBranchCode(instruction, dlxProcessor.BLT);
	}
	else if (instruction.opcode == "bge")
	{
		return getBranchCode(instruction, dlxProcessor.BGE);
	}
	else
	{
		return getBranchCode(instruction, dlxProcessor.BGT);
	}
}

int CodeGenerator::getBranchCode(IntermediateCode instruction, int opcode)
{
	int wordOffset = stoi(instruction.operand[1]);

	return dlxProcessor.assemble(opcode, instruction.registers[0], wordOffset);
}

void CodeGenerator::printControlFlow(Scope * currentScope)
{
	cout << "--------------Control Flow-----------------" << endl;

	for (int i = 0; i < intermediateTargetCodeCandidates.size(); i++)
	{
		printIntermediateTargetCode(intermediateTargetCodeCandidates[i], currentScope);
	}
}

void CodeGenerator::printIntermediateCodes(BasicBlock * block, Scope *currentScope)
{
	blockFirstInstructionAddress[block->id] = intermediateTargetCodeCandidates.size();

	for (int address : block->instructionAddrList)
	{
		IntermediateCode instruction = intermediateCodeList[address];

		if (instruction.opcode != "eliminated")
		{
			intermediateTargetCodeCandidates.push_back(instruction);

			printIntermediateCode(instruction, currentScope);
		}
	}
}

void CodeGenerator::printIntermediateCode(IntermediateCode instruction, Scope *currentScope)
{
	if (instruction.opcode == "eliminated")
		return;

	string addressRepresentation = instruction.getImmediateAddressRepresentation();

	cout << getRegisterIfAssigned(addressRepresentation, currentScope) << " :";

	cout << instruction.opcode;
	for (int i = 0; i < MAXOPERANDLENGTH && (instruction.operand[i].length() > 0 || instruction.operandType->length() > 0); i++)
	{
		if (instruction.operandType[i].compare("JumpAddr") == 0)
		{
			BasicBlock * nextBlock = (BasicBlock *)(instruction.version[i]);
			instruction.operand[i] = to_string(nextBlock->id);
		}
		if (instruction.operandType[i].compare("IntermediateCode") == 0)
			instruction.operand[i] = "(" + to_string(instruction.version[i]) + ")";
		
		string operand = instruction.operand[i];
		if (instruction.operandType[i].compare("var") == 0)
		{
			operand += '_' + to_string(instruction.version[i]);
		}
		
		if (instruction.operandType[i] == "var" || instruction.operandType[i] == "IntermediateCode")
		{
			cout << " " << getRegisterIfAssigned(operand, currentScope);
		}
		else
		{
			cout << " " << operand;
		}
	}
	cout << endl;
}

void CodeGenerator::printIntermediateTargetCode(IntermediateCode instruction, Scope * currentScope)
{
	string addressRepresentation = instruction.getImmediateAddressRepresentation();

	cout << getRegisterIfAssigned(addressRepresentation, currentScope) << " :";

	cout << instruction.opcode;
	for (int i = 0; i < MAXOPERANDLENGTH && (instruction.operand[i].length() > 0 || instruction.operandType->length() > 0); i++)
	{

		if (instruction.operandType[i].compare("IntermediateCode") == 0)
			instruction.operand[i] = "(" + to_string(instruction.version[i]) + ")";

		string operand = instruction.operand[i];
		if (instruction.operandType[i].compare("var") == 0)
		{
			operand += '_' + to_string(instruction.version[i]);
		}

		if (instruction.operandType[i] == "var" || instruction.operandType[i] == "IntermediateCode")
		{
			cout << " " << getRegisterIfAssigned(operand, currentScope);
		}
		else
		{
			cout << " " << operand;
		}
	}
	cout << endl;
}

void CodeGenerator::printTargetCode(bool verbose)
{
	cout << "--------------------Target Code----------------------" << endl;
	for (int i = 0; i < targetCodes.size(); i++)
	{
		cout << i << " : " << dlxProcessor.disassemble(targetCodes[i]);
	}
}

string CodeGenerator::getRegisterIfAssigned(string operand, Scope *currentScope)
{
	if (currentScope->assignedRegisters.find(operand) != currentScope->assignedRegisters.end())
	{
		return "R" + to_string(currentScope->assignedRegisters[operand]);
	}

	return "NA";
}

CodeGenerator::CodeGenerator(Scope * mainScope, vector<Scope*> functions, vector<IntermediateCode> intermediateCodeList)
{
	this->mainScope = mainScope;
	this->functions.insert(this->functions.begin(), functions.begin(), functions.end());
	this->intermediateCodeList.insert(this->intermediateCodeList.begin(), intermediateCodeList.begin(), intermediateCodeList.end());
}

void CodeGenerator::generate()
{
	generateControlFlow();

	processBranchingInstructions();

	integrateRegisterWithIntermediateCodes(mainScope);

	generateCode();

	printTargetCode();

	dlxProcessor.load(targetCodes);

	dlxProcessor.execute();
}
