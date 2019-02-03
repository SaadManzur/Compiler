#include "FileReader.h"

FileReader::FileReader(string fileName)
{
	try
	{
		fileInputStream.open(fileName);
		Next();
	}
	catch (exception &e)
	{
		cerr << "Exception: " << e.what() << endl;
	}
}

void FileReader::Next()
{
	fileInputStream.get(symbol);
}

char FileReader::GetSymbol()
{
	char symbolToReturn = symbol;
	Next();
	return symbolToReturn;
}

void FileReader::Error(string errorMessage)
{
	cerr << errorMessage << endl;
}

