#include "Common.h"
#include "Scanner.h"
#include "Parser.h"
#include<iostream>

using namespace std;

int main()
{
	Logger logger;

	try
	{
		Scanner *scanner = new Scanner("source.txt");

		Parser parser(scanner);
		parser.Parse();
		parser.printAllIntermediateCode();
		cout << "..........complete........." << endl << endl;
		parser.printCodesByBlocks();
	}
	catch (SyntaxException exception)
	{
		logger.Error(exception.getMessage());
	}
	
	int x;
	cin >> x;

	return 0;
}