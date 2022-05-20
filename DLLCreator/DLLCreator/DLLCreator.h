#pragma once

#define __STDC_WANT_LIB_EXT1__ 1 /* strncpy_s */
#include <string> /* std::string */
#include <vector> /* std::vector */
#include <filesystem> /* std::filesystem */

#define WRITE_TO_TEST_FILE

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
		void CreateAPIFile();
		void AddMacroToFilteredHeaderFiles();

		void PrintDirectoryContents(const std::filesystem::directory_entry& entry);
		std::vector<size_t> GetNumbersFromCSVString(const std::string& userInput) const;

		std::string RootPath;
		std::string APIFileName;
		std::string APIFileNamePath;
		std::string APIMacro;
		std::vector<std::filesystem::directory_entry> PathEntries;
		std::vector<std::string> FilteredFilePaths;
	};
}