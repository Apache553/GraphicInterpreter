
#include "Syntax.h"
#include "Interpreter.h"

#include <cassert>
#include <cwctype>

template<size_t N>
inline void GenericProbeFunction(const gi::ProbeRule(&rules)[N], int& ruleIdOut, const gi::Token& token)
{
	for (size_t i = 0; i < N; ++i)
	{
		if (token.type == rules[i].type) {
			ruleIdOut = rules[i].target;
			return;
		}
	}
	if (ruleIdOut < 0)
		FailWithProbeFailure(token);
}

template<typename T, size_t N, size_t M>
bool GenericAcceptFunction(const gi::Token& token, std::stack<gi::Nonterminal*>& parseStack,
	std::vector<gi::Symbol>& symbols, int& ruleId, const gi::ProbeRule(&probeRules)[N], int& progress,
	const gi::TransformFunction<T>(&Rules)[M][gi::MAX_RULE_LENGTH], T* that)
{
	if (ruleId < 0)
		GenericProbeFunction(probeRules, ruleId, token);

	bool ret = true;
	assert(Rules[ruleId][progress] != nullptr);
	ret = Rules[ruleId][progress](parseStack, token, that);

	++progress;

	return ret;
}

template<typename T, size_t N, size_t M>
bool GenericAcceptFunction(const gi::Token& token, std::stack<gi::Nonterminal*>& parseStack,
	std::vector<gi::Symbol>& symbols, int& ruleId, const gi::ProbeRule(&probeRules)[N], int& progress,
	const gi::TransformFunctionEditSymbol<T>(&Rules)[M][gi::MAX_RULE_LENGTH], T* that)
{
	if (ruleId < 0)
		GenericProbeFunction(probeRules, ruleId, token);

	bool ret = true;
	assert(Rules[ruleId][progress] != nullptr);
	ret = Rules[ruleId][progress](parseStack, token, that, symbols);

	++progress;

	return ret;
}


bool gi::Symbol::MatchName(const std::wstring& name) const
{
	if (name.size() == this->name.size())
	{
		for (size_t i = 0; i < name.size(); ++i)
		{
			if (std::towupper(name[i]) != std::towupper(this->name[i]))
				return false;
		}
		return true;
	}
	return false;
}


bool gi::NTAtom::Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols)
{
	if (ruleId < 0)
		Probe(token, symbols);

	bool ret = true;
	assert(Rules[ruleId][progress] != nullptr);
	ret = Rules[ruleId][progress](parseStack, token, this);

	++progress;

	return ret;
}

void gi::NTAtom::Print(int indent)
{
	PrintMessage(JoinAsWideString(IndentString(indent), L"[NTAtom]"));
	switch (ruleId)
	{
	case 0:
		PrintMessage(JoinAsWideString(IndentString(indent), L"LITERAL: ", literal));
		break;
	case 1:
		PrintMessage(JoinAsWideString(IndentString(indent), L"IDENTIFIER: ", identifier));
		break;
	case 2:
		PrintMessage(JoinAsWideString(IndentString(indent), L"IDENTIFIER: ", identifier));
	case 3:
		PrintMessage(JoinAsWideString(IndentString(indent), L"("));
		expression->Print(indent + 2);
		PrintMessage(JoinAsWideString(IndentString(indent), L")"));
		break;
	default:
		PrintMessage(JoinAsWideString(IndentString(indent), L"Error Rule!!!"));
	}
}

double gi::NTAtom::Evaluate(EvaluateContext& context)
{
	double r;
	switch (ruleId)
	{
	case 0:
		context.operands.push(literal);
		break;
	case 1:
		context.operands.push(context.Lookup(identifier));
		break;
	case 2:
		expression->Evaluate(context);
		r = context.GetLastResult();
		context.operands.pop();
		context.operands.push(context.LookupFunction(identifier)(r));
		break;
	case 3:
		expression->Evaluate(context);
		break;
	default:
		throw std::runtime_error("Invalid ruleId!");
	}
	return 0;
}

