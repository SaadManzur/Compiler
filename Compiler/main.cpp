#include "Common.h"
#include "Scanner.h"
#include "Parser.h"
#include "RegisterAllocator.h"
#include<iostream>
#include<cstdio>
#include<cstdlib>
using namespace std;

int main()
{
	Logger logger;
	std::FILE *stream;

	try
	{
		Scanner *scanner = new Scanner("source2.txt");

		Parser parser(scanner);
		parser.Parse();
		parser.printAllIntermediateCode();
		cout << "..........complete........." << endl << endl;
		parser.printCodesByBlocks();
		
		RegisterAllocator registerAllocator(parser);
		registerAllocator.start(parser.getCFGTreeRoot());

		freopen_s(&stream, "cfg.vcg", "w", stdout);
	//	freopen("cfg.vcg", "w", stdout);
		parser.outputVCGFile();
		parser.outputDominatorTree();
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