#include "Common.h"

SyntaxException::SyntaxException()
{
	this->message = "Syntax error";
}

SyntaxException::SyntaxException(string message)
{
	this->message = message;
}
