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
		void FilterFilesAndDirectories();
		void DefinePreprocessorMacro();

		void PrintDirectoryContents(const std::filesystem::directory_entry& entry);
		std::vector<size_t> GetNumbersFromCSVString(const std::string& userInput) const;

		std::string RootPath;
		std::vector<std::filesystem::directory_entry> PathEntries;
		std::vector<std::string> FilteredFilePaths;
	};
}