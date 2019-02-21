#pragma once
#ifndef _SCANNER_H_
#define _SCANNER_H_

#include <iostream>
#include <string>
#include "Common.h"
#include "FileReader.h"
#include <unordered_map>

using namespace std;
#endif

class Scanner
{
private:
	Logger logger;

	map<string, int> keywords = {
		{"then", 41}, {"do", 42}, {"od", 81}, {"fi", 82}, {"else", 90}, {"let", 100}, {"call", 101}, {"if", 102}, 
		{"while", 103}, {"return", 104}, {"var", 110}, {"array", 111}, {"function", 112}, {"procedure", 113}, {"main", 200},
	};

	int symbol;
	char inputSymbol;
	int value;
	int id;
	int lineNumber;
	int colNumber;
	string error;
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
	std::vector<string> identifierList;
	vector<int> versionTable;

	unordered_map<string, int> identifierHashMap;

	Scanner(string fileName);

	int GetSymbol();
	int GetNumber();
	int GetId();

	int GetLineNumber();
	int GetColNumber();

	string Id2String(int id);
	int string2Id(string identifier);
	void Error(string errorMessage);

	int getVersion(int id);
	void updateVersion(int id, int version);
};