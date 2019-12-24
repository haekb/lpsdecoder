#include "LPS.h"
#include <iostream>


LPS::LPS()
{
	m_Closed = false;
	m_SkipIndex = 0;
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
		folderInner.path = builtFolder->path;
		folderInner.path += "/";
		folderInner.path += oldPath;

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


void LPS::Extract()
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
		std::cout << sound.folder->path << "/" << sound.file->filename << "\n";
	}
}
