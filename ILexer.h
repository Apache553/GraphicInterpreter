#pragma once

#include <string>

namespace gi
{
	enum class TokenType
	{
		None = 0,
		Error,
		Literal,   // literal number : 1.0
		Identifier,  // the iteration variable T
		KeywordOrigin,
		KeywordScale,
		KeywordRotation,
		KeywordIs,
		KeywordFor,
		KeywordFrom,
		KeywordTo,
		KeywordStep,
		KeywordDraw,
		KeywordSize,
		KeywordColor,
		OperatorPlus,
		OperatorMinus,
		OperatorMultiply,
		OperatorDivide,
		OperatorPower,
		SplitterSemicolon,
		SplitterLeftBracket,
		SplitterRightBracket,
		SplitterComma
	};

	extern const wchar_t* TokenTypeName[];

	inline const wchar_t* GetTokenTypeName(TokenType type)
	{
		return TokenTypeName[static_cast<std::underlying_type_t<TokenType>>(type)];
	}

	struct Token
	{
		TokenType type;

		std::wstring string;

		size_t line;
		size_t col;
	};

	class ILexer
	{
	public:
		// all input uses wide string
		virtual int Init(const std::wstring& content) = 0;

		// return current token. if error, return TokenType::Error, if reached the end of input stream, return TokenType::None
		virtual Token& GetCurrentToken() = 0;

		// move to next token
		virtual void MoveToNext() = 0;
	};

}