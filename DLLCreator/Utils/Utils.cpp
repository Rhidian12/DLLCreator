#include "Utils.h"

#include <iostream> /* std::cin */
#include <assert.h> /* assert() */

namespace Utils
{
	namespace IO
	{
		/* Reference: https://stackoverflow.com/questions/5866529/how-do-we-clear-the-console-in-assembly/5866648#5866648 */
		/* MSDN: https://docs.microsoft.com/en-us/windows/console/scrolling-a-screen-buffer-s-contents */
		void ClearConsole()
		{
			COORD topLeft = { 0, 0 };
			HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
			CONSOLE_SCREEN_BUFFER_INFO screen;
			DWORD written;

			GetConsoleScreenBufferInfo(console, &screen);
			FillConsoleOutputCharacterA(console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
			FillConsoleOutputAttribute
			(
				console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
				screen.dwSize.X * screen.dwSize.Y, topLeft, &written
			);
			SetConsoleCursorPosition(console, topLeft);
		}

		std::string ReadUserInput()
		{
			std::string input;
			std::cin >> input;

			return input;
		}

		bool ReadUserInput(const std::string& wantedInput)
		{
			std::string input;
			std::cin >> input;

			return input == wantedInput;
		}

		int StringCompare(const char* const pString, const char* const pOtherString, const char delimiter)
		{
			assert(pString != nullptr);
			assert(pOtherString != nullptr);

			int counter{};
			while (true)
			{
				if (pString[counter] != '\0' && pString[counter] != delimiter)
				{
					if (pOtherString[counter] == '\0' || pOtherString[counter] == delimiter)
					{
						return 1;
					}

					if (pOtherString[counter] > pString[counter])
					{
						return -1;
					}

					if (pString[counter] > pOtherString[counter])
					{
						return 1;
					}
				}

				++counter;
			}

			return 0;
		}

		/* https://stackoverflow.com/questions/18680118/own-implementation-of-stdstringfind-brute-force-search */
		bool StringContains(const char* const pString, const char* const pOtherString, const char delimiter)
		{
			assert(pString != nullptr);
			assert(pOtherString != nullptr);

			int stringLen = StringLength(pString, delimiter);
			int otherStringLen = StringLength(pOtherString, delimiter);

			for (int i = 0; i <= stringLen - otherStringLen; ++i)
			{
				int j{};
				while (j < otherStringLen && pString[i + j] == pOtherString[j])
				{
					++j;
				}
				
				/* match found */
				if (j == otherStringLen)
				{ 
					return true;
				}
			}

			return false;
		}

		int StringLength(const char* const pString, const char delimiter)
		{
			int counter{};
			while (pString[counter] != delimiter)
			{
				++counter;
			}

			return counter;
		}
	}
}