#pragma once
#ifndef _SCANNER_H_
#define _SCANNER_H_

#include <iostream>
#include <string>
#include "Common.h"
#include "FileReader.h"
using namespace std;
#endif

class Scanner
{
private:
	Logger logger;
	std::vector<string> identifierList;
	map<string, int> keywords = {
		{"then", 41}, {"do", 42}, {"od", 81}, {"fi", 82}, {"else", 90}, {"let", 100}, {"call", 101}, {"if", 102}, {"while", 103},
		{"return", 104}, {"var", 110}, {"array", 111}, {"function", 112}, {"procedure", 113}, {"main", 200}
	};

	int symbol;
	char inputSymbol;
	int value;
	int id;
	string buffer;

	FileReader *fileReader;

	void Next();

	bool isSecondSymbolIsEqual();
	bool isEqualsToken();
	bool isNotEqualsToken();
	bool isGreaterThanEqualToken();
	bool isLessThanEqualToken();
	bool isBecomesToken();
	bool matchesCharacterSequence(string pattern);

	int getKeywordsToken();
	void readIdentifier();
	void readNumber();

	void ProcessToken();
	void Scan();

public:

	Scanner(string fileName);

	int GetSymbol();
	int GetNumber();
	int GetId();

	void Error(string errorMessage);

};