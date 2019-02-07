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
		

	throw new SyntaxException();
}

void Parser::ident()
{
	if (symbol == identToken)
	{
		Next();
		return;
	}

	throw new SyntaxException();
}

void Parser::number()
{
	if (symbol == numberToken)
	{
		Next();
		return;
	}
	
	throw new SyntaxException();
}

void Parser::designator()
{
	ident();

	while (symbol == openBracketToken)
	{
		Next();
		expression();

		if (symbol == closeBracketToken) Next();
		else throw new SyntaxException();
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
				else throw new SyntaxException();
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

	if (symbol != timesToken && symbol != divToken)
		throw new SyntaxException();

	while (symbol == timesToken || symbol == divToken)
	{
		Next();
		factor();
	}
}

void Parser::expression()
{
	term();

	if (symbol != plusToken && symbol != minusToken)
		throw new SyntaxException();

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
		else throw new SyntaxException();
	}
	else throw new SyntaxException();
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
			else throw new SyntaxException();
		}
		else throw new SyntaxException();
	}
	else throw new SyntaxException();
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
			else throw new SyntaxException();
		}
		else throw new SyntaxException();
	}
	else throw new SyntaxException();
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
			else throw new SyntaxException();
		}
		else throw new SyntaxException();
	}
	else throw new SyntaxException();
}

void Parser::returnStatement()
{
	if (symbol == returnToken)
	{
		Next();

		try 
		{
			expression();
		}
		catch (SyntaxException e)
		{

		}
	}
	else throw new SyntaxException();
}

void Parser::statement()
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

void Parser::statSequence()
{
	statement();
	
	while (symbol == semiToken)
	{
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
		
		if (symbol != openBracketToken) throw new SyntaxException();

		while (symbol == openBracketToken)
		{
			number();

			if (symbol == closeBracketToken) Next();
			else throw new SyntaxException();
		}
	}
	else throw new SyntaxException();
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
	else throw new SyntaxException();
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
			else throw new SyntaxException();
		}
	}
	else throw new SyntaxException();
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
		else throw new SyntaxException();
	}
	else throw new SyntaxException();
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
		else throw new SyntaxException();
	}
	else throw new SyntaxException();
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
				else throw new SyntaxException();
			}
			else throw new SyntaxException();
		}
		else throw new SyntaxException();
	}
	else throw new SyntaxException();
}

Parser::Parser(Scanner *scanner)
{
	this->scanner = scanner;
}

void Parser::Parse()
{
	Next();

	computation();
}
