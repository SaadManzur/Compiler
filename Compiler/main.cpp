#include "Common.h"
#include "Scanner.h"
#include "Parser.h"
#include<iostream>
#include<cstdio>
#include<cstdlib>
#include "EliminateRedundency.h"
using namespace std;

int main()
{
	Logger logger;
	std::FILE *stream;

	try
	{
		Scanner *scanner = new Scanner("cell.txt");

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
		

		freopen_s(&stream, "cfg.vcg", "w", stdout);
		parser->outputVCGFile();

/* */ 	freopen_s(&stream, "cfg after step2.vcg", "w", stdout);
		step2.outputVCGFile();
	
	}
	catch (SyntaxException exception)
	{
		logger.Error(exception.getMessage());
	}
	
	int x;
	cin >> x;

	return 0;
}