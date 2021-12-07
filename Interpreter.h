#pragma once

#include <string>
#include <stack>
#include <vector>

#include "ICanvas.h"
#include "Syntax.h"

namespace gi
{
	class EvaluateContext
	{
	private:
		std::vector<Symbol> staticSymbols = {
			{L"PI", Symbol::Type::Constant, 3.1415926535},
			{L"E", Symbol::Type::Constant, 2.71828182845904523},
			{L"SIN", Symbol::Type::Function, 0.0, std::sin},
			{L"COS", Symbol::Type::Function, 0.0, std::cos},
			{L"TAN", Symbol::Type::Function, 0.0, std::tan},
			{L"SQRT", Symbol::Type::Function, 0.0, std::sqrt},
			{L"EXP", Symbol::Type::Function, 0.0, std::exp},
			{L"LN", Symbol::Type::Function, 0.0, std::log}
		};

		std::vector<Symbol> dynamicSymbols;

		ICanvas* canvas = nullptr;
	public:
		std::stack<double> operands;
		double& Lookup(const std::wstring& name);
		double(*LookupFunction(const std::wstring& name))(double);
		double GetLastResult()const;

		void NewExpression();
		void AddVariableSymbol(const std::wstring& name, double value = 0);

		void SetCanvas(ICanvas* canvas);
		ICanvas* GetCanvas()const;

		void Run(NTProgram* program);
	};

}
