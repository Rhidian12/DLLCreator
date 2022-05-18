#include "DLLCreator.h"

#include <iostream> /* std::cout */
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

		for (const std::filesystem::directory_entry& entry : PathEntries)
		{
			if (entry.is_directory())
			{
				std::cout << "Directory:\t";
			}
			else
			{
				std::cout << "File:\t\t";
			}

			std::cout << entry.path().string() << "\n";
		}
	}
}