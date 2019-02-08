#include "Common.h"
#include "Scanner.h"
#include "Parser.h"

int main()
{
	Logger logger;

	try
	{
		Scanner *scanner = new Scanner("source.txt");

		Parser parser(scanner);
		parser.Parse();
	}
	catch (SyntaxException exception)
	{
		logger.Error(exception.getMessage());
	}

	return 0;
}