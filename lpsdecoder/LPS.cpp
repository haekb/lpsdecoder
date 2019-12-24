#include "LPS.h"
#include <iostream>


LPS::LPS()
{
}

LPS::~LPS()
{
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

	CalculatePath(0, m_Files.at(0));

	return true;
}

bool LPS::Close()
{
	m_Stream.close();
	return true;
}

void LPS::Extract()
{
}



// Bad!
int m_SkipIndex = 0;

uint LPS::FolderNightmare(int index, BuiltFolder* builtFolder)
{
#if 1
	uint fileEndIndex = 0;
	uint foldersTraversed = 0;
	int indexInner = 0;

	auto folder = builtFolder->folder;


	// Okay...let's traverse through some more folders and count up their files.
	for (auto &folderInner : m_BuiltFolders)
	{
		if (folderInner.path.compare("WOOD") == 0) {
			auto boo2 = true;
		}

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

		// Ok we can add the files safely!
		fileEndIndex += folderInner.folder->files;

		// or path???
		std::string oldPath = folderInner.path;
		folderInner.path = builtFolder->path;
		folderInner.path += "/";
		folderInner.path += oldPath;


		// Oh no...
		if (folderInner.folder->folders > 0) {
			// Recursively unwrap this folder
			auto upToIndex = FolderNightmare(indexInner, &folderInner);
			//auto skip = upToIndex - indexInner;
			if (upToIndex > m_SkipIndex) {
				m_SkipIndex = upToIndex;
				//m_SkipIndex--; // Account for some magic. (We're off by one, and the increment below.)
			}
		}


		//builtFolder->path += folderInner.folder->foldername;
		//builtFolder->path += "\\";

		// Increment our cool counters
		indexInner++;
		foldersTraversed++;
	}


	return indexInner;
#else
	uint fileEndIndex = 0;
	uint foldersTraversed = 0;
	int indexInner = 0;

	// Okay...let's traverse through some more folders and count up their files.
	for (Folder folderInner : m_Folders)
	{
		// We're done folks!
		if (foldersTraversed >= folder.folders) {
			break;
		}

		// No infinite loops!
		if (indexInner == index) {
			indexInner++;
			continue;
		}

		// Ignore anything before our folder, we need stuff ahead of it!
		if (folderInner.fileStartIndex < folder.fileStartIndex)
		{
			indexInner++;
			continue;
		}

		// Ok we can add the files safely!
		fileEndIndex += folderInner.files;

		// Oh no...
		if (folderInner.folders > 0) {
			// Recursively unwrap this folder
			fileEndIndex += FolderNightmare(indexInner, folderInner);
		}

		// Increment our cool counters
		indexInner++;
		foldersTraversed++;
	}

	return fileEndIndex;
#endif
}


std::string LPS::CalculatePath(uint fileIndex, File file)
{
#if 1
	m_SortedFolders = m_Folders;

	int index = 0;



	// Sort by priority
	//std::sort(m_SortedFolders.begin(), m_SortedFolders.end(), by_startIndex());
	//std::sort(m_SortedFolders.begin(), m_SortedFolders.end(), by_folder());
	//std::sort(m_SortedFolders.begin(), m_SortedFolders.end(), by_priority());

	std::stable_sort(m_SortedFolders.begin(), m_SortedFolders.end(), by_priority_then_folder_then_startIndex());
	
	std::vector<BuiltSound> builtSounds;

	for (Folder folder : m_SortedFolders)
	{
		// Add a builtFolder with temp path for now!
		BuiltFolder builtFolder;
		builtFolder.folder = &m_SortedFolders[index];
		builtFolder.path = folder.foldername;
		m_BuiltFolders.push_back(builtFolder);
		index++;
	}

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

	
	auto highest_priority = m_SortedFolders[0].priority;

	auto folder = m_BuiltFolders[0];
	uint fileEndIndex = folder.folder->fileStartIndex + folder.folder->files;

	fileEndIndex += FolderNightmare(0, &folder);
	




	for (auto sound : builtSounds)
	{


		std::cout << sound.folder->path << "/" << sound.file->filename << "\n";
	}




	/*

	for (Folder folder : m_FoldersLeft)
	{
		// Ok we only need to traverse the highest folders.
		// The nightmare function takes care of the rest!
		if (folder.priority != highest_priority) {
			break;
		}

		uint fileEndIndex = folder.fileStartIndex + folder.files;

		if (folder.folders > 0)
		{
			fileEndIndex += FolderNightmare(index, folder);
		}

		index++;
	}

	*/
	
	return "";
#else
	// Ok folders are annoying to figure out.
	// They have a "fileStartIndex" which determines the first file under that folder
	// However since they can be nested, I think the first dummy determines the folder (higher it is, the more root it is...?)

	// I'm so bad at this...
	int index = 0;

	std::vector<Folder> folderChoices;

	for (Folder folder : m_Folders)
	{
		uint fileEndIndex = folder.fileStartIndex + folder.files;
		

		if (folder.fileStartIndex <= fileIndex)
		{
			// Holy beans, this sucks. Gotta figure out the real upper range for this
			if (folder.folders > 0) {
				fileEndIndex += FolderNightmare(index, folder);
			}
		}

		// First we need to find folders that are within our range
		if (folder.fileStartIndex <= fileIndex && fileEndIndex > fileIndex) 
		{
			folderChoices.push_back(folder);
		}

		index++;
	}

	std::string path = "";

	std::sort(folderChoices.begin(), folderChoices.end(), by_startIndex());
	std::sort(folderChoices.begin(), folderChoices.end(), by_priority());

	for (Folder folder : folderChoices)
	{
		path += folder.foldername;
		path += "\\";
	}
	// should be chars//snd//male
	return path;
#endif
}
