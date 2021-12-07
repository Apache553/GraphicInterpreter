#pragma once
#include <cassert>
#include <memory>
#include <stack>
#include <stdexcept>
#include <vector>

#include "ILexer.h"
#include "Utils.h"

namespace gi
{
	static constexpr size_t MAX_RULE_LENGTH = 18;

	struct Symbol
	{
		std::wstring name;
		enum class Type
		{
			Variable,
			Function,
			Constant
		} type;
		double value;
		double (*function)(double);

		bool MatchName(const std::wstring& name)const;
	};



	inline void FailWithTokenMismatch(const Token& token, TokenType expected)
	{
		PrintMessage(JoinAsWideString(
			token.line, L',', token.col, L": ",
			L"error: expected token type \'", GetTokenTypeName(expected), L"\'",
			L" but token \'", token.string, L"\' of type \'", GetTokenTypeName(token.type), L"\' is present."
		));
		throw std::runtime_error("bad syntax");
	}

	inline void FailWithProbeFailure(const Token& token)
	{
		PrintMessage(JoinAsWideString(
			token.line, L',', token.col, L": ",
			L"error: syntax rule probe failed: unexpected token \'", token.string, L"\' of type \'", GetTokenTypeName(token.type), L"\' is present."
		));
		throw std::runtime_error("bad syntax");
	}

	inline void FailWithExistSymbol(const Token& token, const std::wstring& symbol)
	{
		PrintMessage(JoinAsWideString(
			token.line, L',', token.col, L": ",
			L"error: redefine symbol \'", symbol, L"\'"
		));
		throw std::runtime_error("bad syntax");
	}

	inline void FailWithNonExistSymbol(const Token& token, const std::wstring& symbol)
	{
		PrintMessage(JoinAsWideString(
			token.line, L',', token.col, L": ",
			L"error: unknown reference to symbol \'", symbol, L"\'"
		));
		throw std::runtime_error("bad syntax");
	}


	class NTExpression;
	class NTComponent;
	class EvaluateContext;

	class Nonterminal
	{
	public:
		// return true if consumes a token, else false
		virtual bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) = 0;

		virtual void Print(int indent) = 0;

		virtual double Evaluate(EvaluateContext& context) = 0;

