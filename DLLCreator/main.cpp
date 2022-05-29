#include "DLLCreator/DLLCreator.h"

#include <iostream>
#include <vld.h>
#include <filesystem> 

int main(int argc, char* argv[])
{
	if (argc >= 2)
	{
		std::string input{};
		std::string output{};

		int counter{ 1 };
		for (counter; counter < argc; ++counter)
		{
			input += argv[counter];

			if (std::filesystem::exists(input))
			{
				++counter;
				break;
			}
		}

		std::cout << "INPUT: " << input << "\n";

		/* has the input been specified? */
		if (counter < argc) 
		{
			for (counter; counter < argc; ++counter)
			{
				output += argv[counter];

				if (std::filesystem::exists(output))
				{
					break;
				}
			}
		}

		std::cout << "OUTPUT: " << output << "\n";

		DLL::DLLCreator dll(input, output);

		dll.Convert();
	}
	else
	{
		std::cout << "This program requires the following command line: \n";
		std::cout << "DLLCreator.exe -I -O\n";
		std::cout << "-I is the path to the Visual Studio root folder to be converted.\n";
		std::cout << "-O is the optional path to the output folder\n";
	}
}