void gi::NTAtom::Probe(const Token& token, std::vector<Symbol>& symbols)
{
	switch (token.type)
	{
	case TokenType::Literal:
		ruleId = 0;
		break;
	case TokenType::Identifier:
		for (auto& symbol : symbols)
		{
			if (symbol.MatchName(token.string))
			{
				if (symbol.type == Symbol::Type::Function) {
					ruleId = 2;
				}
				else
				{
					ruleId = 1;
				}
				break;
			}
		}
		if (ruleId < 0)
			FailWithNonExistSymbol(token, token.string);
		break;
	case TokenType::SplitterLeftBracket:
		ruleId = 3;
		break;
	}
	if (ruleId < 0)
		FailWithProbeFailure(token);
}

bool gi::NTComponent2::Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols)
{
	return GenericAcceptFunction(token, parseStack, symbols, ruleId, ProbeRules, progress, Rules, this);
}

void gi::NTComponent2::Print(int indent)
{
	PrintMessage(JoinAsWideString(IndentString(indent), L"[NTComponent2]"));
	switch (ruleId)
	{
	case 0:
		PrintMessage(JoinAsWideString(IndentString(indent), L"**"));
		component->Print(indent + 2);
		break;
	case 1:
		PrintMessage(JoinAsWideString(IndentString(indent), L"<NULL>"));
		break;
	default:
		PrintMessage(JoinAsWideString(IndentString(indent), L"Error Rule!!!"));
	}
}

double gi::NTComponent2::Evaluate(EvaluateContext& context)
{
	double rh;
	switch (ruleId)
	{
	case 0:
		component->Evaluate(context);
		rh = context.GetLastResult();
		context.operands.pop();
		context.operands.top() = std::pow(context.operands.top(), rh);
		break;
	case 1:
		break;
	default:
		throw std::runtime_error("Invalid ruleId!");
	}
	return 0;
}

bool gi::NTComponent::Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols)
{
	return GenericAcceptFunction(token, parseStack, symbols, ruleId, ProbeRules, progress, Rules, this);
}

void gi::NTComponent::Print(int indent)
{
	PrintMessage(JoinAsWideString(IndentString(indent), L"[NTComponent]"));
	switch (ruleId)
	{
	case 0:
		atom->Print(indent + 2);
		component2->Print(indent + 2);
		break;
	default:
		PrintMessage(JoinAsWideString(IndentString(indent), L"Error Rule!!!"));
	}
}

double gi::NTComponent::Evaluate(EvaluateContext& context)
{
	switch (ruleId)
	{
	case 0:
		atom->Evaluate(context);
		component2->Evaluate(context);
		break;
	default:
		throw std::runtime_error("Invalid ruleId!");
	}
	return 0;
}

bool gi::NTFactor::Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols)
{
	return GenericAcceptFunction(token, parseStack, symbols, ruleId, ProbeRules, progress, Rules, this);
}

void gi::NTFactor::Print(int indent)
{
	PrintMessage(JoinAsWideString(IndentString(indent), L"[NTFactor]"));
	switch (ruleId)
	{
	case 0:
		PrintMessage(JoinAsWideString(IndentString(indent), L"+"));
		factor->Print(indent + 2);
		break;
	case 1:
		PrintMessage(JoinAsWideString(IndentString(indent), L"-"));
		factor->Print(indent + 2);
		break;
	case 2:
		component->Print(indent + 2);
		break;
	default:
		PrintMessage(JoinAsWideString(IndentString(indent), L"Error Rule!!!"));
	}
}

double gi::NTFactor::Evaluate(EvaluateContext& context)
{
	switch (ruleId)
	{
	case 0:
		factor->Evaluate(context);
		break;
	case 1:
		factor->Evaluate(context);
		context.operands.top() = -context.operands.top();
		break;
	case 2:
		component->Evaluate(context);
		break;
	default:
		throw std::runtime_error("Invalid ruleId!");
	}
	return 0;
}

bool gi::NTTerm2::Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols)
{
	return GenericAcceptFunction(token, parseStack, symbols, ruleId, ProbeRules, progress, Rules, this);
}

void gi::NTTerm2::Print(int indent)
{
	PrintMessage(JoinAsWideString(IndentString(indent), L"[NTTerm2]"));
	switch (ruleId)
	{
	case 0:
		PrintMessage(JoinAsWideString(IndentString(indent), L"*"));
		factor->Print(indent + 2);
		term2->Print(indent + 2);
		break;
	case 1:
		PrintMessage(JoinAsWideString(IndentString(indent), L"/"));
		factor->Print(indent + 2);
		term2->Print(indent + 2);
		break;
	case 2:
		PrintMessage(JoinAsWideString(IndentString(indent), L"<NULL>"));
		break;
	default:
		PrintMessage(JoinAsWideString(IndentString(indent), L"Error Rule!!!"));
	}
}

