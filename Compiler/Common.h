#pragma once
#ifndef _COMMON_H_
#define _COMMON_H_

#include <vector>
#include <map>
#include "Logger.h"
#endif

enum Tokens
{
	errorToken = 0,
	timesToken = 1,
	divToken = 2,

	plusToken = 11,
	minusToken = 12,

	eqlToken = 20,
	neqToken = 21,
	lssToken = 22,
	geqToken = 23,
	leqToken = 24,
	gtrToken = 25,

	periodToken = 30,
	commaToken = 31,
	openBracketToken = 32,
	closeBracketToken = 34,
	closeParenToken = 35,

	becomesToken = 40,
	thenToken = 41,
	doToken = 42,
	
	openParenToken = 50,
	numberToken = 60,
	identToken = 61,
	
	semiToken = 70,

	endToken = 80,
	odToken = 81,
	fiToken = 82,

	elseToken = 90,

	letToken = 100,
	callToken = 101,
	ifToken = 102,
	whileToken = 103,
	returnToken = 104,

	varToken = 110,
	arrToken = 111,
	funcToken = 112,
	procToken = 113,

	beginToken = 150,
	mainToken = 200,
	eofToken = 255
};

class SyntaxException : public std::exception
{
private:
	string message;

public:
	SyntaxException();
	SyntaxException(string message);
};