#pragma once

#include <string> /* std::string */

namespace Utils
{
	namespace IO
	{
		void ClearConsole();

		bool ReadUserInput(const std::string & wantedInput);
		std::string ReadUserInput();
	}
}