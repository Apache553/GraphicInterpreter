
#include "Interpreter.h"

#include <cassert>

double& gi::EvaluateContext::Lookup(const std::wstring& name)
{
	for(auto& sym:staticSymbols)
	{
		if(sym.MatchName(name) && (sym.type==Symbol::Type::Constant || sym.type==Symbol::Type::Variable))
		{
			return sym.value;
		}
	}
	for (auto& sym : dynamicSymbols)
	{
		if (sym.MatchName(name) && (sym.type == Symbol::Type::Constant || sym.type == Symbol::Type::Variable))
		{
			return sym.value;
		}
	}
	PrintMessage(JoinAsWideString(L"unknown reference to variable or constant \'", name, L"\'"));
	throw std::runtime_error("bad reference");
}

double(* gi::EvaluateContext::LookupFunction(const std::wstring& name))(double)
{
	for (auto& sym : staticSymbols)
	{
		if (sym.MatchName(name) && sym.type == Symbol::Type::Function)
		{
			return sym.function;
		}
	}
	for (auto& sym : dynamicSymbols)
	{
		if (sym.MatchName(name) && sym.type == Symbol::Type::Function)
		{
			return sym.function;
		}
	}
	PrintMessage(JoinAsWideString(L"unknown reference to function \'", name, L"\'"));
	throw std::runtime_error("bad reference");
}

double gi::EvaluateContext::GetLastResult() const
{
	assert(!operands.empty());
	return operands.top();
}

void gi::EvaluateContext::NewExpression()
{
	while (!operands.empty())
		operands.pop();
	dynamicSymbols.clear();
}

void gi::EvaluateContext::AddVariableSymbol(const std::wstring& name, double value)
{
	for (auto& sym : staticSymbols)
	{
		if (sym.MatchName(name))
		{
			PrintMessage(JoinAsWideString(L"error: already used variable name \'", name, L"\'"));
			throw std::runtime_error("bad reference");
		}
	}
	for (auto& sym : dynamicSymbols)
	{
		if (sym.MatchName(name))
		{
			PrintMessage(JoinAsWideString(L"error: already used variable name \'", name, L"\'"));
			throw std::runtime_error("bad reference");
		}
	}
	dynamicSymbols.push_back({ name,Symbol::Type::Variable,value });
}

void gi::EvaluateContext::SetCanvas(ICanvas* canvas)
{
	this->canvas = canvas;
}

gi::ICanvas* gi::EvaluateContext::GetCanvas()const
{
	return canvas;
}

void gi::EvaluateContext::Run(NTProgram* program)
{
	program->Evaluate(*this);
}
