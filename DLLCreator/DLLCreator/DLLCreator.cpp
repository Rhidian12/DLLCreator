#include "DLLCreator.h"

#include "../Utils/Utils.h"

#include <iostream> /* std::cout, std::cin */
#include <algorithm> /* std::sort */
#include <assert.h> /* assert() */
#include <deque> /* std::deque */
#include <memory> /* std::unique_ptr */

/* Windows specific includes */
#include <fileapi.h> /* CreateFileA() */
#include <errhandlingapi.h> /* GetLastError() */
#include <handleapi.h> /* INVALID_HANDLE_VALUE */
// #include <tchar.h> /* _tcscmp() */

namespace DLL
{
	DLLCreator::DLLCreator(const std::string& rootPath)
		: RootPath(rootPath)
	{}

	void DLLCreator::Convert()
	{
		/* [TODO]: Do all of this with Qt */

		/* Step 1: Get all folders and files in this root directory */
		GetAllFilesAndDirectories();

		/* Step 2: Ask user which files and folders need to be included in the DLL build */
		// FilterFilesAndDirectories();

		/* Step 3: Find the .vcxproj file and define the preprocessor definition in it */
		DefinePreprocessorMacro();

		/* Step 4: Create the API file that defines the macro */
		CreateAPIFile();
	}

	void DLLCreator::GetAllFilesAndDirectories()
	{
		/* Step 1 */
		const std::filesystem::path path(RootPath);

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
					path.find(".lib") != std::string::npos ||
					path.find(".dll") != std::string::npos ||
					path.find(".cpp") != std::string::npos;
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

		/* Read the file into a buffer */
		const DWORD fileSize(GetFileSize(vcxProjFile, nullptr));

		std::unique_ptr<BYTE[]> pBuffer(new BYTE[fileSize]{});
		DWORD readBytes{};
		assert(ReadFile(vcxProjFile, pBuffer.get(), fileSize, &readBytes, nullptr) != 0 && "DLLCreator::DefinePreprocessorMacros() > File could not be read!");

		/* Parse the buffer, searching for preprocessor definitions */
		std::deque<DWORD> lineIndices{};
		DWORD previousIndex{};
		for (DWORD i{}; i < readBytes; ++i)
		{
			/* \n is our delimiter */
			if (pBuffer[i] == static_cast<BYTE>('\n'))
			{
				std::unique_ptr<char[]> pLine{new char[i - previousIndex]{}};
				assert(Utils::IO::StringCopy(pLine.get(), reinterpret_cast<const char*>(pBuffer.get()) + previousIndex, i - previousIndex) && "DLLCreator::DefinePreprocessorMacros() > String could not be copied!");
				if (Utils::IO::StringContains(pLine.get(), "<PreprocessorDefinitions>\n", '\n'))
				{
					lineIndices.push_back(previousIndex);
				}

				// delete[] pLine;

				previousIndex = i + 1;
			}
		}

		constexpr DWORD exportLength{ 7 };
		constexpr BYTE exportMacro[9]("EXPORT;\n");

		/* Make a new buffer with the length of the vcxproj + the export macro added (amount of times as there are preprocesser definitions defined) */
		const DWORD newBufferSize(fileSize + static_cast<DWORD>(lineIndices.size()) * exportLength);
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
					for (DWORD j{}; j < exportLength; ++j)
					{
						pNewBuffer[newFileCounter++] = exportMacro[j];
					}

					pNewBuffer[newFileCounter] = static_cast<BYTE>('%');

					lineIndices.pop_front();

					for (DWORD& elem : lineIndices)
					{
						elem += exportLength;
					}
				}
			}
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

		assert(WriteFile(testFile, pNewBuffer.get(), newBufferSize, &bytesWritten, nullptr) != 0 && "DLLCreator::DefinePreprocessorMacros() > The vcxproj could not be written to!");
		assert(CloseHandle(testFile) != 0 && "DLLCreator::DefinePreprocessorMacros() > Handle to file could not be closed!");
#else
		assert(WriteFile(vcxProjFile, pNewBuffer, newBufferSize, &bytesWritten, nullptr) != 0 && "DLLCreator::DefinePreprocessorMacros() > The vcxproj could not be written to!");
#endif

		// delete[] pBuffer;
		// delete[] pNewBuffer;

		assert(CloseHandle(vcxProjFile) != 0 && "DLLCreator::DefinePreprocessorMacros() > Handle to file could not be closed!");
	}

	void DLLCreator::CreateAPIFile()
	{
		/* Make the API file in the Root Directory */
		std::string api{};

		/* Make sure there is no file already with the apiFileName */
		int counter{};
		bool bShouldLoop(true);
		do
		{
			api = RootPath.substr(RootPath.find_last_of('\\') + 1, RootPath.size() - RootPath.find_last_of('\\')) +
				"_API";

			const auto cIt(std::find_if(PathEntries.cbegin(), PathEntries.cend(), [&api](const std::filesystem::directory_entry& entry)
				{
					return entry.path().string().find(api) != std::string::npos;
				}));

			/* The file already exists, ask the user if it can be overwritten */
			if (cIt != PathEntries.cend())
			{
				if (counter > 0)
				{
					api.append("_CUSTOMTOOL" + std::to_string(0));
				}
				else
				{
					api.append("_CUSTOMTOOL");
				}

				Utils::IO::ClearConsole();

				std::cout << "The file: " << api <<
					" already exists, but the program wants to use this name. Can the file be overwritten? Y/N >> ";

				if (Utils::IO::ReadUserInput("Y"))
				{
					bShouldLoop = false;
				}
			}
			else
			{
				bShouldLoop = false;
			}
		} while (bShouldLoop);

		APIMacro = api;
		APIFileName = api.append(".h");
		APIFileNamePath = RootPath + "\\" + APIFileName;

		/* Now open the actual file and write the contents */
		HANDLE apiFile(
			CreateFileA(APIFileNamePath.c_str(),
				GENERIC_WRITE,
				FILE_SHARE_WRITE,
				nullptr,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				nullptr)
		);

		assert(apiFile != INVALID_HANDLE_VALUE);

		const std::string apiContents(
			std::string("#pragma once\n\n") +
			std::string("#ifdef _WIN32\n") + 
			std::string("\t#ifdef EXPORT\n") + 
			std::string("\t\t#define ") + APIMacro + " __declspec(dllexport)\n" + 
			std::string("\t#else\n") +
			std::string("\t\t#define ") + APIMacro + " __declspec(dllimport)\n" + 
			std::string("\t#endif\n") + 
			std::string("#else\n") + 
			std::string("\t#define ") + APIMacro + "\n" + 
			std::string("#endif"));

		DWORD bytesWritten{};
		assert(WriteFile(apiFile, apiContents.c_str(), static_cast<DWORD>(apiContents.size()), &bytesWritten, nullptr) != 0 && "DLLCreator::CreateAPIFile() > The API file could not be written to!");

		assert(CloseHandle(apiFile) != 0 && "DLLCreator::CreateAPIFile() > Handle to file could not be closed!");
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
}