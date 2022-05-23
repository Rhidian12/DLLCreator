#include "DLLCreator.h"

#include "../Utils/Utils.h"

#include <iostream> /* std::cout, std::cin */
#include <algorithm> /* std::sort */
#include <assert.h> /* assert() */
#include <deque> /* std::deque */
#include <memory> /* std::unique_ptr */
#include <bitset> /* std::bitset */
#include <regex>

/* Windows specific includes */
#include <fileapi.h> /* CreateFileA() */
#include <errhandlingapi.h> /* GetLastError() */
#include <handleapi.h> /* INVALID_HANDLE_VALUE */
// #include <tchar.h> /* _tcscmp() */

#undef max

namespace DLL
{
	DLLCreator::DLLCreator(const std::string& rootPath)
		: RootPath(Utils::IO::ConvertToByteString(rootPath))
		, ProjectName(Utils::IO::ConvertToByteString(rootPath))
	{
		ProjectName = ProjectName.substr(ProjectName.find_last_of('\\') + 1, ProjectName.size() - ProjectName.find_last_of('\\'));
	}

	void DLLCreator::Convert()
	{
		/* [TODO]: Do all of this with Qt */

		/* Step 1: Get all folders and files in this root directory */
		GetAllFilesAndDirectories();

		/* Step 2: Ask user which files and folders need to be included in the DLL build */
		FilterFilesAndDirectories();

		/* Step 3: Find the .vcxproj file and define the preprocessor definition in it */
		DefinePreprocessorMacro();

		/* Step 4: Create the API file that defines the macro */
		CreateAPIFile();

		/* Step 5: Go through every filtered header file and start adding the include and the generated macro */
		AddMacroToFilteredHeaderFiles();

		/* Step 6: Now that the macro and include has been added, generate CMake files */
		GenerateCMakeFiles();
	}

	void DLLCreator::GetAllFilesAndDirectories()
	{
		/* Step 1 */
		const std::filesystem::path path(Utils::IO::ConvertToRegularString(RootPath));

		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			PathEntries.push_back(entry);
		}

		/* First all files, then all directories */
		std::sort(PathEntries.begin(), PathEntries.end(), [](const auto& a, const auto& b)->bool
			{
				/* Files => Directories */
				return static_cast<int>(a.is_directory()) < static_cast<int>(b.is_directory());

				/* Directories => Files */
				//return static_cast<int>(a.is_directory()) > static_cast<int>(b.is_directory());
			});

		/* Exclude certain files we know should not be converted already */
		PathEntries.erase(std::remove_if(PathEntries.begin(), PathEntries.end(), [](const std::filesystem::directory_entry& entry)
			{
				const std::string path(entry.path().string());

				return path.find(".sln") != std::string::npos ||
					path.find(".vcxproj.user") != std::string::npos ||
					path.find(".vcxproj.filters") != std::string::npos ||
					path.find(".rar") != std::string::npos ||
					path.find(".zip") != std::string::npos ||
					path.find(".txt") != std::string::npos ||
					path.find(".props") != std::string::npos ||
					path.find(".bin") != std::string::npos ||
					path.find(".exe") != std::string::npos ||
					// path.find(".lib") != std::string::npos ||
					// path.find(".dll") != std::string::npos ||
					// path.find(".cpp") != std::string::npos ||
					path.find(".vs") != std::string::npos ||
					path.find("x64") != std::string::npos ||
					path.find("Release") != std::string::npos ||
					path.find("Debug") != std::string::npos ||
					path.find("x86") != std::string::npos;
			}), PathEntries.end());

		/* Testing */
		//for (const std::filesystem::directory_entry& entry : PathEntries)
		//{
		//	if (entry.is_directory())
		//	{
		//		std::cout << "Directory:\t";
		//	}
		//	else
		//	{
		//		std::cout << "File:\t\t";
		//	}

