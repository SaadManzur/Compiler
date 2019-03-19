#include "Parser.h"
#include "Common.h"
#include<iostream>
#include<cstring>
#include<cassert>

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
		if (x.kind.compare("IntermediateCode") == 0)
		{
			IntermediateCode instr=createIntermediateCode("load", x, Result());
			currentBlock->addInstruction(instr);
			x.address = instr.address;
		}
		else if(x.isGlobal==1 && 
			currentScope->globalVarsModifies.find(x.address)==currentScope->globalVarsModifies.end())
		{
			if (currentScope->globalVarsUses.find(x.address) == currentScope->globalVarsUses.end())
			{
				updateVersion(x.address, currentCodeAddress, x.isGlobal);
				IntermediateCode instr = createIntermediateCode("LDW", x, Result());
				currentBlock->addInstruction(instr);


				currentScope->globalVarsUses.insert(x.address);
			}
			
		}
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
				x=funcCall();
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

pair<int,int> Parser::getInScopeID(int id)
{
	int isGlobal = 0;
	string  str= scanner->Id2String(id);
	auto it=currentScope->identifierHashMap.find(str);
	if (it == currentScope->identifierHashMap.end())
	{
		it = global->identifierHashMap.find(str);
		if (it == global->identifierHashMap.end())
			throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber(), "Identifier not found");
		else
			isGlobal = 1;
	}
	return make_pair(it->second,isGlobal);
}