double gi::NTTerm2::Evaluate(EvaluateContext& context)
{
	double rh;
	switch (ruleId)
	{
	case 0:
		factor->Evaluate(context);
		rh = context.GetLastResult();
		context.operands.pop();
		context.operands.top() *= rh;
		term2->Evaluate(context);
		break;
	case 1:
		factor->Evaluate(context);
		rh = context.GetLastResult();
		context.operands.pop();
		context.operands.top() /= rh;
		term2->Evaluate(context);
		break;
	case 2:
		break;
	default:
		throw std::runtime_error("Invalid ruleId!");
	}
	return 0;
}

bool gi::NTTerm::Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols)
{
	return GenericAcceptFunction(token, parseStack, symbols, ruleId, ProbeRules, progress, Rules, this);
}

void gi::NTTerm::Print(int indent)
{
	PrintMessage(JoinAsWideString(IndentString(indent), L"[NTTerm]"));
	switch (ruleId)
	{
	case 0:
		factor->Print(indent + 2);
		term2->Print(indent + 2);
		break;
	default:
		PrintMessage(JoinAsWideString(IndentString(indent), L"Error Rule!!!"));
	}
}

double gi::NTTerm::Evaluate(EvaluateContext& context)
{
	switch (ruleId)
	{
	case 0:
		factor->Evaluate(context);
		term2->Evaluate(context);
		break;
	default:
		throw std::runtime_error("Invalid ruleId!");
	}
	return 0;
}

bool gi::NTExpression2::Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols)
{
	return GenericAcceptFunction(token, parseStack, symbols, ruleId, ProbeRules, progress, Rules, this);
}

void gi::NTExpression2::Print(int indent)
{
	PrintMessage(JoinAsWideString(IndentString(indent), L"[NTExpression2]"));
	switch (ruleId)
	{
	case 0:
		PrintMessage(JoinAsWideString(IndentString(indent), L"+"));
		term->Print(indent + 2);
		expression2->Print(indent + 2);
		break;
	case 1:
		PrintMessage(JoinAsWideString(IndentString(indent), L"-"));
		term->Print(indent + 2);
		expression2->Print(indent + 2);
		break;
	case 2:
		PrintMessage(JoinAsWideString(IndentString(indent), L"<NULL>"));
		break;
	default:
		PrintMessage(JoinAsWideString(IndentString(indent), L"Error Rule!!!"));
	}
}

double gi::NTExpression2::Evaluate(EvaluateContext& context)
{
	double rh;
	switch (ruleId)
	{
	case 0:
		term->Evaluate(context);
		rh = context.GetLastResult();
		context.operands.pop();
		context.operands.top() += rh;
		expression2->Evaluate(context);
		break;
	case 1:
		term->Evaluate(context);
		rh = context.GetLastResult();
		context.operands.pop();
		context.operands.top() -= rh;
		expression2->Evaluate(context);
		break;
	case 2:
		break;
	default:
		throw std::runtime_error("Invalid ruleId!");
	}
	return 0;
}

bool gi::NTExpression::Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols)
{
	return GenericAcceptFunction(token, parseStack, symbols, ruleId, ProbeRules, progress, Rules, this);
}

void gi::NTExpression::Print(int indent)
{
	PrintMessage(JoinAsWideString(IndentString(indent), L"[NTExpression]"));
	switch (ruleId)
	{
	case 0:
		term->Print(indent + 2);
		expression2->Print(indent + 2);
		break;
	default:
		PrintMessage(JoinAsWideString(IndentString(indent), L"Error Rule!!!"));
	}
}

double gi::NTExpression::Evaluate(EvaluateContext& context)
{
	switch (ruleId)
	{
	case 0:
		term->Evaluate(context);
		expression2->Evaluate(context);
		break;
	default:
		throw std::runtime_error("Invalid ruleId!");
	}
	return 0;
}

