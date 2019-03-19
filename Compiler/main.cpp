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
		Scanner *scanner = new Scanner("test/test016.txt");

		Parser *parser= new Parser(scanner);
		parser->Parse();
		EliminateRedundency *step2 = new EliminateRedundency(parser);
		step2->copyPropagation();
		step2->updateVersion();
		step2->CSE();
		step2->printCodesByBlocks();

	/**/freopen_s(&stream, "cfg.vcg", "w", stdout);
		parser->outputVCGFile();

		freopen_s(&stream, "cfg after step2.vcg", "w", stdout);
		step2->outputVCGFile();
		freopen_s(&stream, "CONOUT$", "w", stdout);
	//	parser->printAllIntermediateCode();
	//	cout << "..........complete........." << endl << endl;
	//	parser->printCodesByBlocks();
		
	/**/	int currentCodeAddress = parser->getCurrentCodeAddress();

		RegisterAllocator registerAllocator(step2, currentCodeAddress);
		registerAllocator.start(step2->getGlobalScope()->root);
		step2->getGlobalScope()->setRegisters(registerAllocator.getAllAssignedRegisters());
		currentCodeAddress = registerAllocator.getCurrentCodeAddress();

		int lastVirtualRegisterNumber = registerAllocator.getLastVirtualRegisterNumber();
		vector<Scope *> functions = step2->getFunctions();
		for (Scope * function : functions)
		{
			RegisterAllocator registerAllocator(step2, currentCodeAddress, lastVirtualRegisterNumber+1);
			registerAllocator.start(function->root);

			cout << "..........after allocation........." << endl << endl;
			function->setRegisters(registerAllocator.getAllAssignedRegisters());

			currentCodeAddress = registerAllocator.getCurrentCodeAddress();
			lastVirtualRegisterNumber = registerAllocator.getLastVirtualRegisterNumber();
		}
		
		CodeGenerator codeGenerator(step2->getGlobalScope(), step2->getFunctions(), step2->getIntermediateCodeList(), currentCodeAddress);
		codeGenerator.generate();

		cout << "Press any key to execute..." << endl;

		if (getchar())
		{
			codeGenerator.execute();
		} 
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