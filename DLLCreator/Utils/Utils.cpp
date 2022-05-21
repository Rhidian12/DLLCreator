#include "Utils.h"

#include <iostream> /* std::cin */
#include <assert.h> /* assert() */

#undef max

namespace Utils
{
	namespace IO
	{
		unsigned char* operator""_byte(const char* pString, size_t size)
		{
			unsigned char* pNewString(new BYTE[size + 1]);
			strcpy_s(reinterpret_cast<char*>(pNewString), size + 1, pString);
			return pNewString;
		}

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

		int StringCompare(const BYTE* const pString, const BYTE* const pOtherString, const char delimiter)
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

			size_t stringLen = StringLength(pString, delimiter);
			size_t otherStringLen = StringLength(pOtherString, delimiter);

			for (size_t i = 0; i <= stringLen - otherStringLen; ++i)
			{
				size_t j{};
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

		bool StringContains(const BYTE* const pString, const BYTE* const pOtherString, const BYTE delimiter)
		{
			assert(pString != nullptr);
			assert(pOtherString != nullptr);

			size_t stringLen = StringLength(pString, delimiter);
			size_t otherStringLen = StringLength(pOtherString, delimiter);

			for (size_t i = 0; i <= stringLen - otherStringLen; ++i)
			{
				size_t j{};
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

		size_t StringLength(const char* const pString, const char delimiter)
		{
			size_t counter{};
			while (pString[counter] != delimiter)
			{
				++counter;
			}

			return counter;
		}

		size_t StringLength(const BYTE* const pString, const BYTE delimiter)
		{
			size_t counter{};
			while (pString[counter] != delimiter)
			{
				++counter;
			}

			return counter;
		}

		bool StringCopy(char* pDest, const char* pSource, const size_t count)
		{
			assert(pDest != nullptr);
			assert(pSource != nullptr);

			assert(StringLength(pSource, '\n') >= count);

			for (size_t i{}; i < count; ++i)
			{
				pDest[i] = pSource[i];
			}

			return true;
		}

		bool StringCopy(BYTE* pDest, const BYTE* pSource, const size_t count)
		{
			assert(pDest != nullptr);
			assert(pSource != nullptr);

			assert(StringLength(pSource, '\n') >= count);

			for (size_t i{}; i < count; ++i)
			{
				pDest[i] = pSource[i];
			}

			return true;
		}

		DWORD StringFind(const BYTE* pDest, const BYTE* pOtherString, const BYTE delimiter)
		{
			assert(pDest != nullptr && "Utils::IO::StringFind() > pDest is a nullptr");
			assert(pOtherString != nullptr && "Utils::IO::StringFind() > pOtherString is a nullptr");

			const size_t strLen(StringLength(pDest, delimiter));
			const size_t otherStrLen(StringLength(pOtherString, delimiter));

			assert(strLen >= otherStrLen && "Utils::IO::StringFind() > pDest must be larger or equal to pOtherString!");

			for (size_t i{}; i < strLen; ++i)
			{
				if (pDest[i] != delimiter)
				{
					if (pDest[i] == pOtherString[i])
					{
						if (StringCompare(pDest, pOtherString, delimiter) == 0)
						{
							return static_cast<DWORD>(i);
						}
					}
				}
			}

			return std::numeric_limits<DWORD>::max();
		}

		DWORD StringFind(const BYTE* pDest, const BYTE charToFind, const BYTE delimiter)
		{
			assert(pDest != nullptr && "Utils::IO::StringFind() > pDest is a nullptr");

			const size_t strLen(StringLength(pDest, delimiter));

			for (size_t i{}; i < strLen; ++i)
			{
				if (pDest[i] != delimiter)
				{
					if (pDest[i] == charToFind)
					{
						return static_cast<DWORD>(i);
					}
				}
			}

			return std::numeric_limits<DWORD>::max();
		}
	}
}