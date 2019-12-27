#include "LPS.h"
#include <iostream>
#include <Windows.h>

bool DirExist(char* strPath);
bool CreateDir(char* strPath);

LPS::LPS()
{
	m_Closed = false;
	m_SkipIndex = 0;
	m_BaseOffset = 0;
	m_Header = {};
}

LPS::~LPS()
{
	Close();
}

bool LPS::Open(std::string filename)
{
	m_Stream.open(filename, std::ios_base::binary);

	if (!m_Stream.is_open()) {
		return false;
	}

	// Read in the data
	m_Stream.read((char*)&m_Header, sizeof(m_Header));

	/*
	    savepos TMP
    xmath BASE_OFF "TMP + (FILES * 0x48) + (FOLDERS * 0x30)"
	*/
	uint currentPos = m_Stream.tellg();
	m_BaseOffset = currentPos + (m_Header.files * 0x48) + (m_Header.folders * 0x30);

	for (uint i = 0; i < m_Header.files; i++)
	{
		File file;
		m_Stream.read((char*)&file, sizeof(file));
		m_Files.push_back(file);
	}
	for (uint i = 0; i < m_Header.folders; i++)
	{
		Folder folder;
		m_Stream.read((char*)&folder, sizeof(folder));
		m_Folders.push_back(folder);
	}

	return true;
}

bool LPS::Close()
{
	if (m_Closed) {
		return true;
	}

	m_Stream.close();
	m_Closed = true;
	return true;
}

// There's probably a neater way to do this, but eh well.
uint LPS::TraverseFolder(int index, BuiltFolder* builtFolder)
{
	uint foldersTraversed = 0;
	int indexInner = 0;

	// Convenient!
	auto folder = builtFolder->folder;

	// Okay...let's traverse through some more folders and count up their files.
	// This is probably better suited for an iterator loop, then we wouldn't need the skip section. But oh well!
	for (auto &folderInner : m_BuiltFolders)
	{

		// We're done folks!
		if (foldersTraversed >= folder->folders) {
			break;
		}

		// Skip if we need to catch up
		if (indexInner < m_SkipIndex) {
			indexInner++;
			continue;
		}

		// Ignore anything above us!
		if (folderInner.folder->priority > folder->priority)
		{
			indexInner++;
			continue;
		}

		// No infinite loops!
		if (indexInner == index) {
			indexInner++;
			continue;
		}

		// Ignore anything before our folder, we need stuff ahead of it!
		if (folderInner.folder->fileStartIndex < folder->fileStartIndex)
		{
			break;
			indexInner++;
			continue;
		}

		// Bolt on the path
		std::string oldPath = folderInner.path;
		// Ignore root, as it doesn't actually contain a path...
		if (!builtFolder->path.empty()) {
			folderInner.path = builtFolder->path;
			folderInner.path += "\\";
			folderInner.path += oldPath;
		}
		// Oh hey we have a folder!
		if (folderInner.folder->folders > 0) {
			// Recursively unwrap this folder
			auto upToIndex = TraverseFolder(indexInner, &folderInner);

			if (upToIndex > m_SkipIndex) {
				m_SkipIndex = upToIndex;
			}
		}

		// Increment our cool counters
		indexInner++;
		foldersTraversed++;
	}

	return indexInner;
}


