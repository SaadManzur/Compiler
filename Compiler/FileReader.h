#pragma once
#ifndef _FILE_READER_H_
#define _FILE_READER_H_

#include <iostream>
#include <fstream>
#include <string>
using namespace std;
#endif

class FileReader
{
private:

	char symbol;
	ifstream fileInputStream;

public:

	FileReader( string fileName );

	void Next();
	char GetSymbol();
	void Error(string errorMessage);

};
