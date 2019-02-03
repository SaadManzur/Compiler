#include "Scanner.h"

Scanner::Scanner(string fileName)
{
	fileReader = new FileReader(fileName);
	inputSymbol = fileReader->GetSymbol();
	Scan();
}

void Scanner::ProcessToken()
{
	switch (inputSymbol)
	{
	case '*':
		symbol = timesToken;
		buffer = "*";
		Next();
		break;

	case '/':
		symbol = divToken;
		buffer = "/";
		Next();
		break;

	case '+':
		symbol = plusToken;
		buffer = "+";
		Next();
		break;

	case '-':
		symbol = minusToken;
		buffer = "-";
		Next();
		break;

	case '=':
		symbol = errorToken;
		buffer = "=";

		if (isEqualsToken())
		{
			symbol = eqlToken;
			buffer = "==";
			Next();
		}
			
		break;

	case '!':
		symbol = errorToken;
		buffer = "!";

		if (isNotEqualsToken())
		{
			symbol = neqToken;
			buffer = "!=";
			Next();
		}

		break;

	case '<':
		symbol = lssToken;
		buffer = "<";

		if (isLessThanEqualToken())
		{
			symbol = leqToken;
			buffer = "<=";
			Next();
		}

		else if (isBecomesToken())
		{
			symbol = becomesToken;
			buffer = "<-";
			Next();
		}

		break;

	case '>':
		symbol = gtrToken;
		buffer = ">";

		if (isGreaterThanEqualToken())
		{
			symbol = geqToken;
			buffer = ">=";
			Next();
		}

		break;

	case '.':
		symbol = periodToken;
		buffer = ".";
		Next();
		break;

	case ',':
		symbol = commaToken;
		buffer = ",";
		Next();
		break;

	case '[':
		symbol = openBracketToken;
		buffer = "[";
		Next();
		break;

	case ']':
		symbol = closeBracketToken;
		buffer = "]";
		Next();
		break;

	case ')':
		symbol = closeParenToken;
		buffer = ")";
		Next();
		break;

	case '(':
		symbol = openParenToken;
		buffer = "(";
		Next();
		break;

	case ';':
		symbol = semiToken;
		buffer = ";";
		Next();
		break;

	case '}':
		symbol = endToken;
		buffer = "}";
		Next();
		break;

	case '{':
		symbol = beginToken;
		buffer = "{";
		Next();
		break;

	case ' ':
		Next();
		return;

	case '\n':
		Next();
		buffer = "\\n";
		return;

	case '\t':
		Next();
		buffer = "\\t";
		return;

	default:
		if (isalpha(inputSymbol))
		{
			readIdentifier();

			symbol = getKeywordsToken();

			if (symbol == errorToken)
			{
				identifierList.push_back(buffer);
				symbol = ident;
			}
		}
		else if (isdigit(inputSymbol))
		{
			readNumber();

			symbol = number;
		}
		
		break;
	}

	logger.Inform(to_string(symbol) + " " + buffer);
}

void Scanner::Scan()
{
	while (symbol != periodToken)
	{
		ProcessToken();

		if (symbol == errorToken)
		{
			logger.Error("Syntax error.");
			return;
		}

	}
}

void Scanner::Next()
{
	inputSymbol = fileReader->GetSymbol();
}

bool Scanner::isSecondSymbolIsEqual()
{
	Next();

	if (inputSymbol == '=')
		return true;

	return false;
}

bool Scanner::isEqualsToken()
{
	return isSecondSymbolIsEqual();
}

bool Scanner::isNotEqualsToken()
{
	return isSecondSymbolIsEqual();
}

bool Scanner::isGreaterThanEqualToken()
{
	return isSecondSymbolIsEqual();
}

bool Scanner::isLessThanEqualToken()
{
	return isSecondSymbolIsEqual();
}

bool Scanner::isBecomesToken()
{
	if (inputSymbol == '-')
		return true;

	return false;
}

bool Scanner::matchesCharacterSequence(string pattern)
{
	for (int i = 0; i < pattern.length(); i++)
	{
		if (inputSymbol != pattern[i])
			return false;
		
		Next();
	}

	if (inputSymbol != ' ')
		return false;

	return true;
}

int Scanner::getKeywordsToken()
{
	map<string, int>::iterator it = keywords.find(buffer);

	if (it != keywords.end())
		return it->second;

	return errorToken;
}

void Scanner::readIdentifier()
{
	buffer = "";

	while (inputSymbol != ' ' && isalnum(inputSymbol))
	{
		buffer += inputSymbol;
		Next();
	}
}

void Scanner::readNumber()
{
	buffer = "";

	while (isdigit(inputSymbol))
	{
		buffer += inputSymbol;
		Next();
	}

	buffer += "\0";

	value = atoi(buffer.c_str());
}