		virtual ~Nonterminal() = default;
	};

	template<typename T>
	using TransformFunction = bool(*)(std::stack<Nonterminal*>&, const Token&, T*);

	template<typename T>
	using TransformFunctionEditSymbol = bool(*)(std::stack<Nonterminal*>&, const Token&, T*, std::vector<Symbol>&);

	template<typename T, TokenType Expected>
	bool MatchToken(std::stack<Nonterminal*>& parseStack, const Token& token, T* thiz)
	{
		if (token.type != Expected)
			FailWithTokenMismatch(token, Expected);
		return true;
	}

	template<typename T, std::wstring T::* Field>
	bool TransformTokenAsString(std::stack<Nonterminal*>& parseStack, const Token& token, T* thiz)
	{
		thiz->*Field = token.string;
		return true;
	}

	template<typename T, std::wstring T::* Field, TokenType Expected>
	bool MatchAndTransformTokenAsString(std::stack<Nonterminal*>& parseStack, const Token& token, T* thiz)
	{
		if (token.type == Expected)
			thiz->*Field = token.string;
		else
			FailWithTokenMismatch(token, Expected);
		return true;
	}

	template<typename T, std::wstring T::* Field, Symbol::Type SymbolType>
	bool AddSymbolEntry(std::stack<Nonterminal*>& parseStack, const Token& token, T* thiz, std::vector<Symbol>& symbols)
	{
		for (auto iter = symbols.begin(); iter != symbols.end(); ++iter)
		{
			if (iter->MatchName(thiz->*Field))
			{
				FailWithExistSymbol(token, thiz->*Field);
			}
		}
		symbols.push_back({ thiz->*Field, SymbolType });
		return false;
	}

	template<typename T, std::wstring T::* Field>
	bool RemoveSymbolEntry(std::stack<Nonterminal*>& parseStack, const Token& token, T* thiz, std::vector<Symbol>& symbols)
	{
		for (auto iter = symbols.begin(); iter != symbols.end(); ++iter)
		{
			if (iter->MatchName(thiz->*Field))
			{
				symbols.erase(iter);
				return false;
			}
		}
		std::abort(); // unreachable code
	}

	template<typename T, auto Func>
	bool SymbolOperationWrapper(std::stack<Nonterminal*>& parseStack, const Token& token, T* thiz, std::vector<Symbol>& symbols)
	{
		return Func(parseStack, token, thiz);
	}

	template<typename T, double T::* Field>
	bool TransformTokenAsDouble(std::stack<Nonterminal*>& parseStack, const Token& token, T* thiz)
	{
		thiz->*Field = std::stod(token.string);
		return true;
	}

	template<typename T, double T::* Field, TokenType Expected>
	bool MatchAndTransformTokenAsDouble(std::stack<Nonterminal*>& parseStack, const Token& token, T* thiz)
	{
		if (token.type == Expected)
			thiz->*Field = std::stod(token.string);
		else
			FailWithTokenMismatch(token, Expected);
		return true;
	}

	template<typename T, typename U, std::unique_ptr<U> T::* Field>
	bool TransformTokenAsNonterminal(std::stack<Nonterminal*>& parseStack, const Token& token, T* thiz)
	{
		(thiz->*Field).reset(new U);
		parseStack.push((thiz->*Field).get());
		return false;
	}

	template<typename T>
	bool EndNonterminal(std::stack<Nonterminal*>& parseStack, const Token& token, T* thiz)
	{
		parseStack.pop();
		return false;
	}

	struct ProbeRule
	{
		TokenType type;
		int target;
	};

	class NTAtom :public Nonterminal
	{
	private:
		// 0. LITERAL
		// 1. IDENTIFIER
		// 2. IDENTIFIER ( Expression )
		// 3. ( Expression )
		int ruleId = -1;
		int progress = 0;

		void Probe(const Token& token, std::vector<Symbol>& symbols);
	public:
		bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) override;
		void Print(int indent) override;
		double Evaluate(EvaluateContext& context) override;
	private:
		double literal;
		std::wstring identifier;
		std::unique_ptr<NTExpression> expression;

		static constexpr TransformFunction<NTAtom> Rules[][MAX_RULE_LENGTH] = {
			{
				MatchAndTransformTokenAsDouble<NTAtom, &NTAtom::literal, TokenType::Literal>,
				EndNonterminal<NTAtom>,
				nullptr
			},
			{
				MatchAndTransformTokenAsString<NTAtom, &NTAtom::identifier, TokenType::Identifier>,
				EndNonterminal<NTAtom>,
				nullptr
			},
			{
				MatchAndTransformTokenAsString<NTAtom, &NTAtom::identifier, TokenType::Identifier>,
				MatchToken<NTAtom, TokenType::SplitterLeftBracket>,
				TransformTokenAsNonterminal<NTAtom,NTExpression,&NTAtom::expression>,
				MatchToken<NTAtom, TokenType::SplitterRightBracket>,
				EndNonterminal<NTAtom>,
				nullptr
			},
			{
				MatchToken<NTAtom, TokenType::SplitterLeftBracket>,
				TransformTokenAsNonterminal<NTAtom, NTExpression, &NTAtom::expression>,
				MatchToken<NTAtom, TokenType::SplitterRightBracket>,
				EndNonterminal<NTAtom>,
				nullptr
			}
		};
	};

	class NTComponent2 : public Nonterminal
	{
	public:
		bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) override;
		void Print(int indent) override;
		double Evaluate(EvaluateContext& context) override;
	private:
		// 0. Component2 -> ** Component
		// 1. Component2 -> NULL
		int ruleId = -1;
		int progress = 0;

		std::unique_ptr<NTComponent> component;

		static constexpr TransformFunction<NTComponent2> Rules[][MAX_RULE_LENGTH] = {
			{
				MatchToken<NTComponent2, TokenType::OperatorPower>,
				TransformTokenAsNonterminal<NTComponent2, NTComponent, &NTComponent2::component>,
				EndNonterminal<NTComponent2>,
				nullptr
			},
			{
				EndNonterminal<NTComponent2>,
				nullptr
			}
		};

		static constexpr ProbeRule ProbeRules[] = {
			{TokenType::OperatorPower, 0},
			{TokenType::OperatorMultiply, 1},
			{TokenType::OperatorDivide, 1},

			{TokenType::KeywordTo, 1},
			{TokenType::KeywordStep, 1},
			{TokenType::KeywordDraw, 1},
			{TokenType::SplitterRightBracket, 1},
			{TokenType::SplitterComma, 1},
			{TokenType::SplitterSemicolon, 1}
		};
	};

	class NTComponent :public Nonterminal
	{
	public:
		bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) override;
		void Print(int indent) override;
		double Evaluate(EvaluateContext& context) override;
	private:
		// 0. Component -> Atom Component2
		int ruleId = -1;
		int progress = 0;

		std::unique_ptr<NTAtom> atom;
		std::unique_ptr<NTComponent2> component2;

		static constexpr TransformFunction<NTComponent> Rules[][MAX_RULE_LENGTH] = {
			{
				TransformTokenAsNonterminal<NTComponent, NTAtom, &NTComponent::atom>,
				TransformTokenAsNonterminal<NTComponent, NTComponent2, &NTComponent::component2>,
				EndNonterminal<NTComponent>,
				nullptr
			}
		};

		static constexpr ProbeRule ProbeRules[] = {
			{TokenType::Literal, 0},
			{TokenType::Identifier, 0},
			{TokenType::SplitterLeftBracket, 0}
		};
	};

	class NTFactor :public Nonterminal
	{
	public:
		bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) override;
		void Print(int indent) override;
		double Evaluate(EvaluateContext& context) override;
	private:
		// 0. Factor -> + Factor
		// 1. Factor -> - Factor
		// 2. Factor -> Component
		int ruleId = -1;
		int progress = 0;

		std::unique_ptr<NTFactor> factor;
		std::unique_ptr<NTComponent> component;

		static constexpr TransformFunction<NTFactor> Rules[][MAX_RULE_LENGTH] = {
			{
				MatchToken<NTFactor, TokenType::OperatorPlus>,
				TransformTokenAsNonterminal<NTFactor, NTFactor, &NTFactor::factor>,
				EndNonterminal<NTFactor>,
				nullptr
			},
			{
				MatchToken<NTFactor, TokenType::OperatorMinus>,
				TransformTokenAsNonterminal<NTFactor, NTFactor, &NTFactor::factor>,
				EndNonterminal<NTFactor>,
				nullptr
			},
			{
				TransformTokenAsNonterminal<NTFactor, NTComponent, &NTFactor::component>,
				EndNonterminal<NTFactor>,
				nullptr
			}
		};

		static constexpr ProbeRule ProbeRules[] = {
			{TokenType::Literal, 2},
			{TokenType::Identifier, 2},
			{TokenType::SplitterLeftBracket, 2},
			{TokenType::OperatorPlus, 0},
			{TokenType::OperatorMinus, 1}
		};
	};

	class NTTerm2 :public Nonterminal
	{
	public:
		bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) override;
		void Print(int indent) override;
		double Evaluate(EvaluateContext& context) override;
	private:
		// 0. Term2 -> * Factor Term2
		// 1. Term2 -> / Factor Term2
		// 2. Term2 -> NULL
		int ruleId = -1;
		int progress = 0;

		std::unique_ptr<NTFactor> factor;
		std::unique_ptr<NTTerm2> term2;

		static constexpr TransformFunction<NTTerm2> Rules[][MAX_RULE_LENGTH] = {
			{
				MatchToken<NTTerm2, TokenType::OperatorMultiply>,
				TransformTokenAsNonterminal<NTTerm2, NTFactor, &NTTerm2::factor>,
				TransformTokenAsNonterminal<NTTerm2, NTTerm2, &NTTerm2::term2>,
				EndNonterminal<NTTerm2>,
				nullptr
			},
			{
				MatchToken<NTTerm2, TokenType::OperatorDivide>,
				TransformTokenAsNonterminal<NTTerm2, NTFactor, &NTTerm2::factor>,
				TransformTokenAsNonterminal<NTTerm2, NTTerm2, &NTTerm2::term2>,
				EndNonterminal<NTTerm2>,
				nullptr
			},
			{
				EndNonterminal<NTTerm2>,
				nullptr
			}
		};

		static constexpr ProbeRule ProbeRules[] = {
			{TokenType::OperatorPlus, 2},
			{TokenType::OperatorMinus, 2},
			{TokenType::OperatorMultiply, 0},
			{TokenType::OperatorDivide, 1},

			{TokenType::KeywordTo, 2},
			{TokenType::KeywordStep, 2},
			{TokenType::KeywordDraw, 2},
			{TokenType::SplitterRightBracket, 2},
			{TokenType::SplitterComma, 2},
			{TokenType::SplitterSemicolon, 2}
		};
	};

	class NTTerm :public Nonterminal
	{
	public:
		bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) override;
		void Print(int indent) override;
		double Evaluate(EvaluateContext& context) override;
	private:
		// 0. Term -> Factor Term2
		int ruleId = -1;
		int progress = 0;

		std::unique_ptr<NTFactor> factor;
		std::unique_ptr<NTTerm2> term2;

		static constexpr TransformFunction<NTTerm> Rules[][MAX_RULE_LENGTH] = {
			{
				TransformTokenAsNonterminal<NTTerm, NTFactor, &NTTerm::factor>,
				TransformTokenAsNonterminal<NTTerm, NTTerm2, &NTTerm::term2>,
				EndNonterminal<NTTerm>,
				nullptr
			}
		};

		static constexpr ProbeRule ProbeRules[] = {
			{TokenType::Literal, 0},
			{TokenType::Identifier, 0},
			{TokenType::SplitterLeftBracket, 0},
			{TokenType::OperatorPlus, 0},
			{TokenType::OperatorMinus, 0}
		};
	};

	class NTExpression2 :public Nonterminal
	{
	public:
		bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) override;
		void Print(int indent) override;
		double Evaluate(EvaluateContext& context) override;
	private:
		// 0. Expression2 -> + Term Expression2
		// 1. Expression2 -> - Term Expression2
		// 2. Expression2 -> NULL
		int ruleId = -1;
		int progress = 0;

		std::unique_ptr<NTTerm> term;
		std::unique_ptr<NTExpression2> expression2;

		static constexpr TransformFunction<NTExpression2> Rules[][MAX_RULE_LENGTH] = {
			{
				MatchToken<NTExpression2, TokenType::OperatorPlus>,
				TransformTokenAsNonterminal<NTExpression2, NTTerm, &NTExpression2::term>,
				TransformTokenAsNonterminal<NTExpression2, NTExpression2, &NTExpression2::expression2>,
				EndNonterminal<NTExpression2>,
				nullptr
			},
			{
				MatchToken<NTExpression2, TokenType::OperatorMinus>,
				TransformTokenAsNonterminal<NTExpression2, NTTerm, &NTExpression2::term>,
				TransformTokenAsNonterminal<NTExpression2, NTExpression2, &NTExpression2::expression2>,
				EndNonterminal<NTExpression2>,
				nullptr
			},
			{
				EndNonterminal<NTExpression2>,
				nullptr
			}
		};

		static constexpr ProbeRule ProbeRules[] = {
			{TokenType::OperatorPlus, 0},
			{TokenType::OperatorMinus, 1},
			{TokenType::KeywordTo, 2},
			{TokenType::KeywordStep, 2},
			{TokenType::KeywordDraw, 2},
			{TokenType::SplitterRightBracket, 2},
			{TokenType::SplitterComma, 2},
			{TokenType::SplitterSemicolon, 2}
		};
	};

	class NTExpression :public Nonterminal
	{
	public:
		bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) override;
		void Print(int indent) override;
		double Evaluate(EvaluateContext& context) override;
	private:
		// 0. Expression -> Term Expression2
		int ruleId = -1;
		int progress = 0;

		std::unique_ptr<NTTerm> term;
		std::unique_ptr<NTExpression2> expression2;

		static constexpr TransformFunction<NTExpression> Rules[][MAX_RULE_LENGTH] = {
			{
				TransformTokenAsNonterminal<NTExpression, NTTerm, &NTExpression::term>,
				TransformTokenAsNonterminal<NTExpression, NTExpression2, &NTExpression::expression2>,
				EndNonterminal<NTExpression>,
				nullptr
			}
		};

		static constexpr ProbeRule ProbeRules[] = {
			{TokenType::Literal, 0},
			{TokenType::Identifier, 0},
			{TokenType::SplitterLeftBracket, 0},
			{TokenType::OperatorPlus, 0},
			{TokenType::OperatorMinus, 0}
		};
	};

	class NTOriginStatement :public Nonterminal
	{
	public:
		bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) override;
		void Print(int indent) override;
		double Evaluate(EvaluateContext& context) override;
	private:
		// 0. OriginStatement -> ORIGIN IS ( Expression , Expression )
		int ruleId = -1;
		int progress = 0;

		std::unique_ptr<NTExpression> expression1, expression2;

		static constexpr TransformFunction<NTOriginStatement> Rules[][MAX_RULE_LENGTH] = {
			{
				MatchToken<NTOriginStatement, TokenType::KeywordOrigin>,
				MatchToken<NTOriginStatement, TokenType::KeywordIs>,
				MatchToken<NTOriginStatement, TokenType::SplitterLeftBracket>,
				TransformTokenAsNonterminal<NTOriginStatement, NTExpression, &NTOriginStatement::expression1>,
				MatchToken<NTOriginStatement, TokenType::SplitterComma>,
				TransformTokenAsNonterminal<NTOriginStatement, NTExpression, &NTOriginStatement::expression2>,
				MatchToken<NTOriginStatement, TokenType::SplitterRightBracket>,
				EndNonterminal<NTOriginStatement>,
				nullptr
			}
		};

		static constexpr ProbeRule ProbeRules[] = {
			{TokenType::KeywordOrigin, 0}
		};
	};

	class NTScaleStatement :public Nonterminal
	{
	public:
		bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) override;
		void Print(int indent) override;
		double Evaluate(EvaluateContext& context) override;
	private:
		// 0. ScaleStatement -> SCALE IS ( Expression, Expression )
		int ruleId = -1;
		int progress = 0;

		std::unique_ptr<NTExpression> expression1, expression2;

		static constexpr TransformFunction<NTScaleStatement> Rules[][MAX_RULE_LENGTH] = {
			{
				MatchToken<NTScaleStatement, TokenType::KeywordScale>,
				MatchToken<NTScaleStatement, TokenType::KeywordIs>,
				MatchToken<NTScaleStatement, TokenType::SplitterLeftBracket>,
				TransformTokenAsNonterminal<NTScaleStatement, NTExpression, &NTScaleStatement::expression1>,
				MatchToken<NTScaleStatement, TokenType::SplitterComma>,
				TransformTokenAsNonterminal<NTScaleStatement, NTExpression, &NTScaleStatement::expression2>,
				MatchToken<NTScaleStatement, TokenType::SplitterRightBracket>,
				EndNonterminal<NTScaleStatement>,
				nullptr
			}
		};

		static constexpr ProbeRule ProbeRules[] = {
			{TokenType::KeywordScale, 0}
		};
	};

	class NTRotStatement :public Nonterminal
	{
	public:
		bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) override;
		void Print(int indent) override;
		double Evaluate(EvaluateContext& context) override;
	private:
		// 0. RotStatement -> ROT IS Expression
		int ruleId = -1;
		int progress = 0;

		std::unique_ptr<NTExpression> expression;

		static constexpr TransformFunction<NTRotStatement> Rules[][MAX_RULE_LENGTH] = {
			{
				MatchToken<NTRotStatement, TokenType::KeywordRotation>,
				MatchToken<NTRotStatement, TokenType::KeywordIs>,
				TransformTokenAsNonterminal<NTRotStatement, NTExpression, &NTRotStatement::expression>,
				EndNonterminal<NTRotStatement>,
				nullptr
			}
		};

		static constexpr ProbeRule ProbeRules[] = {
			{TokenType::KeywordRotation, 0}
		};
	};

	class NTForStatement :public Nonterminal
	{
	public:
		bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) override;
		void Print(int indent) override;
		double Evaluate(EvaluateContext& context) override;
	private:
		// 0. ForStatement -> FOR IDENTIFIER FROM Expression TO Expression STEP Expression DRAW ( Expression , Expression )
		int ruleId = -1;
		int progress = 0;

		std::wstring iter;
		std::unique_ptr<NTExpression> from, to, step, x, y;

		static constexpr TransformFunctionEditSymbol<NTForStatement> Rules[][MAX_RULE_LENGTH] = {
			{
				SymbolOperationWrapper<NTForStatement, MatchToken<NTForStatement, TokenType::KeywordFor>>,
				SymbolOperationWrapper<NTForStatement, MatchAndTransformTokenAsString<NTForStatement, &NTForStatement::iter, TokenType::Identifier>>,
				SymbolOperationWrapper<NTForStatement, MatchToken<NTForStatement, TokenType::KeywordFrom>>,
				SymbolOperationWrapper<NTForStatement, TransformTokenAsNonterminal<NTForStatement, NTExpression, &NTForStatement::from>>,
				SymbolOperationWrapper<NTForStatement, MatchToken<NTForStatement, TokenType::KeywordTo>>,
				SymbolOperationWrapper<NTForStatement, TransformTokenAsNonterminal<NTForStatement, NTExpression, &NTForStatement::to>>,
				SymbolOperationWrapper<NTForStatement, MatchToken<NTForStatement, TokenType::KeywordStep>>,
				SymbolOperationWrapper<NTForStatement, TransformTokenAsNonterminal<NTForStatement, NTExpression, &NTForStatement::step>>,
				SymbolOperationWrapper<NTForStatement, MatchToken<NTForStatement, TokenType::KeywordDraw>>,
				SymbolOperationWrapper<NTForStatement, MatchToken<NTForStatement, TokenType::SplitterLeftBracket>>,
				AddSymbolEntry<NTForStatement, &NTForStatement::iter,Symbol::Type::Variable>,
				SymbolOperationWrapper<NTForStatement, TransformTokenAsNonterminal<NTForStatement, NTExpression, &NTForStatement::x>>,
				SymbolOperationWrapper<NTForStatement, MatchToken<NTForStatement, TokenType::SplitterComma>>,
				SymbolOperationWrapper<NTForStatement, TransformTokenAsNonterminal<NTForStatement, NTExpression, &NTForStatement::y>>,
				RemoveSymbolEntry<NTForStatement, &NTForStatement::iter>,
				SymbolOperationWrapper<NTForStatement, MatchToken<NTForStatement, TokenType::SplitterRightBracket>>,
				SymbolOperationWrapper<NTForStatement, EndNonterminal<NTForStatement>>,
				nullptr
			}
		};

		static constexpr ProbeRule ProbeRules[] = {
			{TokenType::KeywordFor, 0}
		};
	};

	class NTSizeStatement :public Nonterminal
	{
	public:
		bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) override;
		void Print(int indent) override;
		double Evaluate(EvaluateContext& context) override;
	private:
		// 0. SizeStatement -> SIZE IS Expression
		int ruleId = -1;
		int progress = 0;

		std::unique_ptr<NTExpression> expression;

		static constexpr TransformFunction<NTSizeStatement> Rules[][MAX_RULE_LENGTH] = {
			{
				MatchToken<NTSizeStatement, TokenType::KeywordSize>,
				MatchToken<NTSizeStatement, TokenType::KeywordIs>,
				TransformTokenAsNonterminal<NTSizeStatement, NTExpression, &NTSizeStatement::expression>,
				EndNonterminal<NTSizeStatement>,
				nullptr
			}
		};

		static constexpr ProbeRule ProbeRules[] = {
			{TokenType::KeywordSize, 0}
		};
	};

	class NTColorStatement :public Nonterminal
	{
	public:
		bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) override;
		void Print(int indent) override;
		double Evaluate(EvaluateContext& context) override;
	private:
		// 0. ColorStatement -> COLOR IS ( Expression, Expression, Expression )
		int ruleId = -1;
		int progress = 0;

		std::unique_ptr<NTExpression> expression1, expression2, expression3;

		static constexpr TransformFunction<NTColorStatement> Rules[][MAX_RULE_LENGTH] = {
			{
				MatchToken<NTColorStatement, TokenType::KeywordColor>,
				MatchToken<NTColorStatement, TokenType::KeywordIs>,
				MatchToken<NTColorStatement, TokenType::SplitterLeftBracket>,
				TransformTokenAsNonterminal<NTColorStatement, NTExpression, &NTColorStatement::expression1>,
				MatchToken<NTColorStatement, TokenType::SplitterComma>,
				TransformTokenAsNonterminal<NTColorStatement, NTExpression, &NTColorStatement::expression2>,
				MatchToken<NTColorStatement, TokenType::SplitterComma>,
				TransformTokenAsNonterminal<NTColorStatement, NTExpression, &NTColorStatement::expression3>,
				MatchToken<NTColorStatement, TokenType::SplitterRightBracket>,
				EndNonterminal<NTColorStatement>,
				nullptr
			}
		};

		static constexpr ProbeRule ProbeRules[] = {
			{TokenType::KeywordColor, 0}
		};
	};

	class NTStatement :public Nonterminal
	{
	public:
		bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) override;
		void Print(int indent) override;
		double Evaluate(EvaluateContext& context) override;
	private:
		// 0. Statement -> OriginStatement
		// 1. Statement -> ScaleStatement
		// 2. Statement -> RotStatement
		// 3. Statement -> ForStatement
		// 4. Statement -> SizeStatement
		// 5. Statement -> ColorStatement
		int ruleId = -1;
		int progress = 0;

		std::unique_ptr<NTOriginStatement> originStatement;
		std::unique_ptr<NTScaleStatement> scaleStatement;
		std::unique_ptr<NTRotStatement> rotStatement;
		std::unique_ptr<NTForStatement> forStatement;
		std::unique_ptr<NTSizeStatement> sizeStatement;
		std::unique_ptr<NTColorStatement> colorStatement;

		static constexpr TransformFunction<NTStatement> Rules[][MAX_RULE_LENGTH] = {
			{
				TransformTokenAsNonterminal<NTStatement, NTOriginStatement, &NTStatement::originStatement>,
				EndNonterminal<NTStatement>,
				nullptr
			},
			{
				TransformTokenAsNonterminal<NTStatement, NTScaleStatement, &NTStatement::scaleStatement>,
				EndNonterminal<NTStatement>,
				nullptr
			},
			{
				TransformTokenAsNonterminal<NTStatement, NTRotStatement, &NTStatement::rotStatement>,
				EndNonterminal<NTStatement>,
				nullptr
			},
			{
				TransformTokenAsNonterminal<NTStatement, NTForStatement, &NTStatement::forStatement>,
				EndNonterminal<NTStatement>,
				nullptr
			},
			{
				TransformTokenAsNonterminal<NTStatement, NTSizeStatement, &NTStatement::sizeStatement>,
				EndNonterminal<NTStatement>,
				nullptr
			},
			{
				TransformTokenAsNonterminal<NTStatement, NTColorStatement, &NTStatement::colorStatement>,
				EndNonterminal<NTStatement>,
				nullptr
			}
		};

		static constexpr ProbeRule ProbeRules[] = {
			{TokenType::KeywordOrigin, 0},
			{TokenType::KeywordScale, 1},
			{TokenType::KeywordRotation, 2},
			{TokenType::KeywordFor, 3},
			{TokenType::KeywordSize,4},
			{TokenType::KeywordColor, 5}
		};
	};

	class NTProgram :public Nonterminal
	{
	public:
		bool Accept(const Token& token, std::stack<Nonterminal*>& parseStack, std::vector<Symbol>& symbols) override;
		void Print(int indent) override;
		double Evaluate(EvaluateContext& context) override;
	private:
		// 0. Program -> Statement ; Program
		// 1. Program -> NULL
		int ruleId = -1;
		int progress = 0;

		std::unique_ptr<NTStatement> statement;
		std::unique_ptr<NTProgram> program;

		static constexpr TransformFunction<NTProgram> Rules[][MAX_RULE_LENGTH] = {
			{
				TransformTokenAsNonterminal<NTProgram, NTStatement, &NTProgram::statement>,
				MatchToken<NTProgram, TokenType::SplitterSemicolon>,
				TransformTokenAsNonterminal<NTProgram, NTProgram, &NTProgram::program>,
				EndNonterminal<NTProgram>,
				nullptr
			},
			{
				EndNonterminal<NTProgram>,
				nullptr
			}
		};

		static constexpr ProbeRule ProbeRules[] = {
			{TokenType::KeywordOrigin, 0},
			{TokenType::KeywordScale, 0},
			{TokenType::KeywordRotation, 0},
			{TokenType::KeywordFor, 0},
			{TokenType::KeywordSize,0},
			{TokenType::KeywordColor, 0},
			{TokenType::None, 1}
		};
	};

}
