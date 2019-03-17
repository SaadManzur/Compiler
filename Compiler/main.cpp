#include "Common.h"
#include "Scanner.h"
#include "Parser.h"
#include "RegisterAllocator.h"
#include<iostream>
#include<cstdio>
#include<cstdlib>
#include "EliminateRedundency.h"
#include<cassert>
using namespace std;

int main()
{
	Logger logger;
	std::FILE *stream;
	try
	{
		Scanner *scanner = new Scanner("test004.txt");

		Parser *parser= new Parser(scanner);
		parser->Parse();
/**/	EliminateRedundency step2(parser);
		step2.copyPropagation();
		step2.updateVersion();
		step2.CSE();
		step2.printCodesByBlocks();
		
	//	parser->printAllIntermediateCode();
	//	cout << "..........complete........." << endl << endl;
	//	parser->printCodesByBlocks();
		

		RegisterAllocator registerAllocator(parser);
		registerAllocator.start(parser.getCFGTreeRoot());
		
		cout << "..........after allocation........." << endl << endl;
		parser.setRegisters(registerAllocator.getAllAssignedRegisters());
		parser.printAllIntermediateCode();

		freopen_s(&stream, "cfg.vcg", "w", stdout);
		parser->outputVCGFile();

/* */ 	freopen_s(&stream, "cfg after step2.vcg", "w", stdout);
		step2.outputVCGFile();
	
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