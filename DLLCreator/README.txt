The tool is *not* complete.
For some reason, building CMake at the end throws a lot of errors, relating to some includes, but I do not know exactly why.

The project contains a Sandbox.rar. This is the project I have been testing with, please use this project, since others might
not be guaranteed to work.
Unzip it somewhere, preferably not in the same folder as the tool.
I also apologize, the code has not been cleaned up. There should be a seperate parser, ...

The way the tool works is as follows:

in the command line:

DLLCreator.exe "<PATH-TO-SANDBOX-ROOT-FOLDER>" (Replace <PATH-TO-SANDBOX-ROOT-FOLDER> with the ABSOLUTE path to the root folder)

Next, all of the following things, 1 at a time: (The 0 is a zero, NOT a capital o)

N
Y
0
N
Y
0
Y
Y
Y
3
/lib
/include
3
/lib
/include
3
/lib
/include
3
/lib
/include

Now it should build.
This build will fail sadly enough.