		//	std::cout << entry.path().string() << "\n";
		//}
	}

	void DLLCreator::FilterFilesAndDirectories()
	{
		for (const std::filesystem::directory_entry& entry : PathEntries)
		{
			const std::string path(entry.path().string());

			/* Don't show .cpp, .lib or .dll files */
			if (path.find(".cpp") != std::string::npos ||
				path.find(".lib") != std::string::npos ||
				path.find(".dll") != std::string::npos)
			{
				continue;
			}

			Utils::IO::ClearConsole();

			if (entry.is_directory())
			{
				std::cout << "Directory:\t" << entry.path().string() << "\nEnter Directory? Y/N >> ";

				if (Utils::IO::ReadUserInput("Y"))
				{
					PrintDirectoryContents(entry);
				}
			}
			else
			{
				std::cout << "File:\t\t" << entry.path().string() << "\nShould file be converted? Y/N >> ";

				if (Utils::IO::ReadUserInput("Y"))
				{
					FilteredFilePaths.push_back(entry.path().string());
				}
			}
		}

		//for (const std::string& e : FilteredFilePaths)
		//{
		//	std::cout << e << "\n";
		//}
	}

	void DLLCreator::DefinePreprocessorMacro()
	{
		Utils::IO::ClearConsole();

		std::string vcxprojFilePath = FindVcxprojFilePath();

		/* open the vcxproj file */
		HANDLE vcxProjFile(
			CreateFileA(vcxprojFilePath.c_str(),
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr)
		);

		assert(vcxProjFile != INVALID_HANDLE_VALUE);

		std::basic_string<BYTE> fileContents{};

		/* Read the file into a buffer */
		const DWORD fileSize(GetFileSize(vcxProjFile, nullptr));
		fileContents.resize(fileSize);

		DWORD readBytes{};
		assert(ReadFile(vcxProjFile, fileContents.data(), fileSize, &readBytes, nullptr) != 0 && "DLLCreator::DefinePreprocessorMacros() > File could not be read!");

		const size_t count = std::count(fileContents.cbegin(), fileContents.cend(), '\n');
		std::deque<size_t> charPositions{};

		size_t previousNewLine{};
		for (size_t i{}; i < count; ++i)
		{
			const size_t nextNewLine(fileContents.find('\n', previousNewLine));
			const std::basic_string<BYTE> line(fileContents.substr(previousNewLine, nextNewLine + 1 - previousNewLine));

			{ /* Scope-lock the using directives */
				using namespace Utils;
				using namespace IO;

				if (line.find("<PreprocessorDefinitions>"_byte) != std::string::npos)
				{
					std::cout << line.c_str();

					const size_t moduloPos(line.find_last_of(';'));

					assert(moduloPos != std::string::npos && "DLLCreator::DefinePreprocessorMacros() > % was not found in the Preprocessor line!");

					charPositions.push_back(moduloPos + previousNewLine + 1);
				}
			}

			previousNewLine = nextNewLine + 1;
		}
#if 0
		/* Parse the buffer, searching for preprocessor definitions */
		std::deque<DWORD> lineIndices{};
		DWORD previousIndex{};
		for (DWORD i{}; i < readBytes; ++i)
		{
			/* \n is our delimiter */
			if (pBuffer[i] == static_cast<BYTE>('\n'))
			{
				std::unique_ptr<BYTE[]> pLine{ new BYTE[i - previousIndex]{} };
				assert(Utils::IO::StringCopy(pLine.get(), pBuffer.get() + previousIndex, i - previousIndex) && "DLLCreator::DefinePreprocessorMacros() > String could not be copied!");

				{
					using namespace Utils;
					using namespace IO;
					if (Utils::IO::StringContains(pLine.get(), "<PreprocessorDefinitions>\n"_byte, '\n'))
					{
						lineIndices.push_back(previousIndex);
					}
				}

				previousIndex = i + 1;
			}
		}

		/* Make a new buffer with the length of the vcxproj + the export macro added (amount of times as there are preprocesser definitions defined) */
		const DWORD newBufferSize(fileSize + static_cast<DWORD>(lineIndices.size()) * ExportMacroLength);
		std::unique_ptr<BYTE[]> pNewBuffer(new BYTE[newBufferSize]{});
		for (DWORD i{}, newFileCounter{}; i < fileSize; ++i, ++newFileCounter)
		{
			if (lineIndices.empty() || i < lineIndices.front())
			{
				pNewBuffer[newFileCounter] = pBuffer[i];
			}
			else
			{
				/* just keep filling up the buffer until we meet the % */
				if (pBuffer[i] != static_cast<BYTE>('%'))
				{
					pNewBuffer[newFileCounter] = pBuffer[i];
				}
				else
				{
					/* add macro to the buffer */
					for (DWORD j{}; j < ExportMacroLength; ++j)
					{
						pNewBuffer[newFileCounter++] = ExportMacro[j];
					}

					pNewBuffer[newFileCounter] = static_cast<BYTE>('%');

					lineIndices.pop_front();

					for (DWORD& elem : lineIndices)
					{
						elem += ExportMacroLength;
					}
				}
			}
		}
#endif

		for (size_t i{}; i < charPositions.size();)
		{
			fileContents.insert(charPositions[i], PreproExportMacro);

			for (size_t j{ i }; j < charPositions.size(); ++j)
			{
				charPositions[j] += ExportMacroLength;
			}

			charPositions.pop_front();
		}

		DWORD bytesWritten{};
#ifdef WRITE_TO_TEST_FILE
		/* open the test file */
		HANDLE testFile(
			CreateFileA("Test.txt",
				GENERIC_WRITE,
				FILE_SHARE_WRITE,
				nullptr,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				nullptr)
		);

		assert(testFile != INVALID_HANDLE_VALUE);

		assert(WriteFile(testFile, fileContents.data(), static_cast<DWORD>(fileContents.size()), &bytesWritten, nullptr) != 0 && "DLLCreator::DefinePreprocessorMacros() > The vcxproj could not be written to!");
		assert(CloseHandle(testFile) != 0 && "DLLCreator::DefinePreprocessorMacros() > Handle to file could not be closed!");
#else
		assert(WriteFile(testFile, fileContents.data(), fileContents.size(), &bytesWritten, nullptr) != 0 && "DLLCreator::DefinePreprocessorMacros() > The vcxproj could not be written to!");
#endif

		assert(CloseHandle(vcxProjFile) != 0 && "DLLCreator::DefinePreprocessorMacros() > Handle to file could not be closed!");
	}

	void DLLCreator::CreateAPIFile()
	{
		using namespace Utils;
		using namespace IO;

		/* Make the API file in the Root Directory */
		std::basic_string<BYTE> api{};

		/* Make sure there is no file already with the apiFileName */
		int counter{};
		bool bShouldLoop(true);
		do
		{
			api = RootPath.substr(RootPath.find_last_of('\\') + 1, RootPath.size() - RootPath.find_last_of('\\')) +
				"_API"_byte;

			const auto cIt(std::find_if(PathEntries.cbegin(), PathEntries.cend(), [&api](const std::filesystem::directory_entry& entry)
				{
					return ConvertToByteString(entry.path().string()).find(api) != std::string::npos;
				}));

			/* The file already exists, ask the user if it can be overwritten */
			if (cIt != PathEntries.cend())
			{
				ClearConsole();

				std::cout << "The file: " << api.c_str() <<
					" already exists, but the program wants to use this name. Can the file be overwritten? Y/N >> ";

				if (ReadUserInput("Y"))
				{
					bShouldLoop = false;
				}
				else
				{
					if (counter > 0)
					{
						api.append("_CUSTOMTOOL"_byte.append(ConvertToByteString(std::to_string(0))));
					}
					else
					{
						api.append("_CUSTOMTOOL"_byte);
					}
				}
			}
			else
			{
				bShouldLoop = false;
			}
		} while (bShouldLoop);

		APIMacro = api;

		{ /* Scope-lock using directives */
			using namespace Utils;
			using namespace IO;
			APIFileName = api.append(".h"_byte);
			APIFileNamePath = RootPath + "\\"_byte + APIFileName;
		}

		/* Now open the actual file and write the contents */
		HANDLE apiFile(
			CreateFileA(ConvertToRegularString(APIFileNamePath).c_str(),
				GENERIC_WRITE,
				FILE_SHARE_WRITE,
				nullptr,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				nullptr)
		);

		assert(apiFile != INVALID_HANDLE_VALUE);

		SetFilePointer(apiFile, 0, nullptr, FILE_BEGIN);
		SetEndOfFile(apiFile);

		const std::basic_string<BYTE> apiContents(
			std::basic_string<BYTE>("#pragma once\n\n"_byte) +
			std::basic_string<BYTE>("#ifdef _WIN32\n"_byte) +
			std::basic_string<BYTE>("\t#ifdef EXPORT\n"_byte) +
			std::basic_string<BYTE>("\t\t#define "_byte) + APIMacro + " __declspec(dllexport)\n"_byte +
			std::basic_string<BYTE>("\t#else\n"_byte) +
			std::basic_string<BYTE>("\t\t#define "_byte) + APIMacro + " __declspec(dllimport)\n"_byte +
			std::basic_string<BYTE>("\t#endif\n"_byte) +
			std::basic_string<BYTE>("#else\n"_byte) +
			std::basic_string<BYTE>("\t#define "_byte) + APIMacro + "\n"_byte +
			std::basic_string<BYTE>("#endif"_byte));

		DWORD bytesWritten{};
		assert(WriteFile(apiFile, apiContents.c_str(), static_cast<DWORD>(apiContents.size()), &bytesWritten, nullptr) != 0 && "DLLCreator::CreateAPIFile() > The API file could not be written to!");

		assert(CloseHandle(apiFile) != 0 && "DLLCreator::CreateAPIFile() > Handle to file could not be closed!");
	}

	void DLLCreator::AddMacroToFilteredHeaderFiles()
	{
		using namespace Utils;
		using namespace IO;

		int fileCounter{};
		for (const std::string& entry : FilteredFilePaths)
		{
			/* print file contents */

			/* open the header */
			HANDLE header(
				CreateFileA(entry.c_str(),
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					nullptr,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					nullptr)
			);

			assert(header != INVALID_HANDLE_VALUE);

			std::basic_string<BYTE> fileContents{};

			/* Read the file into a buffer */
			const DWORD fileSize(GetFileSize(header, nullptr));
			fileContents.resize(fileSize);

			DWORD readBytes{};
			assert(ReadFile(header, fileContents.data(), fileSize, &readBytes, nullptr) != 0 && "DLLCreator::AddMacroToFilteredHeaderFiles() > File could not be read!");

			ClearConsole();

			/* Print file contents */
			std::cout << fileContents.c_str();

			std::cout << "\nShould class be fully exported? Y/N >> ";

			const size_t count = std::count(fileContents.cbegin(), fileContents.cend(), '\n');

			std::cout << "\n--------------\n";

			size_t previousNewLine{};
			/* Just add the macro after the class declaration */
			if (ReadUserInput("Y"))
			{
				for (size_t i{}; i < count; ++i)
				{
					size_t nextNewLine(fileContents.find('\n', previousNewLine));
					const std::basic_string<BYTE> line(fileContents.substr(previousNewLine, nextNewLine + 1 - previousNewLine));

					enum class ClassType : uint8_t
					{
						Class = 0,
						Struct = 1
					};
					std::bitset<2> classFlag{};

					size_t classTypeIndex{};

					{ /* Scope-lock using directives */
						using namespace Utils;
						using namespace IO;

						if (classTypeIndex = line.find("class"_byte); classTypeIndex != std::string::npos)
						{
							classFlag.set(static_cast<std::underlying_type_t<ClassType>>(ClassType::Class));
						}
						else if (classTypeIndex = line.find("struct"_byte); classTypeIndex != std::string::npos)
						{
							classFlag.set(static_cast<std::underlying_type_t<ClassType>>(ClassType::Struct));
						}
					}

					if (classFlag.any())
					{
						assert(classTypeIndex != std::string::npos && "DLLCreator::AddMacroToFilteredHeaderFiles() > struct/class could not be found!");

						/* make sure we add the macro after the class/struct */
						/* add macro to the buffer */
						{
							using namespace Utils;
							using namespace IO;

							if (classFlag.test(static_cast<std::underlying_type_t<ClassType>>(ClassType::Class)))
							{
								/* + 5 == length of 'class' */
								constexpr size_t classLen{ 5 };
								fileContents.insert(previousNewLine + classTypeIndex + classLen, " "_byte + APIMacro);
								nextNewLine += classLen + 1;
							}
							else
							{
								/* + 6 == length of 'struct' */
								constexpr size_t structLen{ 6 };
								fileContents.insert(previousNewLine + classTypeIndex + structLen, " "_byte + APIMacro);
								nextNewLine += structLen + 1;
							}
						}

						break;
					}

					previousNewLine = nextNewLine + 1;
				}
			}
			else
			{
				std::cout << "\nWhat functions should be exported? Print the numbers. E.g. 0,1,3,5. Write NONE if no functions should be converted\n";

				/* Get all functions from a file */
				/* [CRINGE]: This is way too simple at the moment, complicated files containing templates and comments will not be parsed correctly */
				const std::regex functionFinder{ "\\s*((?:\\w*(?:[:]{2})*)\\s*\\w*\\s*\\w*[&*]{0,2}\\s*\\w+\\(.*\\))" };

				std::smatch matches{};
				std::vector<std::string> functions{};

				std::string convertedFileContents(Utils::IO::ConvertToRegularString(fileContents));
				size_t counter{};
				while (std::regex_search(convertedFileContents, matches, functionFinder)) /* Temp strings are not allowed */
				{
					std::string match(matches.str());
					match.erase(match.begin(), std::find_if(match.begin(), match.end(), [](const char c)->bool
						{
							return !std::isspace(c);
						}));

					/* store the function for later */
					functions.push_back(match);

					/* Print the function */
					std::cout << std::to_string(counter++) << ". " << match << "\n";

					convertedFileContents = matches.suffix();
				}

				const std::string input(Utils::IO::ReadUserInput());

				if (input != "NONE")
				{
					const std::vector<size_t> selectedFunctions(GetNumbersFromCSVString(input));

					for (const size_t i : selectedFunctions)
					{
						if (i < functions.size())
						{
							/* insert the macro before this function */
							fileContents.insert(fileContents.find(ConvertToByteString(functions[i])), APIMacro + " "_byte);
						}
					}
				}
			}

			/* We also need to add the include to the API file */
			const size_t directoriesDeep(GetNumberOfDirectoriesDeep(entry));
			std::basic_string<BYTE> include{ "#include "_byte };

			for (size_t i{}; i < directoriesDeep; ++i)
			{
				include.append("../"_byte);
			}

			include.append(APIFileName);

			const std::basic_string<BYTE> pragmaOnce("#pragma once"_byte);
			/* Check if the file contains a #pragma once */
			if (size_t pragmaPos = fileContents.find(pragmaOnce); pragmaPos != std::string::npos)
			{
				fileContents.insert(pragmaPos + pragmaOnce.size() + 1, include);
			}
			else
			{
				/* just insert at the top of the file */
				fileContents.insert(0, include);
			}

			/* open the altered header */
			HANDLE newHeader(
				CreateFileA((std::string("Test") + std::to_string(fileCounter++) + ".txt").c_str(),
					GENERIC_WRITE,
					FILE_SHARE_WRITE,
					nullptr,
					OPEN_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					nullptr)
			);

			assert(newHeader != INVALID_HANDLE_VALUE);

			SetFilePointer(newHeader, 0, nullptr, FILE_BEGIN);
			SetEndOfFile(newHeader);

			DWORD bytesWritten{};
			assert(WriteFile(newHeader, fileContents.c_str(), static_cast<DWORD>(fileContents.size()), &bytesWritten, nullptr) != 0 && "DLLCreator::AddMacroToFilteredHeaderFiles() > The new header file could not be written to!");
			assert(CloseHandle(newHeader) != 0 && "DLLCreator::AddMacroToFilteredHeaderFiles() > Handle to file could not be closed!");

			assert(CloseHandle(header) != 0 && "DLLCreator::AddMacroToFilteredHeaderFiles() > Handle to file could not be closed!");
		}
	}

	void DLLCreator::GenerateCMakeFiles()
	{
		Utils::IO::ClearConsole();

		/* First generate the Root CMake file */
		GenerateRootCMakeFile();

		/* Now generate the sub directory CMake files */
		GenerateSubDirectoryCMakeFiles();
	}

	void DLLCreator::GenerateRootCMakeFile()
	{
		using namespace Utils;
		using namespace IO;

		/* open the preset file */
		HANDLE cmakeRootPresetFile(
			CreateFileA("Resources/CMakeRootPreset.txt",
				GENERIC_READ,
				FILE_SHARE_READ,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr)
		);

		assert(cmakeRootPresetFile != INVALID_HANDLE_VALUE);

		std::basic_string<BYTE> fileContents{};

		/* Read the file into a buffer */
		const DWORD fileSize(GetFileSize(cmakeRootPresetFile, nullptr));
		fileContents.resize(fileSize);

		DWORD readBytes{};
		assert(ReadFile(cmakeRootPresetFile, fileContents.data(), fileSize, &readBytes, nullptr) != 0 && "DLLCreator::GenerateRootCMakeFile() > File could not be read!");
		assert(CloseHandle(cmakeRootPresetFile) != 0 && "DLLCreator::GenerateRootCMakeFile() > Handle to file could not be closed!");

		std::string convertedFileContents(ConvertToRegularString(fileContents));

		/* Substitute the version in */
		const std::regex versionMajorRegex("<VERSION_MAJOR>");
		const std::regex versionMinorRegex("<VERSION_MINOR>");
		const std::regex projectNameRegex("<PROJECT_NAME>");

		convertedFileContents = std::regex_replace(convertedFileContents, versionMajorRegex, "3");
		convertedFileContents = std::regex_replace(convertedFileContents, versionMinorRegex, "10");
		convertedFileContents = std::regex_replace(convertedFileContents, projectNameRegex, ConvertToRegularString(ProjectName));

		/* Spacing */
		convertedFileContents.append("\n");

		/* Loop over all sub directories, and add them as sub directories */
		for (const std::filesystem::directory_entry& subD : PathEntries)
		{
			if (subD.is_directory())
			{
				const std::string path(subD.path().string());
				convertedFileContents.append(std::string("\nadd_subdirectory(") +
					path.substr(path.find_last_of('\\') + 1, path.size() - path.find_last_of('\\')) + ")");
			}
		}

		/* make a new root file */
		HANDLE cmakeRootFile(
			CreateFileA((ConvertToRegularString(RootPath) + "\\CMakeLists.txt").c_str(),
				GENERIC_WRITE,
				FILE_SHARE_WRITE,
				nullptr,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				nullptr)
		);

		assert(cmakeRootFile != INVALID_HANDLE_VALUE);

		SetFilePointer(cmakeRootFile, 0, nullptr, FILE_BEGIN);
		SetEndOfFile(cmakeRootFile);

		DWORD bytesWritten{};
		assert(WriteFile(cmakeRootFile, convertedFileContents.c_str(), static_cast<DWORD>(convertedFileContents.size()), &bytesWritten, nullptr) != 0 && "DLLCreator::AddMacroToFilteredHeaderFiles() > The new header file could not be written to!");
		assert(CloseHandle(cmakeRootFile) != 0 && "DLLCreator::GenerateRootCMakeFile() > Handle to file could not be closed!");
	}

	void DLLCreator::GenerateSubDirectoryCMakeFiles()
	{
		using namespace Utils;
		using namespace IO;

		for (const std::filesystem::directory_entry& entry : PathEntries)
		{
			/* Check every file in this subdirectory, if it contains only headers and libs, it is an interface */
			/* if it contains libs, headers AND dlls, it is also an interface, but the dlls need to be copied */
			/* if it contains cpp files, we need to add a normal library (NOT AN INTERFACE) and link the headers */

			if (entry.is_directory())
			{
				const int8_t subDirectoryType(CheckSubDirectory(entry));

				switch (subDirectoryType)
				{
				case 0 /* headers only */:
					/* nothing should happen */
					break;
				case 1 /* contains .cpp */:
					/* Add every .cpp file to the library and set the include directories */
					GenerateSubDirectoryCppCMakeFile(entry);
					break;
				case 2 /* contains .h and .lib */:
					/* add the library name and set the include directories */
					break;
				case 3 /* contains .lib, .h and .dll */:
					/*  */
					break;
				}
			}
		}
	}

	void DLLCreator::GenerateSubDirectoryCppCMakeFile(const std::filesystem::directory_entry& _entry)
	{
		using namespace Utils;
		using namespace IO;

		std::vector<std::string> filesToAddToLibrary{};

		const std::string path(_entry.path().string());
		const std::string libName(path.substr(path.find_last_of('\\') + 1, path.size() - path.find_last_of('\\')));

		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(_entry))
		{
			if (entry.is_regular_file())
			{
				const std::string path(entry.path().string());

				/* We don't need to check for any other file, because it doesn't make sense to have .cpp with .lib or .dll files */
				if (path.find(".cpp") != std::string::npos)
				{
					filesToAddToLibrary.push_back(path.substr(path.find_last_of('\\') + 1, path.size()));
				}
			}
			/* If we find another directory, start this sequence again */
			else if (entry.is_directory())
			{
				const int8_t subDirectoryType(CheckSubDirectory(entry));

				switch (subDirectoryType)
				{
				case 0 /* headers only */:
					/* nothing should happen */
					break;
				case 1 /* contains .cpp */:
					/* Add every .cpp file to the library and set the include directories */
					GenerateSubDirectoryCppCMakeFile(entry);
					break;
				case 2 /* contains .h and .lib */:
					/* add the library name and set the include directories */
					break;
				case 3 /* contains .lib, .h and .dll */:
					/*  */
					break;
				}
			}
		}

		std::cout << "For the directory: " << _entry.path().string() << " are all the headers in the same directory as the .cpp files?\n";
		std::cout << "Y/N >> ";

		std::string userInput{ ReadUserInput() };
		while (userInput != "Y" && userInput != "N")
		{
			ClearConsole();

			std::cout << "Invalid input! Input must be Y or N\n";
			std::cout << "For the directory: " << _entry.path().string() << " are all the headers in the same directory as the .cpp files?\n";
			std::cout << "Y/N >> ";

			userInput = ReadUserInput();
		}

		ClearConsole();

		std::string headerLocation{};
		if (userInput == "N")
		{
			std::cout << "INFO: Headers must be located in the same location\n";
			std::cout << "Please give the RELATIVE location of all the headers\n";
			std::cout << "For example: Point2f >\n\tPoint2f.cpp\n\tInclude\n\t\tPoint2f.h\n";
			std::cout << "Relative location would be Include\n";
			std::cout << "Relative location of headers: ";

			headerLocation = ReadUserInput();
		}

		/* open the cpp preset file */
		HANDLE cmakeCppPresetFile(
			CreateFileA("Resources/CMakeSubDirectoryCppPreset.txt",
				GENERIC_READ,
				FILE_SHARE_READ,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr)
		);

		assert(cmakeCppPresetFile != INVALID_HANDLE_VALUE);

		std::basic_string<BYTE> fileContents{};

		/* Read the file into a buffer */
		const DWORD fileSize(GetFileSize(cmakeCppPresetFile, nullptr));
		fileContents.resize(fileSize);

		DWORD readBytes{};
		assert(ReadFile(cmakeCppPresetFile, fileContents.data(), fileSize, &readBytes, nullptr) != 0 && "DLLCreator::GenerateSubDirectoryCppCMakeFile() > File could not be read!");
		assert(CloseHandle(cmakeCppPresetFile) != 0 && "DLLCreator::GenerateSubDirectoryCppCMakeFile() > Handle to file could not be closed!");

		std::string convertedFileContents(ConvertToRegularString(fileContents));

		/* Substitute the lib name, header location and project name in */
		const std::regex libNameRegex("<LIBRARY_NAME>");
		const std::regex headerLocationRegex("<HEADER_LOCATION>");
		const std::regex projectNameRegex("<PROJECT_NAME>");

		convertedFileContents = std::regex_replace(convertedFileContents, libNameRegex, libName);
		convertedFileContents = std::regex_replace(convertedFileContents, headerLocationRegex, headerLocation);
		convertedFileContents = std::regex_replace(convertedFileContents, projectNameRegex, ConvertToRegularString(ProjectName));

		const size_t insertPos{ convertedFileContents.find_first_of(')') };

		assert(insertPos != std::string::npos && "DLLCreator::GenerateSubDirectoryCppCMakeFile() > Error making CMake file");

		for (const std::string& cppFile : filesToAddToLibrary)
		{
			convertedFileContents.insert(insertPos, cppFile + " ");
		}

		/* make the cpp file */
		HANDLE cmakeCppFile(
			CreateFileA((_entry.path().string() + "\\CMakeLists.txt").c_str(),
				GENERIC_WRITE,
				FILE_SHARE_WRITE,
				nullptr,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				nullptr)
		);

		assert(cmakeCppFile != INVALID_HANDLE_VALUE);

		SetFilePointer(cmakeCppFile, 0, nullptr, FILE_BEGIN);
		SetEndOfFile(cmakeCppFile);

		DWORD bytesWritten{};
		assert(WriteFile(cmakeCppFile, convertedFileContents.c_str(), static_cast<DWORD>(convertedFileContents.size()), &bytesWritten, nullptr) != 0 && "DLLCreator::AddMacroToFilteredHeaderFiles() > The new header file could not be written to!");
		assert(CloseHandle(cmakeCppFile) != 0 && "DLLCreator::GenerateRootCMakeFile() > Handle to file could not be closed!");
	}

	/* returns
	0 if directory contains only .h files
	1 if directory contains .cpp files,
	2 if directory contains only .h and .lib,
	3 if directory contains .lib, .h and .dll */
	int8_t DLLCreator::CheckSubDirectory(const std::filesystem::directory_entry& _entry)
	{
		bool bHasDlls(false), bHasLibs(false), bHasOnlyHeaders(false);
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(_entry))
		{
			if (entry.is_regular_file())
			{
				const std::string path(entry.path().string());

				if (path.find(".cpp"))
				{
					return 1;
				}
				else if (path.find(".dll"))
				{
					bHasDlls = true;
					bHasOnlyHeaders = false;
				}
				else if (path.find(".lib"))
				{
					bHasLibs = true;
					bHasOnlyHeaders = false;
				}
				else if (path.find(".h"))
				{
					bHasOnlyHeaders = true;
				}
			}
			else if (entry.is_directory())
			{
				CheckSubDirectory(entry);
			}
		}

		if (bHasOnlyHeaders)
		{
			return 0;
		}
		else if (bHasLibs)
		{
			return 2;
		}
		else if (bHasDlls)
		{
			return 3;
		}

		return -1;
	}

	std::string DLLCreator::FindVcxprojFilePath() const
	{
		/* First search through the root path entries */

		std::string vcxprojFilePath{};
		const std::string suffix(".vcxproj");
		for (const std::filesystem::directory_entry& entry : PathEntries)
		{
			if (std::equal(suffix.rbegin(), suffix.rend(), entry.path().string().rbegin()))
			{
				vcxprojFilePath = entry.path().string();
				break;
			}
		}

		/* Check if we found a vcxproj file */
		if (vcxprojFilePath.empty())
		{
			/* if we didn't, recursively search through every file to find it */
			for (const std::filesystem::directory_entry& path : PathEntries)
			{
				for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(path))
				{
					if (std::equal(suffix.rbegin(), suffix.rend(), entry.path().string().rbegin()))
					{
						vcxprojFilePath = entry.path().string();
						break;
					}
				}
			}

			/* if after all this we *still* haven't found the vcxproj, ask the user for the location */
			std::string input;
			do
			{
				std::cout << "\nPlease enter the absolute path to the .vcxproj file\n";
				input = Utils::IO::ReadUserInput();
			} while (input.find(".vcxproj") != std::string::npos);

			vcxprojFilePath = input;
		}

		return vcxprojFilePath;
	}

	void DLLCreator::PrintDirectoryContents(const std::filesystem::directory_entry& entry)
	{
		Utils::IO::ClearConsole();

		std::vector<std::filesystem::directory_entry> entries{};

		for (const std::filesystem::directory_entry& _entry : std::filesystem::directory_iterator(entry))
		{
			entries.push_back(_entry);
		}

		/* Sort all entries, first all files, then all directories */
		std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b)->bool
			{
				/* Files => Directories */
				return static_cast<int>(a.is_directory()) < static_cast<int>(b.is_directory());

				/* Directories => Files */
				//return static_cast<int>(a.is_directory()) > static_cast<int>(b.is_directory());
			});

		/* Exclude certain files we know should not be converted already */
		entries.erase(std::remove_if(entries.begin(), entries.end(), [](const std::filesystem::directory_entry& entry)
			{
				const std::string path(entry.path().string());

				return path.find(".sln") != std::string::npos ||
					path.find(".vcxproj.user") != std::string::npos ||
					path.find(".vcxproj.filters") != std::string::npos ||
					path.find(".rar") != std::string::npos ||
					path.find(".zip") != std::string::npos ||
					path.find(".txt") != std::string::npos ||
					path.find(".props") != std::string::npos ||
					path.find(".bin") != std::string::npos ||
					path.find(".exe") != std::string::npos ||
					path.find(".lib") != std::string::npos ||
					path.find(".dll") != std::string::npos ||
					path.find(".cpp") != std::string::npos ||
					path.find(".vs") != std::string::npos ||
					path.find("x64") != std::string::npos ||
					path.find("Release") != std::string::npos ||
					path.find("Debug") != std::string::npos ||
					path.find("x86") != std::string::npos;
			}), entries.end());

		bool bAreFilesPresent(false), bAreDirectoriesPresent(false);
		/* Print everything inside of this directory */
		for (size_t i{}; i < entries.size(); ++i)
		{
			if (entries[i].is_regular_file())
			{
				std::cout << std::to_string(i) + ". File:\t";
				bAreFilesPresent = true;
			}
			else
			{
				std::cout << std::to_string(i) + ". Directory:\t";
				bAreDirectoriesPresent = true;
			}

			std::cout << entries[i].path().string() << "\n";
		}

		/* Get what files should be converted */
		if (bAreFilesPresent)
		{
			std::cout << "What files --NO DIRECTORIES-- should be converted? Print the numbers. E.g. 0,1,3,5. Write NONE if no files should be converted\n";

			/* Get all the numbers from user input and save the requested entries */
			std::string input(Utils::IO::ReadUserInput());
			if (input != "NONE")
			{
				std::vector<size_t> indices(GetNumbersFromCSVString(input));
				for (const size_t i : indices)
				{
					/* make sure the index is valid */
					if (i < entries.size() && entries[i].is_regular_file())
					{
						FilteredFilePaths.push_back(entries[i].path().string());
					}
				}
			}
		}

		/* Enter the directories the user wants to check */
		if (bAreDirectoriesPresent)
		{
			std::cout << "What directories -- NO FILES-- should be entered? Print the numbers. E.g. 2,4,6. Write NONE if no directories should be entered\n";

			std::string input(Utils::IO::ReadUserInput());
			if (input != "NONE")
			{
				std::vector<size_t> indices(GetNumbersFromCSVString(input));
				for (const size_t i : indices)
				{
					/* make sure the index is valid */
					if (i < entries.size() && entries[i].is_directory())
					{
						PrintDirectoryContents(entries[i]);
					}
				}
			}
		}
	}

	std::vector<size_t> DLLCreator::GetNumbersFromCSVString(const std::string& _userInput) const
	{
		std::vector<size_t> numbers{};

		std::istringstream userInput(_userInput);
		std::string subString;
		while (std::getline(userInput, subString, ','))
		{
			bool bIsSubStringValid(true);
			for (const char c : subString)
			{
				if (!std::isdigit(c))
				{
					bIsSubStringValid = false;
					break;
				}
			}

			/* if the substring consists only out of digits */
			if (bIsSubStringValid)
			{
				const int index(std::stoi(subString));

				numbers.push_back(index);
			}
		}

		return numbers;
	}

	size_t DLLCreator::GetNumberOfDirectoriesDeep(const std::string& filePath) const
	{
		const size_t countRootPath(std::count(RootPath.cbegin(), RootPath.cend(), '\\'));
		const size_t countOtherPath(std::count(filePath.cbegin(), filePath.cend(), '\\'));

		return countOtherPath - countRootPath;
	}
}