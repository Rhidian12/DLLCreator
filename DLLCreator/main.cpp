#include "DLLCreator/DLLCreator.h"

#include <iostream>
#include <vld.h>

int main()
{
    /* desktop */
    DLL::DLLCreator dll("C:\\Users\\rhidi\\Desktop\\temp\\Boopigin - Copy - Copy - Copy");

    /* laptop */
    // DLL::DLLCreator dll("D:\\000 Documents\\000 All Programs\\Sandbox");
    // DLL::DLLCreator dll("C:\\Users\\Rhidian De Wit\\Desktop\\Temp\\Boopigin - Copy");
    
    dll.Convert();
}