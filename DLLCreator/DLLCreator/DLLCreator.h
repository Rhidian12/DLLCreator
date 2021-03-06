#pragma once

#define __STDC_WANT_LIB_EXT1__ 1 /* strncpy_s */
#include <string> /* std::string */
#include <vector> /* std::vector */
#include <filesystem> /* std::filesystem */

// #define WRITE_TO_TEST_FILE

namespace DLL
{
	class DLLCreator final
	{
	public:
		DLLCreator(const std::string& rootPath, const std::string& outputPath);

		void Convert();

	private:
		void GetAllFilesAndDirectories();
		void FilterFilesAndDirectories();
		void DefinePreprocessorMacro();
		void CreateAPIFile();
		void AddMacroToFilteredHeaderFiles();
		void GenerateCMakeFiles();
		void ExecuteCMake();

		void GenerateRootCMakeFile();
		void GenerateSubDirectoryCMakeFiles(const std::filesystem::directory_entry& entry);
		void GenerateLibraryDirectoryCMakeFiles(const std::filesystem::directory_entry& entry);
		void GenrerateSubDirectoryHCMakeFiles(const std::filesystem::directory_entry& _entry);
		void GenerateSubDirectoryCppCMakeFile(const std::filesystem::directory_entry& _entry);
		void GenerateSubDirectoryCppCMakeFile(const std::string& libName, const std::string& path);
		void GenerateSubDirectoryHAndLibCMakeFile(const std::filesystem::directory_entry& _entry);
		void GenerateSubDirectoryHAndLibAndDLLCMakeFile(const std::filesystem::directory_entry& _entry);

		/* returns
		0 if directory contains only .h files
		1 if directory contains .cpp files,
		2 if directory contains only .h and .lib,
		3 if directory contains .lib, .h and .dll */
		int8_t CheckSubDirectory(const std::filesystem::directory_entry& _entry);
		std::string FindVcxprojFilePath() const;
		void PrintDirectoryContents(const std::filesystem::directory_entry& entry);
		std::vector<size_t> GetNumbersFromCSVString(const std::string& userInput) const;
		size_t GetNumberOfDirectoriesDeep(const std::string& filePath) const;

		std::basic_string<unsigned char /* BYTE */> RootPath;
		std::basic_string<unsigned char /* BYTE */> ProjectName;
		std::basic_string<unsigned char /* BYTE */> APIFileName;
		std::basic_string<unsigned char /* BYTE */> APIFileNamePath;
		std::basic_string<unsigned char /* BYTE */> APIMacro;
		std::string OutputPath{};
		std::vector<std::filesystem::directory_entry> PathEntries;
		std::vector<std::string> FilteredFilePaths;

		constexpr inline static const unsigned long /* DWORD */ ExportMacroLength{7};
		constexpr inline static const unsigned char /* BYTE */ PreproExportMacro[ExportMacroLength + 1]{"EXPORT;"};

		std::vector<std::string> LibIncludeDirectories{};
		std::vector<std::string> LibSourceDirectories{};
		std::vector<std::string> DllDirectories{};
	};
}