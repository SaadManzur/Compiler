#pragma once
#ifndef _PARSER_H_
#define _PARSER_H_

#include <iostream>
#include "Common.h"
#include "Scanner.h"
using namespace std;

#endif

class Parser
{
private:
	int symbol;
	Scanner *scanner;

	void Next();

	bool letter();
	bool digit();
	void relOp();

	void ident();
	void number();

	void designator();
	void factor();
	void term();
	void expression();
	void relation();

	void assignment();
	void funcCall();
	void ifStatement();
	void whileStatement();
	void returnStatement();

	void statement();
	void statSequence();

	void typeDecl();
	void varDecl();
	void funcDecl();
	void formalParam();
	void funcBody();

	void computation();

	void InputNum();
	void OutputNum();

	void OutputNewLine();

public:
	Parser(Scanner *scanner);

	void Parse();
};