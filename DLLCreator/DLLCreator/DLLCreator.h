#pragma once

#include <string>

namespace DLL
{
	class DLLCreator final
	{
	public:
		DLLCreator(const std::string& rootPath);

		void Convert();

	private:
		std::string RootPath;
	};
}