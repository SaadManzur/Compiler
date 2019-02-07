#include "Scanner.h"
#include "Parser.h"

int main()
{
	Scanner *scanner = new Scanner("source.txt");

	Parser parser(scanner);
	parser.Parse();

	return 0;
}