void LPS::Extract(std::string pathToVGMStream, bool extractOnly, bool cleanUp)
{
	int index = 0;
	std::vector<BuiltSound> builtSounds;
	m_SortedFolders = m_Folders;
	m_SkipIndex = 0;

	// Sort it!
	std::stable_sort(m_SortedFolders.begin(), m_SortedFolders.end(), by_priority_then_folder_then_startIndex());
	
	// Build out the initial "Built" list.
	for (Folder folder : m_SortedFolders)
	{
		// Add a builtFolder with temp path for now!
		BuiltFolder builtFolder;
		builtFolder.folder = &m_SortedFolders[index];
		builtFolder.path = folder.foldername;
		m_BuiltFolders.push_back(builtFolder);
		index++;
	}

	// Load in our file list with immediate folders
	index = 0;
	for (Folder folder : m_SortedFolders)
	{

		if (folder.files == 0) {
			index++;
			continue;
		}

		for (int i = folder.fileStartIndex; i < folder.files + folder.fileStartIndex; i++)
		{
			BuiltSound sound;

			sound.file = &m_Files[i];
			sound.folder = &m_BuiltFolders[index];
			builtSounds.push_back(sound);

		}
		index++;
	}

	// Traverse the root, and get this nightmare rolling
	auto folder = m_BuiltFolders[0];
	TraverseFolder(0, &folder);
	
	// Cool, we can extract now!
	for (auto sound : builtSounds)
	{
		// Build the extraction path
		std::string outputFilename = "out\\";
		std::string extension = ".ss2";
		std::string originalOutputFilename = "";

		outputFilename += sound.folder->path + "\\";

		// Create the directory if needed
		auto result = CreateDir((char*)outputFilename.c_str());

		outputFilename += sound.file->filename;

		// Save the original output
		originalOutputFilename = outputFilename.c_str();

		// Replace extension
		outputFilename.replace(outputFilename.end() - 4, outputFilename.end(), extension);

		std::ofstream fileOut((char*)outputFilename.c_str(), std::ios_base::binary);

		uint currentPos = 0;
		uint togo = sound.file->size;
		uint inc = 2048;
		auto seek = sound.file->offset + m_BaseOffset;


		if (!fileOut.is_open()) {
			continue;
		}

		m_Stream.seekg(seek);

		/* SS2 header via the BMS script
		put 0x64685353 long MEMORY_FILE
		put 0x18 long MEMORY_FILE
		put 0x10 long MEMORY_FILE
		put FREQUENCY long MEMORY_FILE
		put CHANNELS long MEMORY_FILE
		put INTERLEAVE long MEMORY_FILE
		put 0 long MEMORY_FILE
		put 0xffffffff long MEMORY_FILE
		put 0x64625353 long MEMORY_FILE
		put SIZE long MEMORY_FILE
		*/
		SS2Header header = {
			0x64685353,
			0x18,
			0x10,
			sound.file->frequency,
			1, // Mono!
			0, // No interleave?
			0,
			0xffffffff,
			0x64625353,
			sound.file->size
		};

		// Write out the header
		// There's probably a better way to do this.
		fileOut.write(reinterpret_cast<const char*>(&header.type), sizeof(header.type));
		fileOut.write(reinterpret_cast<const char*>(&header.one), sizeof(header.one));
		fileOut.write(reinterpret_cast<const char*>(&header.two), sizeof(header.two));
		fileOut.write(reinterpret_cast<const char*>(&header.frequency), sizeof(header.frequency));
		fileOut.write(reinterpret_cast<const char*>(&header.channels), sizeof(header.channels));
		fileOut.write(reinterpret_cast<const char*>(&header.interleave), sizeof(header.interleave));
		fileOut.write(reinterpret_cast<const char*>(&header.zero), sizeof(header.zero));
		fileOut.write(reinterpret_cast<const char*>(&header.max), sizeof(header.max));
		fileOut.write(reinterpret_cast<const char*>(&header.three), sizeof(header.three));
		fileOut.write(reinterpret_cast<const char*>(&header.size), sizeof(header.size));

#if 0
		char* buffer = new char[togo];
		m_Stream.read(buffer, togo);
		fileOut.write(buffer, togo);
#else
		// Extract 2048 by 2048
		while (togo > 0) {
			if (togo < inc) {
				inc = togo;
			}

			char* buffer = new char[inc];

			m_Stream.read(buffer, inc);
			currentPos += m_Stream.gcount();

			fileOut.write(buffer, inc);


			togo -= inc;
		}
#endif

		std::cout << "Extracted " << outputFilename << "\n";

		fileOut.close();

		// Sleep, otherwise Windows will choke on CreateProcess
		Sleep(100);

		// Only extracting, let's skip the conversion.
		if (extractOnly) {
			continue;
		}

		// Conversion variables - Big ol' Windows mess
		char buffer[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, buffer);
		std::string path = buffer;
		path += "\\";

		// Loop once, no fade out!
		std::string commandArgs = " -o \"" + path + originalOutputFilename + "\" -l 1.0 -f 0.0 \"" + path + outputFilename + "\"";
		size_t exePosition = pathToVGMStream.find_last_of("\\");

		// Trim off the starting slashes, and grab the exe name!
		std::string exe = pathToVGMStream.substr(exePosition+1, pathToVGMStream.length());
		commandArgs = exe + commandArgs;

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		char* lpApplicationName = (char*)pathToVGMStream.c_str();
		char* lpCommandArgs = (char*)commandArgs.c_str();

		// set the size of the structures
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		// start the program up
		auto appResult = CreateProcess(lpApplicationName,   // the path
			lpCommandArgs,        // Command line
			NULL,           // Process handle not inheritable
			NULL,           // Thread handle not inheritable
			FALSE,          // Set handle inheritance to FALSE
			0,              // No creation flags
			NULL,           // Use parent's environment block
			NULL,           // Use parent's starting directory 
			&si,            // Pointer to STARTUPINFO structure
			&pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
		);

		if (result) {
			//std::cout << "Converted " << outputFilename << " to wav\n";
		}

		// Close process and thread handles. 
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		// Clean up if they requested it.
		if (cleanUp) {
			if (remove((char*)outputFilename.c_str()) != 0) {
				std::cout << "Failed to clean up " << outputFilename << "\n";
				continue;
			}

			std::cout << "Cleaned up " << outputFilename << "\n";
		}
		
	}
}



//
// From WinUtil of NOLF SDK
// 
bool DirExist(char* strPath)
{
	if (!strPath || !*strPath) return false;

	bool bDirExists = false;

	bool bRemovedBackSlash = false;
	if (strPath[strlen(strPath) - 1] == '\\')
	{
		strPath[strlen(strPath) - 1] = '\0';
		bRemovedBackSlash = true;
	}

	UINT oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
	struct stat statbuf;
	int error = stat(strPath, &statbuf);
	SetErrorMode(oldErrorMode);
	if (error != -1) bDirExists = true;

	if (bRemovedBackSlash)
	{
		strPath[strlen(strPath)] = '\\';
	}

	return bDirExists;
}

//
// From WinUtil of NOLF SDK
// 
bool CreateDir(char* strPath)
{
	if (DirExist(strPath)) return true;
	if (strPath[strlen(strPath) - 1] == ':') return false;		// special case

	char strPartialPath[MAX_PATH];
	strPartialPath[0] = '\0';

	char* token = strtok(strPath, "\\");
	while (token)
	{
		strcat(strPartialPath, token);
		if (!DirExist(strPartialPath) && strPartialPath[strlen(strPartialPath) - 1] != ':')
		{
			if (!CreateDirectoryA(strPartialPath, NULL)) return false;
		}
		strcat(strPartialPath, "\\");
		token = strtok(NULL, "\\");
	}

	return true;
}
