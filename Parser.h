#pragma once

#include "ILexer.h"
#include "Syntax.h"

#include <memory>
#include <vector>
#include <cmath>

namespace gi
{

	class Parser
	{
	private:
		std::unique_ptr<NTProgram> astRoot;
	public:
		Parser() = default;
		void Parse(ILexer& lexer);
		std::unique_ptr<NTProgram> GetASTRoot();

		std::vector<Symbol> symbols = {
			{L"PI", Symbol::Type::Constant, 3.1415926535},
			{L"E", Symbol::Type::Constant, 2.71828182845904523},
			{L"SIN", Symbol::Type::Function, 0.0, std::sin},
			{L"COS", Symbol::Type::Function, 0.0, std::cos},
			{L"TAN", Symbol::Type::Function, 0.0, std::tan},
			{L"SQRT", Symbol::Type::Function, 0.0, std::sqrt},
			{L"EXP", Symbol::Type::Function, 0.0, std::exp},
			{L"LN", Symbol::Type::Function, 0.0, std::log}
		};
	};

}
