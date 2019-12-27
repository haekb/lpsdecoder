// lpsdecoder.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "LPS.h"
#include "cxxopts.hpp"

int main(int argc, char* argv[], char* envp[])
{
    bool showHelp = false;
    std::string filename = "";

    cxxopts::Options options("Lithtech LPS Decoder and Extractor", "Decodes and Extracts a PS2 Lithtech LPS audio stream file.");
    options.add_options()
        ("h,help", "Help", cxxopts::value(showHelp))
        ("f,file", "LPS Filename", cxxopts::value(filename))
        ;

    auto result = options.parse(argc, argv);

    if (filename.empty()) {
        std::cout << "Must enter a filename with -f or --file <path>";
        return 1;
    }

    LPS* lps = new LPS();
    if (!lps->Open("global.lps")) {
        std::cout << "Failed to open " << filename << "\n";
        return 1;
    }
    std::cout << "Extracting!\n";
    lps->Extract();
    std::cout << "Done!\n";
    lps->Close();
}
