#include "Parser.h"

void Parser::Next()
{
	symbol = scanner->GetSymbol();
}

void Parser::relOp()
{
	if (symbol >= 20 && symbol <= 25)
	{
		Next();
		return;
	}
		

	throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::ident()
{
	if (symbol == identToken)
	{
		Next();
		return;
	}

	throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::number()
{
	if (symbol == numberToken)
	{
		Next();
		return;
	}
	
	throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::designator()
{
	try
	{
		ident();

		while (symbol == openBracketToken)
		{
			Next();
			expression();

			if (symbol == closeBracketToken) Next();
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
	}
	catch (SyntaxException exception)
	{
		throw exception;
	}
}
	

void Parser::factor()
{
	try
	{
		designator();
	}
	catch (SyntaxException e)
	{
		try 
		{
			number();
		}
		catch (SyntaxException e)
		{
			if (symbol == openParenToken)
			{
				Next();
				expression();

				if (symbol == closeParenToken) Next();
				else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
			}
			else
			{
				funcCall();
			}
		}
	}
}

void Parser::term()
{
	factor();

	while (symbol == timesToken || symbol == divToken)
	{
		Next();
		factor();
	}
}

void Parser::expression()
{
	term();

	while (symbol == plusToken || symbol == minusToken)
	{
		Next();
		term();
	}
}

void Parser::relation()
{
	expression();
	relOp();
	expression();
}

void Parser::assignment()
{
	if (symbol == letToken)
	{
		Next();
		designator();

		if (symbol == becomesToken)
		{
			Next();
			expression();
		}
		else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::funcCall()
{
	if (symbol == callToken)
	{
		Next();
		ident();

		if (symbol == openParenToken)
		{
			Next();

			if(symbol != closeParenToken)
			{
				expression();

				while (symbol == commaToken)
				{
					Next();
					expression();
				}
			}
			
			if (symbol == closeParenToken) Next();
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
		else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::ifStatement()
{
	if (symbol == ifToken)
	{
		Next();
		relation();

		if (symbol == thenToken)
		{
			Next();
			statSequence();

			if (symbol == elseToken)
			{
				Next();
				statSequence();
			}

			if (symbol == fiToken) Next();
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
		else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::whileStatement()
{
	if (symbol == whileToken)
	{
		Next();
		relation();

		if (symbol == doToken)
		{
			Next();
			statSequence();
			
			if (symbol == odToken) Next();
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
		else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::returnStatement()
{
	if (symbol == returnToken)
	{
		Next();

		expression();
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::statement()
{
	try
	{
		ident();

		string identifierName = scanner->Id2String(scanner->GetId());

		if (identifierName == "OutputNum")
		{
			if (symbol == openParenToken)
			{
				Next();
				ident();

				if (symbol == closeParenToken)
				{
					OutputNum();
					Next();
				}
			}
		}
		else if (identifierName == "InputNum")
		{
			Next();

			if (symbol == openParenToken)
			{
				Next();

				if (symbol == closeParenToken)
				{
					InputNum();
					Next();
				}
			}
		}
		else if (identifierName == "OutputNewLine")
		{
			Next();

			if (symbol == openParenToken)
			{
				Next();

				if (symbol == closeParenToken)
				{
					OutputNewLine();
					Next();
				}
			}
		}
	}
	catch (SyntaxException exception)
	{
		try
		{
			assignment();
		}
		catch (SyntaxException e)
		{
			try
			{
				funcCall();
			}
			catch (SyntaxException e)
			{
				try
				{
					ifStatement();
				}
				catch (SyntaxException e)
				{
					try
					{
						whileStatement();
					}
					catch (SyntaxException e)
					{
						returnStatement();
					}
				}
			}
		}
	}
}

void Parser::statSequence()
{
	statement();
	
	while (symbol == semiToken)
	{
		Next();
		statement();
	}
}

void Parser::typeDecl()
{
	if (symbol == varToken)
	{
		Next();
	}
	else if (symbol == arrToken)
	{
		Next();
		
		if (symbol != openBracketToken) throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());

		while (symbol == openBracketToken)
		{
			number();

			if (symbol == closeBracketToken) Next();
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::varDecl()
{
	typeDecl();
	ident();

	while (symbol == commaToken)
	{
		Next();
		ident();
	}

	if (symbol == semiToken) Next();
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::funcDecl()
{
	if (symbol == funcToken || symbol == procToken)
	{
		ident();

		if (symbol != semiToken)
		{
			formalParam();
		}

		if (symbol == semiToken)
		{
			Next();
			funcBody();

			if (symbol == semiToken) Next();
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::formalParam()
{
	if (symbol == openParenToken)
	{
		Next();
		
		if (symbol != closeParenToken)
		{
			ident();

			while (symbol == commaToken)
			{
				Next();
				ident();
			}
		}

		if (symbol == closeParenToken) Next();
		else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::funcBody()
{
	while (symbol != beginToken)
	{
		varDecl();
	}

	if (symbol == beginToken)
	{
		statSequence();

		if (symbol == endToken) Next();
		else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::computation()
{
	if (symbol == mainToken)
	{
		Next();

		while (symbol == varToken || symbol == arrToken)
		{
			varDecl();
		}
		
		while (symbol == funcToken || symbol == procToken)
		{
			funcDecl();
		}

		if (symbol == beginToken)
		{
			Next();
			statSequence();

			if (symbol == endToken)
			{
				Next();
				if (symbol == periodToken) return;
				else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
			}
			else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
		}
		else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
	}
	else throw SyntaxException(scanner->GetLineNumber(), scanner->GetColNumber());
}

void Parser::InputNum()
{
	return;
}

void Parser::OutputNum()
{
	return;
}

void Parser::OutputNewLine()
{
	return;
}

Parser::Parser(Scanner *scanner)
{
	this->scanner = scanner;
}

void Parser::Parse()
{
	try
	{
		Next();

		computation();
	}
	catch (SyntaxException exception)
	{
		throw exception;
	}
}
