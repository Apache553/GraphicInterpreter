
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1

#include "Canvas.h"
#include "Lexer.h"
#include "Parser.h"
#include "Interpreter.h"

#include <fstream>
#include <codecvt>

#include <shellapi.h>

using namespace gi;

int main()
{
	LPCWSTR lpCmdline = GetCommandLineW();
	int nArgs;
	LPWSTR* pArgv = CommandLineToArgvW(lpCmdline, &nArgs);

	if (nArgs != 2)
	{
		PrintMessage(JoinAsWideString("Usage: ", pArgv[0], L" FILENAME"));
		return 1;
	}

	std::ifstream fs;
	std::string utf8content;
	{
		std::ostringstream oss;

		fs.open(pArgv[1], std::ios::binary);
		if (!fs.good())
		{
			PrintMessage(L"Failed to open file!");
			return 1;
		}

		oss << fs.rdbuf();
		utf8content = oss.str();
	}
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring content = converter.from_bytes(utf8content);

	std::unique_ptr<NTProgram> ast;
	Canvas canvas;
	canvas.InitializeWindow();
	canvas.SetDrawBackgroundColor(0x66, 0xCC, 0xFF);

	try {
		Lexer lexer;
		Parser parser;
		EvaluateContext interpreter;
		lexer.Init(content);
		parser.Parse(lexer);
		ast = parser.GetASTRoot();
		ast->Print(0);
		interpreter.SetCanvas(&canvas);
		interpreter.Run(ast.get());
	}
	catch (std::exception& e)
	{
		PrintMessage(L"encountered an error, stop processing.");
		PrintMessage(converter.from_bytes(e.what()));
		return 1;
	}

	canvas.ShowWindow();
	return 0;
}