#include "DLLCreator.h"

#include "../Utils/Utils.h"

#include <iostream> /* std::cout, std::cin */
#include <algorithm> /* std::sort */

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