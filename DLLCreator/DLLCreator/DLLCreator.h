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

		std::string FindVcxprojFilePath() const;
		void PrintDirectoryContents(const std::filesystem::directory_entry& entry);
		std::vector<size_t> GetNumbersFromCSVString(const std::string& userInput) const;
		size_t GetNumberOfDirectoriesDeep(const std::string& filePath) const;

		std::basic_string<unsigned char /* BYTE */> RootPath;
		std::basic_string<unsigned char /* BYTE */> APIFileName;
		std::basic_string<unsigned char /* BYTE */> APIFileNamePath;
		std::basic_string<unsigned char /* BYTE */> APIMacro;
		std::vector<std::filesystem::directory_entry> PathEntries;
		std::vector<std::string> FilteredFilePaths;

		constexpr inline static const unsigned long /* DWORD */ ExportMacroLength{7};
		constexpr inline static const unsigned char /* BYTE */ PreproExportMacro[ExportMacroLength + 1]{"EXPORT;"};
	};
}