void Parser::assignment()
{
	int start, end;
	Result x, y;
	IntermediateCode instr;
	if (symbol == letToken)
	{
		Next();
		start = currentBlock->instructionAddrList.size();
		y=designator();
		end = currentBlock->instructionAddrList.size();
		if (symbol == becomesToken)
		{
			Next();
			x=expression();
			if (y.kind == "var")
			{
				updateVersion(y.address, currentCodeAddress,y.isGlobal);
				instr = createIntermediateCode(becomesToken, x, y);
				if(y.isGlobal==1)
					currentScope->globalVarsModifies.insert(y.address);
			}
			else if (y.kind.compare("IntermediateCode") == 0)
			{
				reOrderInstructions(start,end);
				instr = createIntermediateCode("store", x, y);
			//	y.address = instr.address;
			}
			if (y.kind == "var" && !joinBlockStack.empty()) //update phi
			{
				updatePhi(y);
			}
			else if (intermediateCodelist[instr.version[1]].opcode.compare("adda")==0 && !joinBlockStack.empty())
				insertKill(instr);
			currentBlock->addInstruction(instr);
			
		}
		else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

Result Parser::funcCall()
{
	Result x;
	if (symbol == callToken)
	{
		Next();
		Result y=ident();

		string identifierName = scanner->Id2String(y.address);

		if (identifierName == "OutputNum")
		{
			if (symbol == openParenToken)
			{
				Next();
				x = expression();

				if (symbol == closeParenToken)
				{
					OutputNum(x);
					Next();
					return x;
				}
				else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
			}
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
		else if (identifierName == "InputNum")
		{
			//	Next();

			if (symbol == openParenToken)
			{
				Next();

				if (symbol == closeParenToken)
				{
					x=InputNum();
					Next();
					return x;
				}
				else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
			}
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
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
					return x;
				}
				else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
			}
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
	//	Result x;
		IntermediateCode instr;
		storeGlobalVars(identifierName);
		if (symbol == openParenToken)
		{
			Next();
		/*	if (isfunction(identifierName))
			{
				instr = createIntermediateCode("push", Result(), Result());
				currentBlock->addInstruction(instr);
			}
        */
			
			if(symbol != closeParenToken)
			{
				vector<Result> params;
				x=expression();
				params.push_back(x);
				

				while (symbol == commaToken)
				{
					Next();
					x=expression();
					params.push_back(x);
				}
				for (int i = 0; i < params.size(); i++)
				{
					instr = createIntermediateCode("push", params[i], Result());
					currentBlock->addInstruction(instr);
				}

			}
			createAndAddCode("call", identifierName,"");
			x = createAndAddCode("pop", "", "");
			if (symbol == closeParenToken) Next();
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
		else
		{
			x=createAndAddCode("call", identifierName, "");
			x = createAndAddCode("pop", "", "");
			//throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
		loadGlobalVars(identifierName);
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	return x;
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
		vector<int> oldCachedGlobalVersionTable;
//		unordered_map<string, int> oldCachedIdentifierHashMap;
		int oldPhiFlag;
		
		storeOldCachedVersion(oldCachedVersionTable, oldCachedGlobalVersionTable);
		cacheVersionTable();
		oldPhiFlag = phiFlag;

		currentBlock = thenBlock;

		int jumpToElse=-1;

		if (symbol == thenToken)
		{
			Next();

			BasicBlock *joinBlock = new BasicBlock();
			joinBlock->isJoinBlock = true;
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
				loadOldCachedVersion(oldCachedVersionTable, oldCachedGlobalVersionTable);
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
		joinBlock->isLoopHeader = true;
		joinBlockStack.push(joinBlock);

		currentBlock->next.push_back(joinBlock);
		currentBlock->dominates.push_back(joinBlock);
		currentBlock = joinBlock;

		Next();
		int topAddress = currentCodeAddress;
		int oldWhileStartAddr = whileStartAddr;
		whileStartAddr = currentCodeAddress;
		IntermediateCode  instrJumpToOd=relation();
		int oldPhiFlag = phiFlag;
		

	//	vector<string> oldCachedIdentifierList;
		vector<int> oldCachedVersionTable;
		vector<int> oldCachedGlobalVersionTable;
	//	unordered_map<string, int> oldCachedIdentifierHashMap;

		storeOldCachedVersion(oldCachedVersionTable, oldCachedGlobalVersionTable);
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
				currentBlock->addInstruction(instrJumpToTop);

			//	intermediateCodelist[instrJumpToTop.address].operand[0] = "(" + to_string(topAddress) + ")";
				intermediateCodelist[instrJumpToTop.address].version[0] = (long long)joinBlock;
				intermediateCodelist[instrJumpToTop.address].operandType[0] = "JumpAddr";

				BasicBlock *followBlock = new BasicBlock();
			//	intermediateCodelist[instrJumpToOd.address].operand[1] = "("+to_string(currentCodeAddress)+")";
				intermediateCodelist[instrJumpToOd.address].version[1] = (long long)followBlock;
				intermediateCodelist[instrJumpToOd.address].operandType[1] = "JumpAddr";

				currentBlock->next.push_back(joinBlock);
				
				joinBlock->next.push_back(followBlock);
				joinBlock->dominates.push_back(followBlock);
				currentBlock = followBlock;

				

				joinBlockStack.pop();
				whileStartAddr = oldWhileStartAddr;
				phiFlag = oldPhiFlag;

				restoreVersionTableFromCache();
				loadOldCachedVersion(oldCachedVersionTable, oldCachedGlobalVersionTable);

				commitPhi(joinBlock);
			} 
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
		else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

int Parser::returnStatement()
{
	Result y;
	y.kind = "const";
	y.value = currentScope->arguments.size() + 2;

	if (symbol == returnToken)
	{
		Next();

		Result x=expression();
		IntermediateCode instr = createIntermediateCode("STS", x, y);
		currentBlock->addInstruction(instr);

		flushGlobalVariables();
		createAndAddCode("epilogue", string(), string());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	return 0;
}

int Parser::statement()
{
	int noReturnStat=1;
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
						noReturnStat=returnStatement();
					}
				}
			}
		}
	}
	return noReturnStat;
}

int Parser::statSequence()
{
	int noReturnStat = 1;
	statement();
	
	while (symbol == semiToken)
	{
		Next();
		noReturnStat=statement();
	}
	return noReturnStat;
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
			Next();
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
		currentScope->root = new BasicBlock();
		currentBlock = currentScope->root;

		currentScope->functionType = symbol == funcToken ? 1 : 0;

		Next();
		Result x=ident();

		currentScope->functionName = scanner->Id2String(x.address);   // add a function add func similar to update scope

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
	Result x;
	vector<int > dim;
	if (symbol == openParenToken)
	{
		Next();
		
		if (symbol != closeParenToken)
		{
			x=ident();
			updateScope(dim, x);

			determineType(x);   //need get the inscopeId;
			currentScope->numOfArg++;
			currentScope->arguments.push_back(x.address);

			while (symbol == commaToken)
			{
				Next();
				x=ident();
				updateScope(dim, x);

				determineType(x);   //need get the inscopeId;
				currentScope->numOfArg++;
				currentScope->arguments.push_back(x.address);
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
		createAndAddCode("prologue", string(), string());
		loadArguments();
		Next();
		int noReturnStat=statSequence();
		if (noReturnStat)
		{
			flushGlobalVariables();
			createAndAddCode("epilogue", string(), string());
		}
			
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

		global->root=root = new BasicBlock();
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
		currentBlock = root;

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

Result Parser::createAndAddCode(int op, Result x, Result y)
{
	IntermediateCode instr = createIntermediateCode(op, x, y);
	currentBlock->addInstruction(instr);
	x.address = instr.address;
	x.kind = "IntermediateCode";
	return x;
}

Result Parser::createAndAddCode(string opcode, string x, string y)
{
	IntermediateCode instr;
	instr.opcode = opcode;
	instr.operand[0] = x;
	instr.operand[1] = y;
	instr.operandType[0] = "reg"; //might need to change
	instr.operandType[1] = "address";  //might need to change

	instr.address = currentCodeAddress++;
	intermediateCodelist.push_back(instr);
	currentBlock->addInstruction(instr);
	Result res;
	res.address = instr.address;
	res.kind = "IntermediateCode";;
	return res;
}

Result Parser::createAndAddCode(string opcode, Result x, Result y)
{
	IntermediateCode instr = createIntermediateCode(opcode, x, y);
	currentBlock->addInstruction(instr);
	x.address = instr.address;
	x.kind = "IntermediateCode";
	return x;
}

Result Parser::InputNum()
{
	Result x;
	IntermediateCode instr = createIntermediateCode("read", Result(), Result());  //Result parameters are dummy
	currentBlock->addInstruction(instr);
	x.address = instr.address;
	x.kind= "IntermediateCode";
	return x;
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
		IntermediateCode instr = createIntermediateCode("var", Result(), Result());
		currentScope->variableList.push_back(scanner->Id2String(x.address));
		currentScope->versionTable.push_back(instr.address);   // changed from initial version -1
		currentScope->identifierHashMap.insert(make_pair(scanner->Id2String(x.address), currentScope->variableList.size()-1));
	}
	else
	{
		currentScope->arrayList.push_back(scanner->Id2String(x.address));
		currentScope->arrayDimensions.push_back(dimension);
		currentScope->identifierHashMap.insert(make_pair(scanner->Id2String(x.address), currentScope->arrayList.size() - 1));
	}
}

Result Parser::accessArray(vector<Result>& dimension, Result x)
{
//	currentScope->arrayList.
	int arrayId = x.address;
	Scope * effectiveScope = x.isGlobal ? global : currentScope;
	string arrayName = effectiveScope->arrayList[arrayId];
	int offset = effectiveScope->arrayDimensions[arrayId].back();// size() - 1;
	Result y;
	y.kind = "const";
	for (int i = dimension.size() - 2; i >= 0; i--)
	{
		y.value = offset;
		dimension[i] = createAndAddCode(timesToken, dimension[i], y);
		dimension[i] = createAndAddCode(plusToken, dimension[i + 1], dimension[i]);
		offset *= effectiveScope->arrayDimensions[arrayId][i];
	}
	y.value = 4;
	Result m=createAndAddCode(timesToken, dimension[0], y);
	Result n=createAndAddCode("add", "baseReg", arrayName + "_baseAddr");
	return createAndAddCode("adda", m, n);
}

void Parser::determineType(Result & x)
{
	string identifier = scanner->Id2String(x.address);
	pair<int, int> temp= getInScopeID(x.address);
	x.address = temp.first;
	x.isGlobal = temp.second;
	Scope * effectiveScope = x.isGlobal ? global : currentScope;
	if (x.address < effectiveScope->arrayList.size() && effectiveScope->arrayList[x.address].compare(identifier) == 0)
		x.kind = "array";
//	else if (isGlobal && x.address < global->arrayList.size() && global->arrayList[x.address].compare(identifier) == 0)
//		x.kind = "array";
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

int Parser::getVersion(Result x)
{
	assert(x.isGlobal != -1);
	Scope * effectiveScope = x.isGlobal ? global : currentScope;
	if (x.address < effectiveScope->versionTable.size())
		return effectiveScope->versionTable[x.address];
	throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber(), "version not found\n");
}

void Parser::updateVersion(int id, int version,int isGlobal)
{
	assert(isGlobal != -1);
	Scope* effectiveScope = isGlobal ? global : currentScope;
	if (id < effectiveScope->versionTable.size())
		effectiveScope->versionTable[id] = version;
	else
		throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber(), "identifier not found\n");
}

string Parser::getName(Result x)
{
	assert(x.isGlobal != -1);
	if (x.isGlobal)
		return global->variableList[x.address];
	else
		return currentScope->variableList[x.address];
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

		if (assignedRegisters.empty())
		{
			cout << ' ' << instr.operand[i];
			if (instr.operandType[i].compare("var") == 0)
				cout << '_' << instr.version[i];
		}
		else
		{
			string operand = instr.operand[i];

			if (instr.operandType[i].compare("var") == 0)
			{
				operand += "_" + to_string(instr.version[i]);
			}
				
			if (instr.operandType[i] == "var" || instr.operandType[i] == "IntermediateCode")
			{
				cout << " R" << assignedRegisters[operand];
			}
			else
			{
				cout << ' ' << instr.operand[i];
			}
		}
	}
	cout <<endl;
}*/

void Parser::Parse()
{
	try
	{
		Next();

		computation();
		secondPass();
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
	case plusToken: 
		instr.opcode = "add";
		instr.iOpcode = 0;
		break;
	case minusToken: 
		instr.opcode = "sub";
		instr.iOpcode = 1;
		break;
	case timesToken: 
		instr.opcode = "mul";
		instr.iOpcode = 2;
		break;
	case divToken: 
		instr.opcode = "div";
		instr.iOpcode = 3;
		break;
	case becomesToken: 
		instr.opcode = "mov";
		break;
	case eqlToken: 
		instr.opcode = "bne";
		break;
	case neqToken: 
		instr.opcode = "beq";
		break;
	case lssToken: 
		instr.opcode = "bge";
		break;
	case geqToken: 
		instr.opcode = "blt";
		break;
	case leqToken: 
		instr.opcode = "bgt";
		break;
	case gtrToken: 
		instr.opcode = "ble";
		break;
	}

	if (x.kind.compare("const")==0)
		instr.operand[0] = to_string(x.value);
	else if (x.kind.compare("var")==0)
	{ 
		instr.operand[0] = string(getName(x));
		instr.version[0] = getVersion(x);
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
		instr.operand[1] = string(getName(y));
		instr.version[1] = getVersion(y);
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
		instr.operand[0] = string(getName(x));
		instr.version[0] = getVersion(x);
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
		instr.operand[1] = string(getName(y));
		instr.version[1] = getVersion(y);
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

	string varName = getName(x);
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
			instr.version[2] = getCachedVersion(x); //cachedVersionTable[x.address];
			instr.operandType[2] = "var";
			intermediateCodelist[instr.address].operand[2] = varName;
			intermediateCodelist[instr.address].version[2] = getCachedVersion(x);// cachedVersionTable[x.address];
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
			instr.version[1] = getCachedVersion(x); //cachedVersionTable[x.address];
			instr.operandType[1] = "var";
			intermediateCodelist[instr.address].operand[1] = varName;
			intermediateCodelist[instr.address].version[1] = getCachedVersion(x); //cachedVersionTable[x.address];
			intermediateCodelist[instr.address].operandType[1] = "var";
			//add 2nd param
			instr.operand[2] = varName;
			instr.version[2] = getVersion(x);
			instr.operandType[2] = "var";
			intermediateCodelist[instr.address].operand[2] = varName;
			intermediateCodelist[instr.address].version[2] = getVersion(x);
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
			
			instr.version[1] = getVersion(x);
			intermediateCodelist[instr.address].version[1] = getVersion(x);
		}
		else if (phiFlag == 2 || phiFlag==3) //if it is a elseBlock or whileBlock
		{
			instr.version[2] = getVersion(x);
			intermediateCodelist[instr.address].version[2] = getVersion(x);
		}
		
	}
}

void Parser::cacheVersionTable()
{
//	cachedIdentifierHashMap = scanner->identifierHashMap;
//	cachedIdentifierList = scanner->identifierList;
	cachedVersionTable = currentScope->versionTable;
	cachedGlobalVersionTable = global->versionTable;
}

void Parser::storeOldCachedVersion(vector<int>& versionTable, vector<int>& globalVersionTable)
{
//	identifierList = cachedIdentifierList;
//	identifierHashMap = cachedIdentifierHashMap;
	versionTable = cachedVersionTable;
	globalVersionTable = cachedGlobalVersionTable;
}

void Parser::loadOldCachedVersion(vector<int>& versionTable, vector<int>& globalVersionTable)
{
//	cachedIdentifierList= identifierList;
//	cachedIdentifierHashMap= identifierHashMap;
	cachedVersionTable= versionTable;
	cachedGlobalVersionTable = globalVersionTable;
}

void Parser::restoreVersionTableFromCache()
{
//	scanner->identifierList = cachedIdentifierList;
	currentScope->versionTable = cachedVersionTable;
	global->versionTable = cachedGlobalVersionTable;
//	scanner->identifierHashMap = cachedIdentifierHashMap;
}

int Parser::functionNametoScopeId(string func)
{
	for (int i = 0; i < functions.size(); i++)
	{
		if (functions[i]->functionName.compare(func) == 0)
			return i;
	}
	return -1;
}

void Parser::secondPass(BasicBlock * cfgNode)
{
	if (cfgNode == NULL)
	{
		cfgNode = global->root;
		visitedNodes.clear();
	}
	visitedNodes.insert(cfgNode);

	IntermediateCode instr;
	for (int i = 0; i < cfgNode->instructionAddrList.size(); i++)
	{
		
		instr = intermediateCodelist[cfgNode->instructionAddrList[i]];
		if (instr.opcode.compare("call") == 0)
		{
			int id=functionNametoScopeId(instr.operand[0]);
			if (id == -1)
				throw SyntaxException("Function Undefined");
			instr.opcode = "JSR";
			instr.version[0] = (long long)functions[id]->root;
			instr.operandType[0]= "JumpAddr";
			intermediateCodelist[instr.address] = instr;

			functionCalls.push_back("edge: { sourcename: \""+to_string(cfgNode->id)+ "\"  targetname: \"" +
										to_string(functions[id]->root->id)+"\" color: Red }\n");

			if (functions[id]->functionType==1)
			{
				instr = createIntermediateCode("push", Result(), Result());
				auto it = cfgNode->instructionAddrList.begin();
				it=cfgNode->instructionAddrList.insert(it+i-functions[id]->numOfArg, instr.address);  // i+functions[id]->numOfArg
				//instr= createIntermediateCode("pop", Result(), Result());
				//cfgNode->instructionAddrList.insert(it + functions[id]->numOfArg+1, instr.address);


			}
			else
			{
				auto it = cfgNode->instructionAddrList.begin();
				cfgNode->instructionAddrList.erase(it + i + 1);
			}
		}
	}
	for (int i = 0; i < cfgNode->next.size(); i++)
	{
		if (visitedNodes.find(cfgNode->next[i]) == visitedNodes.end())
		{
			secondPass(cfgNode->next[i]);
		}

	}
	if (cfgNode == global->root)
	{
		for(int i=0;i< functions.size();i++)
			secondPass(functions[i]->root);
	}
}

void Parser::reOrderInstructions(int start, int end)
{
	int temp;
	auto it = currentBlock->instructionAddrList.begin() + start;
	for (int i = start; i < end; i++)
	{
		temp = *it;
		it=currentBlock->instructionAddrList.erase(it);
		currentBlock->instructionAddrList.push_back(temp);
	}
}

int Parser::getCachedVersion(Result x)
{
	assert(x.isGlobal != -1);
	if (x.isGlobal)
		return cachedGlobalVersionTable[x.address];
	else
		return cachedVersionTable[x.address];
}

void Parser::insertKill(IntermediateCode instr)
{
	BasicBlock * joinBlock;
	stack<BasicBlock*> backup = joinBlockStack;
	IntermediateCode instrAdda,temp;

	instrAdda = intermediateCodelist[instr.version[1]];
	assert(instr.opcode.compare("store") == 0 && instrAdda.opcode.compare("adda") == 0);
	string arrayName = intermediateCodelist[instrAdda.version[1]].operand[1];

	while (!joinBlockStack.empty())
	{
		joinBlock = joinBlockStack.top();
		joinBlockStack.pop();
		temp = createIntermediateCode("kill", Result(), Result());
		temp.operand[0] = arrayName;
		intermediateCodelist[temp.address].operand[0] = arrayName;
		joinBlock->addInstruction(temp);
	}
	joinBlockStack = backup;
}

void Parser::storeGlobalVars(string funcName)
{
	Result x;
	x.isGlobal = 1;
	x.kind = "var";
	IntermediateCode instr;
	int id = functionNametoScopeId(funcName);
	if (id == -1)
	{
		for (int i = 0; i < global->variableList.size(); i++)
		{
			x.address = i;
			instr = createIntermediateCode("STW", x, Result());
			currentBlock->addInstruction(instr);
		}
		return;
	//   throw SyntaxException("Function Undefined");
	}
		
	auto it = functions[id]->globalVarsUses.begin();
	for (; it != functions[id]->globalVarsUses.end(); it++)
	{
		x.address = *it;
		instr = createIntermediateCode("STW", x, Result());
		currentBlock->addInstruction(instr);
	}
}

void Parser::loadGlobalVars(string funcName)
{
	Result x;
	x.isGlobal = 1;
	x.kind = "var";
	IntermediateCode instr;
	int id = functionNametoScopeId(funcName);
	if (id == -1)
	{
		for (int i = 0; i < global->variableList.size(); i++)
		{
			x.address = i;
			updateVersion(x.address, currentCodeAddress, x.isGlobal);
			instr = createIntermediateCode("LDW", x, Result());
			currentBlock->addInstruction(instr);
		}
		return;
		//   throw SyntaxException("Function Undefined");
	}

	auto it = functions[id]->globalVarsModifies.begin();
	for (; it != functions[id]->globalVarsModifies.end(); it++)
	{
		x.address = *it;
		updateVersion(x.address, currentCodeAddress, x.isGlobal);
		instr = createIntermediateCode("LDW", x, Result());
		currentBlock->addInstruction(instr);
	}
}

void Parser::flushGlobalVariables()
{
	Result x;
	x.isGlobal = 1;
	x.kind = "var";
	IntermediateCode instr;
	if (currentScope == global)
		return;
	for (auto it = currentScope->globalVarsModifies.begin(); it !=currentScope->globalVarsModifies.end(); it++)
	{
		x.address = *it;
		instr = createIntermediateCode("STW", x, Result());
		currentBlock->addInstruction(instr);
	}
}

void Parser::loadArguments()
{
	Result x, y;
	x.isGlobal = 0;
	x.kind = "var";
	
	y.kind = "const";
	y.value = currentScope->arguments.size() + 1;

	IntermediateCode instr;
	for (int i = 0; i < currentScope->arguments.size(); i++, y.value--)
	{
		x.address = currentScope->arguments[i];
		updateVersion(x.address, currentCodeAddress, x.isGlobal);
		instr = createIntermediateCode("LDS", x, y);
		currentBlock->addInstruction(instr);

	}
}



void Parser::renameLoopOccurances(Result x,int newVersion)
{
	int oldVersion = getCachedVersion(x); //cachedVersionTable[x.address];
	string varName = getName(x);
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
		//	int varId = scanner->string2Id(instr.operand[0]);
			pair<int, int> scopeInfo= getInScopeID(scanner->string2Id(instr.operand[0]));
			int varId = scopeInfo.first;
			updateVersion(varId, instr.version[0], scopeInfo.second);

			if (!joinBlockStack.empty()) //update phi in the outer joinBlock
			{
				Result x;
				x.kind = "var";
				x.address = varId;
				x.isGlobal = scopeInfo.second;
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
/*	if (cfgNode == root)
	{
	//	cout << "}";
		outputDominatorTree();
	}
*/
	if (cfgNode == global->root)
	{
		for (int i = 0; i < functions.size(); i++)
			outputVCGFile(functions[i]->root);
		for (int i = 0; i < functionCalls.size(); i++)
			cout << functionCalls[i];
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

IntermediateCode Parser::getIntermediateCode(int address)
{
	return intermediateCodelist[address];
}

BasicBlock * Parser::getCFGTreeRoot()
{
	return root;
}

pair<Scope*, vector<Scope*>> Parser::getScopeInfo()
{
	return make_pair(global, functions);
}

void Parser::outputFunctionCalls()
{
	for (int i = 0; i < functionCalls.size(); i++)
		cout << functionCalls[i];
}

void Parser::setRegisters(Scope *function, map<string, int> assignedRegisters)
{
	function->assignedRegisters.insert(assignedRegisters.begin(), assignedRegisters.end());
}

vector<Scope*> Parser::getFunctions()
{
	return functions;
}

int Parser::getCurrentCodeAddress()
{
	return currentCodeAddress;
}
