#pragma once

#include <string> /* std::string */
#include <vector> /* std::vector */
#include <filesystem> /* std::filesystem */

namespace DLL
{
	class DLLCreator final
	{
	public:
		DLLCreator(const std::string& rootPath);

		void Convert();

	private:
		void GetAllFilesAndDirectories();

		std::string RootPath;
		std::vector<std::filesystem::directory_entry> PathEntries;
	};
}