bool gi::NTOriginStatement::Accept(const Token& token, std::stack<Nonterminal*>& parseStack,
	std::vector<Symbol>& symbols)
{
	return GenericAcceptFunction(token, parseStack, symbols, ruleId, ProbeRules, progress, Rules, this);
}

void gi::NTOriginStatement::Print(int indent)
{
	PrintMessage(JoinAsWideString(IndentString(indent), L"[NTOriginStatement]"));
	switch (ruleId)
	{
	case 0:
		PrintMessage(JoinAsWideString(IndentString(indent), L"ORIGIN"));
		PrintMessage(JoinAsWideString(IndentString(indent), L"IS"));
		PrintMessage(JoinAsWideString(IndentString(indent), L"("));
		expression1->Print(indent + 2);
		PrintMessage(JoinAsWideString(IndentString(indent), L","));
		expression2->Print(indent + 2);
		PrintMessage(JoinAsWideString(IndentString(indent), L")"));
		break;
	default:
		PrintMessage(JoinAsWideString(IndentString(indent), L"Error Rule!!!"));
	}
}

double gi::NTOriginStatement::Evaluate(EvaluateContext& context)
{
	double x, y;
	switch (ruleId)
	{
	case 0:
		context.NewExpression();
		expression1->Evaluate(context);
		x = context.GetLastResult();
		context.NewExpression();
		expression2->Evaluate(context);
		y = context.GetLastResult();
		context.GetCanvas()->SetDrawOrigin(x, y);
		break;
	default:
		throw std::runtime_error("Invalid ruleId!");
	}
	return 0;
}

bool gi::NTScaleStatement::Accept(const Token& token, std::stack<Nonterminal*>& parseStack,
	std::vector<Symbol>& symbols)
{
	return GenericAcceptFunction(token, parseStack, symbols, ruleId, ProbeRules, progress, Rules, this);
}

void gi::NTScaleStatement::Print(int indent)
{
	PrintMessage(JoinAsWideString(IndentString(indent), L"[NTScaleStatement]"));
	switch (ruleId)
	{
	case 0:
		PrintMessage(JoinAsWideString(IndentString(indent), L"SCALE"));
		PrintMessage(JoinAsWideString(IndentString(indent), L"IS"));
		PrintMessage(JoinAsWideString(IndentString(indent), L"("));
		expression1->Print(indent + 2);
		PrintMessage(JoinAsWideString(IndentString(indent), L","));
		expression2->Print(indent + 2);
		PrintMessage(JoinAsWideString(IndentString(indent), L")"));
		break;
	default:
		PrintMessage(JoinAsWideString(IndentString(indent), L"Error Rule!!!"));
	}
}

double gi::NTScaleStatement::Evaluate(EvaluateContext& context)
{
	double x, y;
	switch (ruleId)
	{
	case 0:
		context.NewExpression();
		expression1->Evaluate(context);
		x = context.GetLastResult();
		context.NewExpression();
		expression2->Evaluate(context);
		y = context.GetLastResult();
		context.GetCanvas()->SetDrawScale(x, y);
		break;
	default:
		throw std::runtime_error("Invalid ruleId!");
	}
	return 0;
}

bool gi::NTRotStatement::Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols)
{
	return GenericAcceptFunction(token, parseStack, symbols, ruleId, ProbeRules, progress, Rules, this);
}

void gi::NTRotStatement::Print(int indent)
{
	PrintMessage(JoinAsWideString(IndentString(indent), L"[NTRotStatement]"));
	switch (ruleId)
	{
	case 0:
		PrintMessage(JoinAsWideString(IndentString(indent), L"ROT"));
		PrintMessage(JoinAsWideString(IndentString(indent), L"IS"));
		expression->Print(indent + 2);
		break;
	default:
		PrintMessage(JoinAsWideString(IndentString(indent), L"Error Rule!!!"));
	}
}

double gi::NTRotStatement::Evaluate(EvaluateContext& context)
{
	double r;
	switch (ruleId)
	{
	case 0:
		context.NewExpression();
		expression->Evaluate(context);
		r = context.GetLastResult();
		context.GetCanvas()->SetDrawRotation(r);
		break;
	default:
		throw std::runtime_error("Invalid ruleId!");
	}
	return 0;
}

