#include "Common.h"

SyntaxException::SyntaxException()
{
	this->message = "Syntax error";
}

SyntaxException::SyntaxException(string message)
{
	this->message = message;
}

SyntaxException::SyntaxException(int lineNumber, int colNumber, string message)
{
	this->message = message;
	this->lineNumber = lineNumber;
	this->colNumber = colNumber;
}

string SyntaxException::getMessage()
{
	string message = this->message + " at[" + to_string(lineNumber) + "," + to_string(colNumber) + "]";

	return message;
}
