#pragma once

/* This has to be placed in the header sadly enough */
#define WIN32_LEAN_AND_MEAN /* Exclude rarely-used stuff from Windows headers */
#include <Windows.h> /* for ClearConsole() */

#include <string> /* std::string */

namespace Utils
{
	namespace IO
	{
		void ClearConsole();

		bool ReadUserInput(const std::string& wantedInput);
		std::string ReadUserInput();

		/* 0 means equal, 1 means pString > pOtherString, -1 means pOtherString > pString */
		int StringCompare(const char* const pString, const char* const pOtherString, const char delimiter);
		bool StringContains(const char* const pString, const char* const pOtherString, const char delimiter);
		int StringLength(const char* const pString, const char delimiter);
	}
}