bool gi::NTForStatement::Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols)
{
	return GenericAcceptFunction(token, parseStack, symbols, ruleId, ProbeRules, progress, Rules, this);
}

void gi::NTForStatement::Print(int indent)
{
	PrintMessage(JoinAsWideString(IndentString(indent), L"[NTForStatement]"));
	switch (ruleId)
	{
	case 0:
		PrintMessage(JoinAsWideString(IndentString(indent), L"FOR"));
		PrintMessage(JoinAsWideString(IndentString(indent), L"IDENTIFIER: ", iter));
		PrintMessage(JoinAsWideString(IndentString(indent), L"FROM"));
		from->Print(indent + 2);
		PrintMessage(JoinAsWideString(IndentString(indent), L"TO"));
		to->Print(indent + 2);
		PrintMessage(JoinAsWideString(IndentString(indent), L"STEP"));
		step->Print(indent + 2);
		PrintMessage(JoinAsWideString(IndentString(indent), L"DRAW"));
		PrintMessage(JoinAsWideString(IndentString(indent), L"("));
		x->Print(indent + 2);
		PrintMessage(JoinAsWideString(IndentString(indent), L","));
		y->Print(indent + 2);
		PrintMessage(JoinAsWideString(IndentString(indent), L")"));
		break;
	default:
		PrintMessage(JoinAsWideString(IndentString(indent), L"Error Rule!!!"));
	}
}

double gi::NTForStatement::Evaluate(EvaluateContext& context)
{
	double iterFrom, iterTo, iterStep, iterValue;
	switch (ruleId)
	{
	case 0:
		context.NewExpression();
		from->Evaluate(context);
		iterFrom = context.GetLastResult();
		context.NewExpression();
		to->Evaluate(context);
		iterTo = context.GetLastResult();
		context.NewExpression();
		step->Evaluate(context);
		iterStep = context.GetLastResult();
		if (iterFrom > iterTo)
		{
			PrintMessage(L"FROM > TO! Invert loop direction.");
			std::swap(iterFrom, iterTo);
			iterStep = -iterStep;
		}
		iterValue = iterFrom;
		for (size_t i = 0; iterValue <= iterTo; ++i)
		{
			iterValue = iterFrom + static_cast<double>(i) * iterStep;
			context.NewExpression();
			context.AddVariableSymbol(iter, iterValue);
			x->Evaluate(context);
			double cx = context.GetLastResult();
			context.NewExpression();
			context.AddVariableSymbol(iter, iterValue);
			y->Evaluate(context);
			double cy = context.GetLastResult();
			context.GetCanvas()->DrawPoint(cx, cy);
		}
		break;
	default:
		throw std::runtime_error("Invalid ruleId!");
	}
	return 0;
}

bool gi::NTSizeStatement::Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols)
{
	return GenericAcceptFunction(token, parseStack, symbols, ruleId, ProbeRules, progress, Rules, this);
}

void gi::NTSizeStatement::Print(int indent)
{
	PrintMessage(JoinAsWideString(IndentString(indent), L"[NTSizeStatement]"));
	switch (ruleId)
	{
	case 0:
		PrintMessage(JoinAsWideString(IndentString(indent), L"SIZE"));
		PrintMessage(JoinAsWideString(IndentString(indent), L"IS"));
		expression->Print(indent + 2);
		break;
	default:
		PrintMessage(JoinAsWideString(IndentString(indent), L"Error Rule!!!"));
	}
}

double gi::NTSizeStatement::Evaluate(EvaluateContext& context)
{
	double r;
	switch (ruleId)
	{
	case 0:
		context.NewExpression();
		expression->Evaluate(context);
		r = context.GetLastResult();
		context.GetCanvas()->SetDrawPointSize(static_cast<int>(r));
		break;
	default:
		throw std::runtime_error("Invalid ruleId!");
	}
	return 0;
}

bool gi::NTColorStatement::Accept(const Token& token, std::stack<Nonterminal*>& parseStack,
	std::vector<Symbol>& symbols)
{
	return GenericAcceptFunction(token, parseStack, symbols, ruleId, ProbeRules, progress, Rules, this);
}

