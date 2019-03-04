#include "Parser.h"
#include "Common.h"
#include<iostream>
#include<cstring>

using namespace std;

void Parser::Next()
{
	symbol = scanner->GetSymbol();
}

int Parser::relOp()
{
	int token;
	if (symbol >= 20 && symbol <= 25)
	{
		token = symbol;
		Next();
		return token;
	}
		

	throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

Result Parser::ident()
{
	Result x;
	if (symbol == identToken)
	{
		x.kind = "var";
		x.address = scanner->GetId();
//		x.address = getID();
		Next();
		return x;
	}
	throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

Result Parser::number()
{
	Result x;
	if (symbol == numberToken)
	{
		x.kind = "const";
		x.value = scanner->GetNumber();
		Next();
		return x;
	}
	throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

Result Parser::designator()
{
	Result x;
	vector<Result> dimension;
	try
	{
		x=ident();
		determineType(x);

		while (symbol == openBracketToken)
		{
			Next();
			Result y=expression();
			dimension.push_back(y);

			if (symbol == closeBracketToken) Next();
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
		if (x.kind.compare("var") == 0 && dimension.size()>0)
			throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		if (x.kind.compare("array") == 0)
		{
			if(dimension.size()==0)
				throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
			x = accessArray(dimension, x);
		}
	}
	catch (SyntaxException exception)
	{
		throw exception;
	}
	return x;
}
	

Result Parser::factor()
{
	Result x;
	try
	{
		x=designator();
	}
	catch (SyntaxException e)
	{
		try 
		{
			x=number();
		}
		catch (SyntaxException e)
		{
			if (symbol == openParenToken)
			{
				Next();
				x=expression();

				if (symbol == closeParenToken) Next();
				else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
			}
			else
			{
				funcCall();
			}
		}
	}
	return x;
}

Result Parser::term()
{
	Result x, y;
	x=factor();
	int op;
	while (symbol == timesToken || symbol == divToken)
	{
		op = symbol;
		Next();
		y=factor();
		x = compute(op, x, y);
	}
	return x;
}

Result Parser::expression()
{
	Result x, y;
	x=term();
	int op;

	while (symbol == plusToken || symbol == minusToken)
	{
		op = symbol;
		Next();
		y=term();
		x=compute(op, x, y);
	}
	return x;
}

IntermediateCode Parser::relation()
{
	Result x, y;
	int relToken;
	x=expression();
	relToken=relOp();
	y=expression();

	IntermediateCode instr = createIntermediateCode("cmp", x, y);
	currentBlock->addInstruction(instr);
	//creating conditional branch instruction
	x.address = instr.address;
	x.kind = "IntermediateCode";
	instr = createIntermediateCode(relToken, x, y);   //here y is a dummy parameter
	currentBlock->addInstruction(instr);
	return instr;
}

int Parser::getInScopeID(int id)
{
	string  str= scanner->Id2String(id);
	auto it=currentScope->identifierHashMap.find(str);
	if (it == currentScope->identifierHashMap.end())
		throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber(), "Identifier not found");

	return it->second;
}

void Parser::assignment()
{
	Result x, y;
	if (symbol == letToken)
	{
		Next();
		y=designator();

		if (symbol == becomesToken)
		{
			Next();
			x=expression();
			if (y.kind == "var")
			{
				updateVersion(y.address, currentCodeAddress);
			}
			IntermediateCode instr=createIntermediateCode(becomesToken, x, y);
			if (y.kind == "var" && !joinBlockStack.empty()) //update phi
			{
				updatePhi(y);
			}
			currentBlock->addInstruction(instr);
			
		}
		else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::funcCall()
{
	if (symbol == callToken)
	{
		Next();
		ident();

		if (symbol == openParenToken)
		{
			Next();

			if(symbol != closeParenToken)
			{
				expression();

				while (symbol == commaToken)
				{
					Next();
					expression();
				}
			}
			
			if (symbol == closeParenToken) Next();
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
		else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::ifStatement()
{
	if (symbol == ifToken)
	{
		Next();

		IntermediateCode instrJumpToElse= relation();
		IntermediateCode instrJumpToFi;
	
		BasicBlock *thenBlock = new BasicBlock();
		BasicBlock *ifBlock = currentBlock;
		ifBlock->next.push_back(thenBlock);
		ifBlock->dominates.push_back(thenBlock);

//		vector<string> oldCachedIdentifierList;
		vector<int> oldCachedVersionTable;
//		unordered_map<string, int> oldCachedIdentifierHashMap;
		int oldPhiFlag;
		
		storeOldCachedVersion(oldCachedVersionTable);
		cacheVersionTable();
		oldPhiFlag = phiFlag;

		currentBlock = thenBlock;

		int jumpToElse=-1;

		if (symbol == thenToken)
		{
			Next();

			BasicBlock *joinBlock = new BasicBlock();
			joinBlockStack.push(joinBlock);
			
			phiFlag = 1;
				
			statSequence();

			restoreVersionTableFromCache();

			currentBlock->next.push_back(joinBlock);

			if (symbol == elseToken)
			{
				
				instrJumpToFi = createIntermediateCode("bra", Result(), Result());   //Result parameters are dummy
				thenBlock->addInstruction(instrJumpToFi);

				jumpToElse = currentCodeAddress;

				BasicBlock * elseBlock = new BasicBlock();
				ifBlock->next.push_back(elseBlock);
				ifBlock->dominates.push_back(elseBlock); 

				currentBlock = elseBlock;
				phiFlag = 2;  //indicated code in else basic blog

				Next();
				
				statSequence();


				restoreVersionTableFromCache();
				
				currentBlock->next.push_back(joinBlock);
				
			}

			if (symbol == fiToken)
			{
				if (jumpToElse == -1)   //incidates no else block
				{
					jumpToElse = currentCodeAddress;
					ifBlock->next.push_back(joinBlock);
				}
				ifBlock->dominates.push_back(joinBlock);
				currentBlock = joinBlock;

			//	intermediateCodelist[instrJumpToElse.address].operand[1] = "(" + to_string(jumpToElse) + ")";
				intermediateCodelist[instrJumpToElse.address].version[1] = (long long)(ifBlock->next[1]);
				intermediateCodelist[instrJumpToElse.address].operandType[1] = "JumpAddr";
				if (instrJumpToFi.opcode.length() > 0)
				{
					//intermediateCodelist[instrJumpToFi.address].operand[0]= "(" + to_string(currentCodeAddress) + ")";
					intermediateCodelist[instrJumpToFi.address].version[0] = (long long)(thenBlock->next[0]);
					intermediateCodelist[instrJumpToFi.address].operandType[0] = "JumpAddr";
				}

				joinBlockStack.pop();
				loadOldCachedVersion(oldCachedVersionTable);
				phiFlag = oldPhiFlag;

				commitPhi(joinBlock);
				Next();

			}
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
		else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::whileStatement()
{
	if (symbol == whileToken)
	{
		BasicBlock *joinBlock = new BasicBlock();
		joinBlockStack.push(joinBlock);

		currentBlock->next.push_back(joinBlock);
		currentBlock->dominates.push_back(joinBlock);
		currentBlock = joinBlock;

		Next();
		int topAddress = currentCodeAddress;
		IntermediateCode  instrJumpToOd=relation();
		int oldPhiFlag = phiFlag;
		int oldWhileStartAddr = whileStartAddr;
		whileStartAddr = currentCodeAddress;

	//	vector<string> oldCachedIdentifierList;
		vector<int> oldCachedVersionTable;
	//	unordered_map<string, int> oldCachedIdentifierHashMap;

		storeOldCachedVersion(oldCachedVersionTable);
		cacheVersionTable();
		

		if (symbol == doToken)
		{
			BasicBlock *doBlock = new BasicBlock();
			joinBlock->next.push_back(doBlock);
			joinBlock->dominates.push_back(doBlock);
			currentBlock = doBlock;

			Next();
			phiFlag = 3;

			statSequence();
			
			if (symbol == odToken) {
				Next();

				IntermediateCode instrJumpToTop = createIntermediateCode("bra", Result(), Result());  //Result parameters are dummy
				doBlock->addInstruction(instrJumpToTop);

			//	intermediateCodelist[instrJumpToTop.address].operand[0] = "(" + to_string(topAddress) + ")";
				intermediateCodelist[instrJumpToTop.address].version[0] = (long long)joinBlock;
				intermediateCodelist[instrJumpToTop.address].operandType[0] = "JumpAddr";

				BasicBlock *followBlock = new BasicBlock();
			//	intermediateCodelist[instrJumpToOd.address].operand[1] = "("+to_string(currentCodeAddress)+")";
				intermediateCodelist[instrJumpToOd.address].version[1] = (long long)followBlock;
				intermediateCodelist[instrJumpToOd.address].operandType[1] = "JumpAddr";

				doBlock->next.push_back(joinBlock);
				
				joinBlock->next.push_back(followBlock);
				joinBlock->dominates.push_back(followBlock);
				currentBlock = followBlock;

				

				joinBlockStack.pop();
				whileStartAddr = oldWhileStartAddr;
				phiFlag = oldPhiFlag;

				restoreVersionTableFromCache();
				loadOldCachedVersion(oldCachedVersionTable);

				commitPhi(joinBlock);
			} 
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
		else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::returnStatement()
{
	if (symbol == returnToken)
	{
		Next();

		expression();
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::statement()
{
	try
	{
		ident();

		string identifierName = scanner->Id2String(scanner->GetId());

		if (identifierName == "OutputNum")
		{
			if (symbol == openParenToken)
			{
				Next();
				Result x=ident();

				if (symbol == closeParenToken)
				{
					OutputNum(x);
					Next();
				}
			}
		}
		else if (identifierName == "InputNum")
		{
		//	Next();

			if (symbol == openParenToken)
			{
				Next();

				if (symbol == closeParenToken)
				{
					InputNum();
					Next();
				}
			}
		}
		else if (identifierName == "OutputNewLine")
		{
			//Next();

			if (symbol == openParenToken)
			{
				Next();

				if (symbol == closeParenToken)
				{
					OutputNewLine();
					Next();
				}
			}
		}
	}
	catch (SyntaxException exception)
	{
		try
		{
			assignment();
		}
		catch (SyntaxException e)
		{
			try
			{
				funcCall();
			}
			catch (SyntaxException e)
			{
				try
				{
					ifStatement();
				}
				catch (SyntaxException e)
				{
					try
					{
						whileStatement();
					}
					catch (SyntaxException e)
					{
						returnStatement();
					}
				}
			}
		}
	}
}

void Parser::statSequence()
{
	statement();
	
	while (symbol == semiToken)
	{
		Next();
		statement();
	}
}

vector<int> Parser::typeDecl()
{
	vector<int> dimension;
	if (symbol == varToken)
	{
		Next();
		return dimension;
	}
	else if (symbol == arrToken)
	{
		Next();
		
		if (symbol != openBracketToken) throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());

		while (symbol == openBracketToken)
		{
			Result x=number();
			dimension.push_back(x.value);

			if (symbol == closeBracketToken) Next();
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
		return dimension;
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::varDecl()
{

	vector<int> dimension=typeDecl();
	Result x=ident();          //x.kind info isnt valid here
	updateScope(dimension, x);

	while (symbol == commaToken)
	{
		Next();
		x=ident();
		updateScope(dimension, x);
	}

	if (symbol == semiToken) Next();
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::funcDecl()
{
	if (symbol == funcToken || symbol == procToken)
	{
		currentScope = new Scope();
		functions.push_back(currentScope);

		ident();

		if (symbol != semiToken)
		{
			formalParam();
		}

		if (symbol == semiToken)
		{
			Next();
			funcBody();

			if (symbol == semiToken) Next();
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::formalParam()
{
	if (symbol == openParenToken)
	{
		Next();
		
		if (symbol != closeParenToken)
		{
			ident();

			while (symbol == commaToken)
			{
				Next();
				ident();
			}
		}

		if (symbol == closeParenToken) Next();
		else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::funcBody()
{
	while (symbol != beginToken)
	{
		varDecl();
	}

	if (symbol == beginToken)
	{
		statSequence();

		if (symbol == endToken) Next();
		else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::computation()
{
	if (symbol == mainToken)
	{
		global = new Scope();
		currentScope = global;

		root = new BasicBlock();
		currentBlock = root;

		Next();

		while (symbol == varToken || symbol == arrToken)
		{
			varDecl();
		}
		
		while (symbol == funcToken || symbol == procToken)
		{
			funcDecl();
		}

		currentScope = global;

		if (symbol == beginToken)
		{
			Next();
			statSequence();

			if (symbol == endToken)
			{
				Next();
				if (symbol == periodToken) return;
				else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
			}
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
		else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::InputNum()
{
	IntermediateCode instr = createIntermediateCode("read", Result(), Result());  //Result parameters are dummy
	currentBlock->addInstruction(instr);
	return;
}

void Parser::OutputNum(Result x)
{
	IntermediateCode instr=createIntermediateCode("write", x, Result());   // 2nd parameter is dummy
	currentBlock->addInstruction(instr);
	return;
}

void Parser::updateScope(vector<int>& dimension, Result & x)
{
	if (currentScope->identifierHashMap.find(scanner->Id2String(x.address)) != currentScope->identifierHashMap.end())
	{
		throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber(), "Redefinition of Identifier");
	}
	
	if (dimension.size() == 0)
	{
		currentScope->variableList.push_back(scanner->Id2String(x.address));
		currentScope->versionTable.push_back(-1);   // initial version -1;
		currentScope->identifierHashMap.insert(make_pair(scanner->Id2String(x.address), currentScope->variableList.size()-1));
	}
	else
	{
		currentScope->arrayList.push_back(scanner->Id2String(x.address));
		currentScope->arrayDimensions.push_back(dimension);
		currentScope->identifierHashMap.insert(make_pair(scanner->Id2String(x.address), currentScope->arrayList.size() - 1));
	}
}

Result Parser::accessArray(vector<Result>& dimension, Result & x)
{
//	currentScope->arrayList.
	return Result();
}

void Parser::determineType(Result & x)
{
	string identifier = scanner->Id2String(x.address);
	x.address = getInScopeID(x.address);
	if (x.address < currentScope->arrayList.size() && currentScope->arrayList[x.address].compare(identifier) == 0)
		x.kind == "array";
}

void Parser::OutputNewLine()
{
	IntermediateCode instr = createIntermediateCode("writeNL", Result(), Result());  //Result parameters are dummy
	currentBlock->addInstruction(instr);
	return;
}

Result Parser::compute(int op, Result x, Result y)
{
	if (x.kind.compare("const") == 0 && y.kind.compare("const") == 0)
	{
		if (op == plusToken)
			x.value += y.value;
		else if (op == minusToken)
			x.value -= y.value;
		else if (op == timesToken)
			x.value *= y.value;
		else if (op == divToken)
			x.value /= y.value;
	}
	else
	{
		IntermediateCode instr;

		instr = createIntermediateCode(op, x, y);
		currentBlock->addInstruction(instr);
		x.kind = "IntermediateCode";
		x.address = instr.address;
	}
	return x;
}

int Parser::getVersion(int id)
{
	if (id < currentScope->versionTable.size())
		return currentScope->versionTable[id];
	throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber(), "version not found\n");
}

void Parser::updateVersion(int id, int version)
{
	if (id < currentScope->versionTable.size())
		currentScope->versionTable[id] = version;
	else
		throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber(), "identifier not found\n");
}

Parser::Parser(Scanner *scanner)
{
	this->scanner = scanner;
	currentCodeAddress = 0;
	phiFlag = 0;
}

/*void Parser::printIntermediateCode(IntermediateCode instr)
{
	cout << instr.address<<' '<<':'<<' ';
	cout << instr.opcode;
	for (int i = 0; i < MAXOPERANDLENGTH && (instr.operand[i].length() > 0 || instr.operandType->length()>0)  ; i++)
	{
		if (instr.operandType[i].compare("JumpAddr") == 0)
		{
			BasicBlock * nextBlock = (BasicBlock *)(instr.version[i]);
			instr.operand[i] = to_string(nextBlock->id);
		}
		cout << ' ' << instr.operand[i];
		if (instr.operandType[i].compare("var") == 0)
			cout << '_' << instr.version[i];
	}
	cout <<endl;
}*/

void Parser::Parse()
{
	try
	{
		Next();

		computation();
	}
	catch (SyntaxException exception)
	{
		throw exception;
	}
}

IntermediateCode Parser::createIntermediateCode(int op, Result x, Result y)
{
	IntermediateCode instr;
	instr.address = currentCodeAddress++;

	switch (op) {
	case plusToken: instr.opcode = "add";
		break;
	case minusToken: instr.opcode = "sub";
		break;
	case timesToken: instr.opcode = "mul";
		break;
	case divToken: instr.opcode = "div";
		break;
	case becomesToken: instr.opcode = "mov";
		break;
	case eqlToken: instr.opcode = "bne";
		break;
	case neqToken: instr.opcode = "beq";
		break;
	case lssToken: instr.opcode = "bge";
		break;
	case geqToken: instr.opcode = "blt";
		break;
	case leqToken: instr.opcode = "bgt";
		break;
	case gtrToken: instr.opcode = "ble";
		break;
	}

	if (x.kind.compare("const")==0)
		instr.operand[0] = to_string(x.value);
	else if (x.kind.compare("var")==0)
	{ 
		instr.operand[0] = string(scanner->Id2String(x.address));
		instr.version[0] = getVersion(x.address);
	}	
	else if (x.kind.compare("IntermediateCode") == 0)
	{
		instr.operand[0] = "(" + to_string(x.address) + ")";
		instr.version[0] = x.address;
	}
		

	if (y.kind.compare("const")==0)
		instr.operand[1] = to_string(y.value);
	else if (y.kind.compare("var") == 0)
	{
		instr.operand[1] = string(scanner->Id2String(y.address));
		instr.version[1] = getVersion(y.address);
	}
	else if (y.kind.compare("IntermediateCode") == 0)
	{
		instr.operand[1] = "(" + to_string(y.address) + ")";
		instr.version[1] = y.address;
	}
		

	instr.operandType[0] = x.kind;
	instr.operandType[1] = y.kind;

	intermediateCodelist.push_back(instr);
	return instr;
}

IntermediateCode Parser::createIntermediateCode(string opcode, Result x, Result y)
{
	IntermediateCode instr;
	instr.address = currentCodeAddress++;

	instr.opcode = opcode;

	if (x.kind.compare("const") == 0)
		instr.operand[0] = to_string(x.value);
	else if (x.kind.compare("var") == 0)
	{
		instr.operand[0] = string(scanner->Id2String(x.address));
		instr.version[0] = getVersion(x.address);
	}
	else if (x.kind.compare("IntermediateCode") == 0)
	{
		instr.operand[0] = "(" + to_string(x.address) + ")";
		instr.version[0] = x.address;
	}
		

	if (y.kind.compare("const") == 0)
		instr.operand[1] = to_string(y.value);
	else if (y.kind.compare("var") == 0)
	{
		instr.operand[1] = string(scanner->Id2String(y.address));
		instr.version[1] = getVersion(y.address);
	}
	else if (y.kind.compare("IntermediateCode") == 0)
	{
		instr.operand[1] = "(" + to_string(y.address) + ")";
		instr.version[1] = y.address;
	}
		

	instr.operandType[0] = x.kind;
	instr.operandType[1] = y.kind;

	intermediateCodelist.push_back(instr);
	return instr;
}

void Parser::printAllIntermediateCode()
{
	for (int i = 0; i < intermediateCodelist.size(); i++)
	{
		printIntermediateCode(intermediateCodelist[i]);
	}
}

void Parser::updatePhi(Result x)
{
	if (joinBlockStack.empty())
		return;
	BasicBlock* joinBlock = joinBlockStack.top();

	string varName = scanner->Id2String(x.address);
	IntermediateCode instr;

	int i = 0;
	for (; i < joinBlock->instructionAddrList.size(); i++)
	{
		instr = intermediateCodelist[joinBlock->instructionAddrList[i]];
		if (instr.opcode.compare("phi") == 0 && instr.operand[0].compare(varName)==0)
			break;
	}
	if (i==joinBlock->instructionAddrList.size())
	{
		//create a new phi instruction
		IntermediateCode instr; 
		if (phiFlag == 1) //if it is in ifBlock
		{
			instr= createIntermediateCode("phi", x, x);
			//manually handle version for Phi destination
			instr.version[0] = currentCodeAddress - 1;
			intermediateCodelist[instr.address].version[0] = currentCodeAddress - 1;

			//add operand 3
			instr.operand[2] = varName;
			instr.version[2] = cachedVersionTable[x.address];
			instr.operandType[2] = "var";
			intermediateCodelist[instr.address].operand[2] = varName;
			intermediateCodelist[instr.address].version[2] = cachedVersionTable[x.address];
			intermediateCodelist[instr.address].operandType[2] = "var";
		}
		else if (phiFlag == 2 || phiFlag==3)  //if it is a elseBlock or whileBlock
		{
			instr = createIntermediateCode("phi", x,Result());  //2nd param is dummy
			//manually handle version for Phi destination
			instr.version[0] = currentCodeAddress - 1;
			intermediateCodelist[instr.address].version[0] = currentCodeAddress - 1;

			//add 1st param from cache
			instr.operand[1] = varName;
			instr.version[1] = cachedVersionTable[x.address];
			instr.operandType[1] = "var";
			intermediateCodelist[instr.address].operand[1] = varName;
			intermediateCodelist[instr.address].version[1] = cachedVersionTable[x.address];
			intermediateCodelist[instr.address].operandType[1] = "var";
			//add 2nd param
			instr.operand[2] = varName;
			instr.version[2] = getVersion(x.address);
			instr.operandType[2] = "var";
			intermediateCodelist[instr.address].operand[2] = varName;
			intermediateCodelist[instr.address].version[2] = getVersion(x.address);
			intermediateCodelist[instr.address].operandType[2] = "var";

			if (phiFlag == 3)  //if while loop
			{
				renameLoopOccurances(x, currentCodeAddress - 1);
			}
		}
		if (phiFlag == 3)  //if while Loop
			joinBlock->addInstructionInBegining(instr);
		else
			joinBlock->addInstruction(instr);
	}
	else
	{
		//update existing phi instruction
		if (phiFlag == 1)  //if it is in ifBlock
		{
			
			instr.version[1] = getVersion(x.address);
			intermediateCodelist[instr.address].version[1] = getVersion(x.address);
		}
		else if (phiFlag == 2 || phiFlag==3) //if it is a elseBlock or whileBlock
		{
			instr.version[2] = getVersion(x.address);
			intermediateCodelist[instr.address].version[2] = getVersion(x.address);
		}
		
	}
}

void Parser::cacheVersionTable()
{
//	cachedIdentifierHashMap = scanner->identifierHashMap;
//	cachedIdentifierList = scanner->identifierList;
	cachedVersionTable = currentScope->versionTable;
}

void Parser::storeOldCachedVersion(vector<int>& versionTable)
{
//	identifierList = cachedIdentifierList;
//	identifierHashMap = cachedIdentifierHashMap;
	versionTable = cachedVersionTable;
}

void Parser::loadOldCachedVersion(vector<int>& versionTable)
{
//	cachedIdentifierList= identifierList;
//	cachedIdentifierHashMap= identifierHashMap;
	cachedVersionTable= versionTable;
}

void Parser::restoreVersionTableFromCache()
{
//	scanner->identifierList = cachedIdentifierList;
	currentScope->versionTable = cachedVersionTable;
//	scanner->identifierHashMap = cachedIdentifierHashMap;
}

void Parser::renameLoopOccurances(Result x,int newVersion)
{
	int oldVersion = cachedVersionTable[x.address];
	string varName = scanner->Id2String(x.address);
	for (int i = whileStartAddr; i < currentCodeAddress-1; i++)  // the -1 is to exclude current phi instruction
	{
		for(int j=0 ; j<3 ;j ++)
		if (intermediateCodelist[i].operand[j].compare(varName)==0 && intermediateCodelist[i].version[j] == oldVersion)
			intermediateCodelist[i].version[j] = newVersion;
	}
}

void Parser::commitPhi(BasicBlock * joinBlock)
{
	IntermediateCode instr;
	for (int i = 0; i < joinBlock->instructionAddrList.size(); i++)
	{
		instr= intermediateCodelist[joinBlock->instructionAddrList[i]];
		if (instr.opcode.compare("phi") == 0)
		{
			int varId = scanner->string2Id(instr.operand[0]);
			updateVersion(varId, instr.version[0]);

			if (!joinBlockStack.empty()) //update phi in the outer joinBlock
			{
				Result x;
				x.kind = "var";
				x.address = varId;
				updatePhi(x);
			}
		}
	}
}

void Parser::printCodesByBlocks(BasicBlock *cfgNode)
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

void Parser::outputVCGFile(BasicBlock *cfgNode)
{
	if (cfgNode == NULL)
	{
		cfgNode = root;
		visitedNodes.clear();
		cout<<"graph: { title: \"control flow graph\""<<endl;
		cout << "manhattan_edges: yes" << endl;
		cout << "smanhattan_edges: yes" << endl;
	}
	cout << "node: {" << endl;
	cout << "title: \"" << cfgNode->id << "\"" << endl;
	cout << "label: \"" << cfgNode->id << "[" << endl;
	visitedNodes.insert(cfgNode);
	for (int i = 0; i < cfgNode->instructionAddrList.size(); i++)
	{
		printIntermediateCode(intermediateCodelist[cfgNode->instructionAddrList[i]]);
	}
	cout << "]\"" << endl<<"}";
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
		outputDominatorTree();
	}
}

void Parser::outputDominatorTree(BasicBlock * cfgNode)
{
	if (cfgNode == NULL)
	{
		cfgNode = root;
		visitedNodes.clear();
	//	cout << "graph: { title: \"Dominator Tree\"" << endl;
	//	cout << "manhattan_edges: yes" << endl;
	//	cout << "smanhattan_edges: yes" << endl;
	}
	cout << "node: {" << endl;
	cout << "title: \"" << 0<<cfgNode->id << "\"" << endl;
	cout << "label: \"" << cfgNode->id;
	cout << "\""  << "}"<<endl;
	visitedNodes.insert(cfgNode);
	for (int i = 0; i < cfgNode->dominates.size(); i++)
	{
		cout << "edge: { sourcename: \"" <<0<< cfgNode->id << "\"  targetname: \"" <<0<< cfgNode->dominates[i]->id;
		cout << "\" color: green }" << endl;
		if (visitedNodes.find(cfgNode->dominates[i]) == visitedNodes.end())
		{
			outputDominatorTree(cfgNode->dominates[i]);
		}

	}
	if (cfgNode == root)
	{
		cout << "}";
	}
}

vector<IntermediateCode>& Parser::getIntermediateCodelist()
{
	return intermediateCodelist;
}

BasicBlock * Parser::getCFGTreeRoot()
{
	return root;
}
