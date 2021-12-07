#pragma once

#include <string>
#include <sstream>

namespace gi
{
	void PrintMessage(const std::wstring& msg);

	template<typename ... Args>
	std::wstring JoinAsWideString(Args&& ... args)
	{
		std::wostringstream oss;
		int dummy[] = { 0, (oss << std::forward<Args>(args), 0) ... };
		return oss.str();
	}

	inline std::wstring IndentString(int indent)
	{
		return std::wstring(indent, L' ');
	}
}
