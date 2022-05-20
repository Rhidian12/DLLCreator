#include "DLLCreator/DLLCreator.h"

#include <iostream>
#include <vld.h>

int main()
{
    /* desktop */
    DLL::DLLCreator dll("C:\\Users\\rhidi\\Documents\\Sandbox\\Sandbox_Desktop");

    /* laptop */
    //DLL::DLLCreator dll("D:\\000 Documents\\000 All Programs\\Sandbox");

    dll.Convert();
}