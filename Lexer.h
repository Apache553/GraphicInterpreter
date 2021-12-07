#pragma once

#include <regex>

#include "ILexer.h"

namespace gi
{
	class Lexer : public ILexer {
	public:
		// return current token. if error, return TokenType::Error, if reached the end of input stream, return TokenType::None
		Token& GetCurrentToken() final;
		// move to next token
		void MoveToNext() final;
		// init lexer
		int Init(const std::wstring& content) final;
	private:
		// input code
		std::wstring input_code;
		// regex pattern set
		struct RegexStruct {
			std::wregex Literal;
			std::wregex Identifier;
			std::wregex Comment;
			std::wregex Operator;
			std::wregex Power;  // power must be detected before operator
			std::wregex Splitter;
			std::wregex Space;
		};
		RegexStruct regex_set =
		{
			std::wregex(LR"((\+|-)?([1-9][0-9]*|0)(\.([0-9]*)?)?)"),
			std::wregex(LR"(([a-zA-Z_][0-9a-zA-Z_]*))"),
			std::wregex(LR"(((--)|(//)).*)"),
			std::wregex(LR"([\+\-\*\/])"),
			std::wregex(LR"(\*\*)"),
			std::wregex(LR"([,;\(\)])"),
			std::wregex(LR"([ \n\t])")
		};
		// points to current token
		std::wstring::iterator token_cursor;
		// real get token
		Token getToken();
		// current token
		Token curr_token;
		// current line
		size_t line;
		// current token
		size_t col;
	};
}