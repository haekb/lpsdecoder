#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>

typedef unsigned int uint;

struct Header {
    //LithTech PSX Sound Data 1.20
    char id[28];
    uint dummy[2];
    uint folders;
    uint files;
    uint dummy2[2];
};

struct File {
    char filename[0x20];
    uint zero;
    uint offset;
    uint size;
    uint frequency;
    uint zero2;
    uint size2;
    uint dummy[4];
};

struct Folder {
    char foldername[32];
    uint folders;   // Number of folders in this folder
    uint files;     // Number of files in this folder (Note: Add this onto fileStartIndex to get the range of files.)
    uint priority;  // Higher number is the most root...yea i don't know.
    uint fileStartIndex;
};

struct BuiltFolder {
    std::string path;
    Folder* folder;
};

// Final struct we'll iterate through to extract and build paths
struct BuiltSound {
    File* file;
    BuiltFolder* folder;
    //Folder* folder;
};

struct by_startIndex {
    bool operator()(Folder const& a, Folder const& b) const {
        return a.fileStartIndex < b.fileStartIndex;
    }
};

struct by_folder {
    bool operator()(Folder const& a, Folder const& b) const {
        return a.folders > b.folders;
    }
};

struct by_priority {
    bool operator()(Folder const& a, Folder const& b) const {
        return a.priority > b.priority;
    }
};

struct by_priority_then_folder_then_startIndex {
    bool operator()(Folder const& a, Folder const& b) const {

        if (a.priority != b.priority) return a.priority > b.priority;
        
        if (a.folders != b.folders) return a.folders > b.folders;

        return a.fileStartIndex > b.fileStartIndex;
    }
};

class LPS
{
public:
	LPS();
	~LPS();

	bool Open(std::string filename);
	bool Close();

	void Extract();
protected:

    uint FolderNightmare(int index, BuiltFolder* builtFolder);
    std::string CalculatePath(uint fileIndex, File file);

    Header m_Header;
    std::vector<File> m_Files;
    std::vector<Folder> m_Folders;

    std::vector<Folder> m_SortedFolders;

    std::vector<BuiltFolder> m_BuiltFolders;

    std::ifstream m_Stream;
};

