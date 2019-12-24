// lpsdecoder.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "LPS.h"
int main()
{
    std::cout << "Hello World!\n";

    LPS* lps = new LPS();
    lps->Open("global.lps");

    std::cout << "Done!\n";
    lps->Close();
}