void gi::NTColorStatement::Print(int indent)
{
	PrintMessage(JoinAsWideString(IndentString(indent), L"[NTColorStatement]"));
	switch (ruleId)
	{
	case 0:
		PrintMessage(JoinAsWideString(IndentString(indent), L"COLOR"));
		PrintMessage(JoinAsWideString(IndentString(indent), L"IS"));
		PrintMessage(JoinAsWideString(IndentString(indent), L"("));
		expression1->Print(indent + 2);
		PrintMessage(JoinAsWideString(IndentString(indent), L","));
		expression2->Print(indent + 2);
		PrintMessage(JoinAsWideString(IndentString(indent), L","));
		expression3->Print(indent + 2);
		PrintMessage(JoinAsWideString(IndentString(indent), L")"));
		break;
	default:
		PrintMessage(JoinAsWideString(IndentString(indent), L"Error Rule!!!"));
	}
}

inline uint8_t RoundColorValue(int v)
{
	if (v < 0)
		return 0;
	if (v > 255)
		return 255;
	return static_cast<uint8_t>(v);
}

double gi::NTColorStatement::Evaluate(EvaluateContext& context)
{
	int ir, ig, ib;
	switch (ruleId)
	{
	case 0:
		context.NewExpression();
		expression1->Evaluate(context);
		ir = static_cast<int>(context.GetLastResult());
		context.NewExpression();
		expression2->Evaluate(context);
		ig = static_cast<int>(context.GetLastResult());
		context.NewExpression();
		expression3->Evaluate(context);
		ib = static_cast<int>(context.GetLastResult());
		context.GetCanvas()->SetDrawPointColor(
			RoundColorValue(ir),
			RoundColorValue(ig),
			RoundColorValue(ib));
		break;
	default:
		throw std::runtime_error("Invalid ruleId!");
	}
	return 0;
}

bool gi::NTStatement::Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols)
{
	return GenericAcceptFunction(token, parseStack, symbols, ruleId, ProbeRules, progress, Rules, this);
}

void gi::NTStatement::Print(int indent)
{
	PrintMessage(JoinAsWideString(IndentString(indent), L"[NTStatement]"));
	switch (ruleId)
	{
	case 0:
		originStatement->Print(indent + 2);
		break;
	case 1:
		scaleStatement->Print(indent + 2);
		break;
	case 2:
		rotStatement->Print(indent + 2);
		break;
	case 3:
		forStatement->Print(indent + 2);
		break;
	case 4:
		sizeStatement->Print(indent + 2);
		break;
	case 5:
		colorStatement->Print(indent + 2);
		break;
	default:
		PrintMessage(JoinAsWideString(IndentString(indent), L"Error Rule!!!"));
	}
}

double gi::NTStatement::Evaluate(EvaluateContext& context)
{
	switch (ruleId)
	{
	case 0:
		originStatement->Evaluate(context);
		break;
	case 1:
		scaleStatement->Evaluate(context);
		break;
	case 2:
		rotStatement->Evaluate(context);
		break;
	case 3:
		forStatement->Evaluate(context);
		break;
	case 4:
		sizeStatement->Evaluate(context);
		break;
	case 5:
		colorStatement->Evaluate(context);
		break;
	default:
		throw std::runtime_error("Invalid ruleId!");
	}
	return 0;
}

bool gi::NTProgram::Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols)
{
	return GenericAcceptFunction(token, parseStack, symbols, ruleId, ProbeRules, progress, Rules, this);
}

void gi::NTProgram::Print(int indent)
{
	PrintMessage(JoinAsWideString(IndentString(indent), L"[NTProgram]"));
	switch (ruleId)
	{
	case 0:
		statement->Print(indent + 2);
		PrintMessage(JoinAsWideString(IndentString(indent), L";"));
		program->Print(indent + 2);
		break;
	case 1:
		PrintMessage(JoinAsWideString(IndentString(indent), L"<NULL>"));
		break;
	default:
		PrintMessage(JoinAsWideString(IndentString(indent), L"Error Rule!!!"));
	}
}

double gi::NTProgram::Evaluate(EvaluateContext& context)
{
	switch (ruleId)
	{
	case 0:
		statement->Evaluate(context);
		program->Evaluate(context);
		break;
	case 1:
		break;
	default:
		throw std::runtime_error("Invalid ruleId!");
	}
	return 0;
}
