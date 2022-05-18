#include "Utils.h"

#include <windows.h> /* for ClearConsole() */
#include <iostream> /* std::cin */

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
	}
}