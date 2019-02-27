#include "Common.h"
#include "Scanner.h"
#include "Parser.h"
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
		Scanner *scanner = new Scanner("source.txt");

		Parser parser(scanner);
		parser.Parse();
		parser.printAllIntermediateCode();
		cout << "..........complete........." << endl << endl;
		parser.printCodesByBlocks();
		freopen_s(&stream, "cfg.vcg", "w", stdout);
	//	freopen("cfg.vcg", "w", stdout);
		parser.outputVCGFile();
		parser.outputDominatorTree();
	}
	catch (SyntaxException exception)
	{
		logger.Error(exception.getMessage());
	}
	
	int x;
	cin >> x;

	return 0;
}