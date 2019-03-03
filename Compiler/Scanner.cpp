#include "Scanner.h"

Scanner::Scanner(string fileName) : lineNumber(1), colNumber(1)
{
	fileReader = new FileReader(fileName);
	inputSymbol = fileReader->GetSymbol();
}

int Scanner::GetSymbol()
{
	ProcessToken();

	return symbol;
}

int Scanner::GetNumber()
{
	return value;
}

int Scanner::GetId()
{
	return id;
}

int Scanner::GetLineNumber()
{
	return lineNumber;
}

int Scanner::GetColNumber()
{
	return colNumber;
}

string Scanner::Id2String(int id)
{
	return identifierList[id];
}

int Scanner::string2Id(string identifier)
{
	auto it = identifierHashMap.find(identifier);
	if (it == identifierHashMap.end())
		return -1;
	else
		return it->second;
}

int Scanner::getVersion(int id)
{
	if (id < versionTable.size())
		return versionTable[id];
	else
		return -1;

}

void Scanner::updateVersion(int id, int version)
{
	if (id < versionTable.size())
		versionTable[id] = version;
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
		ProcessToken();
		return;

	case '\n':
		Next();
		buffer = "\\n";
		lineNumber++;
		colNumber = 1;
		ProcessToken();
		return;

	case '\t':
		Next();
		buffer = "\\t";
		ProcessToken();
		return;

	default:
		if (isalpha(inputSymbol))
		{
			readIdentifier();

			symbol = getKeywordsToken();

			if (symbol == errorToken)
			{
				symbol = identToken;
				id = string2Id(buffer);
				if (id==-1)
				{
					identifierList.push_back(buffer);
					versionTable.push_back(-1);      // considering initially all versions are -1 
					id = identifierList.size() - 1;
					
					identifierHashMap.insert(make_pair(buffer, id));
				}
				
			}
		}
		else if (isdigit(inputSymbol))
		{
			readNumber();

			symbol = numberToken;
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
			error = "Syntax error.";
			return;
		}

	}
}

void Scanner::Next()
{
	inputSymbol = fileReader->GetSymbol();
	colNumber++;
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

	value = atoi(buffer.c_str());
}