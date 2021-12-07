
#include "Parser.h"

void gi::Parser::Parse(ILexer& lexer)
{
	std::stack<Nonterminal*> parseStack;
	std::unique_ptr<NTProgram> root = std::make_unique<NTProgram>();

	parseStack.push(root.get());
	lexer.MoveToNext();
	bool printToken = true;

	while (!parseStack.empty())
	{
		Token& token = lexer.GetCurrentToken();
		if(printToken)
		{
			PrintMessage(JoinAsWideString(L"Token: ", token.line, L',', token.col, L": ", token.string));
			printToken = false;
		}
		if (token.type == TokenType::Error)
		{
			PrintMessage(JoinAsWideString(
				token.line, L',', token.col, L": ",
				L"error: characters in position cannot be identified as a token."
			));
			throw std::runtime_error("bad token");
		}
		if (parseStack.top()->Accept(token, parseStack, symbols))
		{
			
			lexer.MoveToNext();
			printToken = true;
		}
	}

	astRoot = std::move(root);
}

std::unique_ptr<gi::NTProgram> gi::Parser::GetASTRoot()
{
	return std::move(astRoot);
}
