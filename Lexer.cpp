
#include "Lexer.h"

#include <cwctype>

int gi::Lexer::Init(const std::wstring& content) {
	input_code = content;
	token_cursor = input_code.begin();
	line = 0;
	col = 0;
	return 1;
}

gi::Token& gi::Lexer::GetCurrentToken() {
	return curr_token;
}

void gi::Lexer::MoveToNext() {
	curr_token = getToken();
}

gi::Token gi::Lexer::getToken() {
	gi::Token res_token;
	// if whitespace col and line must be changed
	res_token.col = col + 1;
	res_token.line = line + 1;

	// convert str to lowercase
	auto strToLower = [](std::wstring& str) {
		for (auto& it : str) {
			if (it >= L'A' && it <= L'Z')
				it += L'a' - L'A';
		}
	};
	// declare params about regex
	std::wsmatch search_res;
	std::wstring::const_iterator cit = token_cursor;
	size_t semicolon_pos = input_code.find(L";");
	// if cannot find ";"
	if (semicolon_pos != std::wstring::npos)
		semicolon_pos = input_code.length() - 1;
	std::wstring::iterator it_semicolon = input_code.begin() + semicolon_pos + 1;   // +1 to detect ";"
	if (it_semicolon > input_code.end()) {
		it_semicolon = input_code.end();
	}
	std::wstring::const_iterator cit_semicolon = it_semicolon;
	bool search_flag = false;

	/* EOF */
	if (token_cursor == input_code.end()) {
		res_token.type = gi::TokenType::None;
		res_token.string = L"EOF";
		return res_token;
	}

	/* Space */
	{
		while (1) {
			// if space
			wchar_t curr_char = *token_cursor;
			if (*token_cursor == L'\n') {
				col = 0, line++, res_token.line++, res_token.col = 0;
				// check EOF
				if (++token_cursor == input_code.end()) {
					res_token.type = gi::TokenType::None;
					res_token.string = L"EOF";
					return res_token;
				}
			}
			else if (std::iswspace(*token_cursor)) {
				col++, res_token.col++;
				// check EOF
				if (++token_cursor == input_code.end()) {
					res_token.type = gi::TokenType::None;
					res_token.string = L"EOF";
					return res_token;
				}
			}
			else
				break;
		}
	}

	/* Comment */
	{
		// try match Comment
		search_flag = std::regex_search(token_cursor, it_semicolon, regex_set.Comment, std::regex_constants::match_flag_type::match_continuous);
		// real search Comment
		if (search_flag) {
			std::regex_search(cit, cit_semicolon, search_res, regex_set.Comment);
			std::wstring search_str = search_res[0];
			//line++, col = 0, res_token.line++, res_token.col = 0;
			col += search_str.length();
			res_token.col += search_str.length();
			token_cursor += search_str.length();
			return getToken();
		}
	}

	/* Splitter */
	{
		// try to match Splitter
		search_flag = std::regex_search(token_cursor, it_semicolon, regex_set.Splitter, std::regex_constants::match_flag_type::match_continuous);
		// real search Splitter
		if (search_flag) {
			std::regex_search(cit, cit_semicolon, search_res, regex_set.Splitter);
			std::wstring search_str = search_res[0];
			res_token.string = search_str;

			// Splitter map
			struct SplitterTokenMap {
				std::wstring str;
				gi::TokenType type;
			};
			std::vector<SplitterTokenMap> key_map = {
				{L";",gi::TokenType::SplitterSemicolon},
				{L",",gi::TokenType::SplitterComma},
				{L"(",gi::TokenType::SplitterLeftBracket},
				{L")",gi::TokenType::SplitterRightBracket}
			};
			// identify Splitter
			for (auto& it : key_map) {
				if (it.str == search_str) {
					res_token.type = it.type;
					goto return_token;
				}
			}
			res_token.type = gi::TokenType::Error;
			goto return_token;
		}
	}

	/* Literal */
	{
		// try to match literal
		search_flag = std::regex_search(token_cursor, it_semicolon, regex_set.Literal, std::regex_constants::match_flag_type::match_continuous);
		// real search literal
		if (search_flag && curr_token.type != gi::TokenType::Literal) {
			std::regex_search(cit, cit_semicolon, search_res, regex_set.Literal);
			std::wstring search_str = search_res[0];
			res_token.string = search_str;
			res_token.type = gi::TokenType::Literal;
			goto return_token;
		}
	}

	/* Identifier */
	{
		// try to match Identifier
		search_flag = std::regex_search(token_cursor, it_semicolon, regex_set.Identifier, std::regex_constants::match_flag_type::match_continuous);
		// real search Identifier
		if (search_flag) {
			std::regex_search(cit, cit_semicolon, search_res, regex_set.Identifier);
			std::wstring search_str = search_res[0];
			res_token.string = search_str;
			// identifier may be param&Sin&PI
			// we must identify it below

			// convert to lowercase
			strToLower(search_str);

			// keyword map
			struct KeywordTokenMap {
				std::wstring str;
				gi::TokenType type;
			};
			std::vector<KeywordTokenMap> key_map = {
				{L"rot",TokenType::KeywordRotation},
				{L"draw",TokenType::KeywordDraw},
				{L"for",TokenType::KeywordFor},
				{L"to",TokenType::KeywordTo},
				{L"from",TokenType::KeywordFrom},
				{L"origin",TokenType::KeywordOrigin},
				{L"scale",TokenType::KeywordScale},
				{L"step",TokenType::KeywordStep},
				{L"is",TokenType::KeywordIs},
				{L"size",TokenType::KeywordSize},
				{L"color",TokenType::KeywordColor}
			};
			// identify keywork
			for (auto& it : key_map) {
				if (it.str == search_str) {
					res_token.type = it.type;
					goto return_token;
				}
			}
			// normal identifier like PI Cos
			res_token.type = gi::TokenType::Identifier;
			goto return_token;
		}
	}

	/* Power */
	{
		// try match Power
		search_flag = std::regex_search(token_cursor, it_semicolon, regex_set.Power, std::regex_constants::match_flag_type::match_continuous);
		// real search Power
		if (search_flag) {
			res_token.type = gi::TokenType::OperatorPower;
			res_token.string = L"**";
			goto return_token;
		}
	}

	/* Operator */
	{
		// try match Operator
		search_flag = std::regex_search(token_cursor, it_semicolon, regex_set.Operator, std::regex_constants::match_flag_type::match_continuous);
		// real search Operator
		if (search_flag) {
			std::regex_search(cit, cit_semicolon, search_res, regex_set.Operator);
			std::wstring search_str = search_res[0];
			res_token.string = search_str;

			// identifier may be +=*/
			// we must identify it below

			// Operator map
			struct OperatorTokenMap {
				std::wstring str;
				gi::TokenType type;
			};
			std::vector<OperatorTokenMap> key_map = {
				{L"+",gi::TokenType::OperatorPlus},
				{L"-",gi::TokenType::OperatorMinus},
				{L"*",gi::TokenType::OperatorMultiply},
				{L"/",gi::TokenType::OperatorDivide}
			};
			// identify Operator
			for (auto& it : key_map) {
				if (it.str == search_str) {
					res_token.type = it.type;
					goto return_token;
				}
			}

		}
	}

	// normal identifier like PI Cos
	res_token.type = gi::TokenType::Error;

return_token:
	col += res_token.string.length();
	token_cursor += res_token.string.length();
	return res_token;
}
