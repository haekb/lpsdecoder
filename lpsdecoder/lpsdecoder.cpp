// lpsdecoder.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include "LPS.h"

// See: https://github.com/jarro2783/cxxopts
#include "cxxopts.hpp"

int main(int argc, char* argv[], char* envp[])
{
    
    bool showHelp = false;
    bool extractOnly = false;
    bool cleanUpSS2 = true;
    std::string filename = "";

    char buffer[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, buffer);

#ifdef _DEBUG
    std::string pathToVGMStream = "\\..\\Tools\\VGMStream\\test.exe";
#else
    std::string pathToVGMStream = "\\VGMStream\\test.exe";
#endif
    pathToVGMStream = buffer + pathToVGMStream;

    cxxopts::Options options("Lithtech LPS Decoder and Extractor", "Decodes and Extracts a PS2 Lithtech LPS audio stream file.");
    options.add_options()
        ("h,help", "Help", cxxopts::value(showHelp))
        ("f,file", "LPS Filename", cxxopts::value(filename))
        ("e,extract-only", "Extract Only (Don't convert to wav)", cxxopts::value(extractOnly))
        ("c,clean-up-ss2", "Clean up SS2 files after converting to wav", cxxopts::value<bool>(cleanUpSS2)->default_value("true"))
        ("v,path-to-vgm-stream", "Path to VGMStream", cxxopts::value(pathToVGMStream))
        ;

    auto result = options.parse(argc, argv);

    if (showHelp) {
        std::cout << options.help() << "\n";

        return 0;
    }

    if (filename.empty()) {
        std::cout << "Must enter a filename with -f or --file <path>";
        return 1;
    }

    LPS* lps = new LPS();
    if (!lps->Open(filename)) {
        std::cout << "Failed to open " << filename << "\n";
        return 1;
    }
    std::cout << "Extracting!\n";
    lps->Extract(pathToVGMStream, extractOnly, cleanUpSS2);
    std::cout << "Done!\n";
    lps->Close();
}
