#include "DLLCreator.h"

#include "../Utils/Utils.h"

#include <iostream> /* std::cout, std::cin */
#include <algorithm> /* std::sort */
#include <assert.h> /* assert() */
#include <deque> /* std::deque */
#include <memory> /* std::unique_ptr */
#include <bitset> /* std::bitset */

/* Windows specific includes */
#include <fileapi.h> /* CreateFileA() */
#include <errhandlingapi.h> /* GetLastError() */
#include <handleapi.h> /* INVALID_HANDLE_VALUE */
// #include <tchar.h> /* _tcscmp() */

#undef max

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
		FilterFilesAndDirectories();

		/* Step 3: Find the .vcxproj file and define the preprocessor definition in it */
		DefinePreprocessorMacro();

		/* Step 4: Create the API file that defines the macro */
		CreateAPIFile();

		/* Step 5: Go through every filtered header file and start adding the include and the generated macro */
		AddMacroToFilteredHeaderFiles();
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
					path.find(".cpp") != std::string::npos ||
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

				if (line.find(std::unique_ptr<unsigned char[]>("<PreprocessorDefinitions>"_byte).get()) != std::string::npos)
				{
					std::cout << line.c_str();

					const size_t moduloPos(line.find_last_of(';'));

					assert(moduloPos != std::string::npos && "DLLCreator::DefinePreprocessorMacros() > % was not found in the Preprocessor line!");

					charPositions.push_back(moduloPos + previousNewLine + 1);
				}
			}

			previousNewLine = nextNewLine + 2;
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
			fileContents.insert(charPositions[i], ExportMacro);

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

		assert(WriteFile(testFile, fileContents.data(), fileContents.size(), &bytesWritten, nullptr) != 0 && "DLLCreator::DefinePreprocessorMacros() > The vcxproj could not be written to!");
		assert(CloseHandle(testFile) != 0 && "DLLCreator::DefinePreprocessorMacros() > Handle to file could not be closed!");
#else
		assert(WriteFile(testFile, fileContents.data(), fileContents.size(), &bytesWritten, nullptr) != 0 && "DLLCreator::DefinePreprocessorMacros() > The vcxproj could not be written to!");
#endif

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
				Utils::IO::ClearConsole();

				std::cout << "The file: " << api <<
					" already exists, but the program wants to use this name. Can the file be overwritten? Y/N >> ";

				if (Utils::IO::ReadUserInput("Y"))
				{
					bShouldLoop = false;
				}
				else
				{
					if (counter > 0)
					{
						api.append("_CUSTOMTOOL" + std::to_string(0));
					}
					else
					{
						api.append("_CUSTOMTOOL");
					}
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

	void DLLCreator::AddMacroToFilteredHeaderFiles()
	{
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

			/* Read the file into a buffer */
			const DWORD fileSize(GetFileSize(header, nullptr));

			std::unique_ptr<BYTE[]> pBuffer(new BYTE[fileSize]{});
			DWORD readBytes{};
			assert(ReadFile(header, pBuffer.get(), fileSize, &readBytes, nullptr) != 0 && "DLLCreator::AddMacroToFilteredHeaderFiles() > File could not be read!");

			Utils::IO::ClearConsole();

			/* Print file contents */
			for (DWORD i{}; i < readBytes; ++i)
			{
				std::cout << pBuffer[i];
			}

			std::cout << "\n Should class be fully exported? Y/N >> ";

			DWORD previousIndex{};
			/* Just add the macro after the class declaration */
			if (Utils::IO::ReadUserInput("Y"))
			{
				/* Make a new buffer with the length of the file + the macro + a space */
				/* This should normally be fileSize + ExportMacroLength + 1, but we don't want to count the ; in the macro, so - 1 */
				/* This -1 cancels out the + 1 */
				const DWORD newBufferSize(fileSize + ExportMacroLength);
				std::unique_ptr<BYTE[]> pNewBuffer(new BYTE[newBufferSize]{});

				for (DWORD i{}, newFileCounter{}; i < fileSize; ++i, ++newFileCounter)
				{
					/* \n is our delimiter */
					if (pBuffer[i] == static_cast<BYTE>('\n'))
					{
						std::unique_ptr<BYTE[]> pLine{ new BYTE[i - previousIndex]{} };
						assert(Utils::IO::StringCopy(pLine.get(), (pBuffer.get()) + previousIndex, i - previousIndex) && "DLLCreator::DefinePreprocessorMacros() > String could not be copied!");

						enum class ClassType : uint8_t
						{
							Class = 0,
							Struct = 1
						};
						std::bitset<2> classFlag{};

						if (Utils::IO::StringContains(pLine.get(), reinterpret_cast<const BYTE*>("class\n"), '\n'))
						{
							classFlag.set(static_cast<std::underlying_type_t<ClassType>>(ClassType::Class));
						}
						else if (Utils::IO::StringContains(pLine.get(), reinterpret_cast<const BYTE*>("struct\n"), '\n'))
						{
							classFlag.set(static_cast<std::underlying_type_t<ClassType>>(ClassType::Struct));
						}

						if (classFlag.any())
						{
							/* make sure we add the macro after the class/struct */
							/* add macro to the buffer */

							if (classFlag.test(static_cast<std::underlying_type_t<ClassType>>(ClassType::Class)))
							{
								const DWORD index{ Utils::IO::StringFind(pLine.get(), reinterpret_cast<const BYTE*>("class\n"), '\n') };

								if (index != std::numeric_limits<DWORD>::max())
								{
									/* first write the class keyword still */
									for (int j{}; j < Utils::IO::StringLength(pLine.get(), '\n') - index; ++j)
									{
										pNewBuffer[newFileCounter++] = pBuffer[i++];
									}

									/* add a space */
									pNewBuffer[newFileCounter++] = static_cast<BYTE>(' ');

									/* write the macro, without the ; */
									for (DWORD j{}; j < ExportMacroLength - 1; ++j)
									{
										pNewBuffer[newFileCounter++] = ExportMacro[j];
									}

									/* add another space */
									pNewBuffer[newFileCounter++] = static_cast<BYTE>(' ');

									/* write the rest of the line */
									for (size_t j{ index + strlen("class\0") + 2 }; j < Utils::IO::StringLength(pLine.get(), '\n'); ++j)
									{
										pNewBuffer[newFileCounter++] = pBuffer[i++];
									}
								}
							}
							else
							{

							}
						}

						previousIndex = i + 1;
					}
				}
			}
			else
			{
				std::cout << "\n What functions should be exported? Print the numbers. E.g. 0,1,3,5. Write NONE if no functions should be converted\n";


			}
		}
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