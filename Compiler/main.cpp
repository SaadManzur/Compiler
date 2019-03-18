#include "Common.h"
#include "Scanner.h"
#include "Parser.h"
#include "RegisterAllocator.h"
#include "CodeGenerator.h"
#include <iostream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include "EliminateRedundency.h"
#include <cassert>
using namespace std;

int main()
{
	Logger logger;
	std::FILE *stream;
	try
	{
		Scanner *scanner = new Scanner("test012.txt");

		Parser *parser= new Parser(scanner);
		parser->Parse();
		EliminateRedundency *step2 = new EliminateRedundency(parser);
		step2->copyPropagation();
		step2->updateVersion();
		step2->CSE();
		step2->printCodesByBlocks();
		
	//	parser->printAllIntermediateCode();
	//	cout << "..........complete........." << endl << endl;
	//	parser->printCodesByBlocks();
		
		RegisterAllocator registerAllocator(step2);
		registerAllocator.start(step2->getGlobalScope()->root);
		step2->getGlobalScope()->setRegisters(registerAllocator.getAllAssignedRegisters());

		vector<Scope *> functions = step2->getFunctions();
		for (Scope * function : functions)
		{
			RegisterAllocator registerAllocator(step2);
			registerAllocator.start(function->root);

			cout << "..........after allocation........." << endl << endl;
			function->setRegisters(registerAllocator.getAllAssignedRegisters());
		}
		
		CodeGenerator codeGenerator(step2->getGlobalScope(), step2->getFunctions(), step2->getIntermediateCodeList());
		codeGenerator.generate();

		freopen_s(&stream, "cfg.vcg", "w", stdout);
		parser->outputVCGFile();

		freopen_s(&stream, "cfg after step2.vcg", "w", stdout);
		step2->outputVCGFile();
	}
	catch (SyntaxException exception)
	{
		logger.Error(exception.getMessage());
		cout << exception.getMessage() << endl;
	}
	
	getchar();
	getchar();

	return 0;
}