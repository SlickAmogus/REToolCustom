#include <stdio.h>
#include <windows.h>
#include "dir.h"
#include "misc.h"

static WIN32_FIND_DATA findFileData;
dir_queue_t *dir_que = 0;
unsigned int dir_filesTotal; //This and dirsTotal are for reference only.
unsigned int dir_dirsTotal;
int *dir_alphaque;
int dir_alphaqueNum; //This is what external functions should use for iterating through final list
static bool addDirs;
static bool addFiles;
static bool searchSubDirs;
static unsigned int resetTo = DIRQUE_STARTINGSIZE;
static unsigned int highestDirNum = 0;
static unsigned int dirQueueCurrentSize = DIRQUE_STARTINGSIZE;
static unsigned int dirQueCur = 0;

bool Dir_CompareFilenames(char *file1, char *file2) //Compare filenames ignoring case. Treating \ and / as the same
{
	int i = 0;
	char a, b;
	while (1)
	{
		if (file1[i] == 0 || file2[i] == 0)
		{
			if (file1[i] == 0 && file2[i] == 0)
				break;
			else
				return 0;
		}

		a = tolower(file1[i]);
		b = tolower(file2[i]);
		if (a == b
			|| ((a == '\\' || a == '/') && (b == '\\' || b == '/'))
			)
		{
			i++;
			continue;
		}
		return 0;
	}
	if (i == 0)
		return 0;
	return 1;
}

static void Dir_IncreaseQueSize()
{
	dirQueueCurrentSize += DIRQUE_INCREASESIZEBY;
	dir_que = (dir_queue_t *)realloc(dir_que, sizeof(dir_queue_t) * dirQueueCurrentSize);
	dir_alphaque = (int *)realloc(dir_alphaque, sizeof(int) * dirQueueCurrentSize);
	for (unsigned int i = dirQueueCurrentSize - DIRQUE_INCREASESIZEBY; i < dirQueueCurrentSize; i++)
		dir_que[i].active = 0;
}

void Dir_Init()
{
	dirQueueCurrentSize = DIRQUE_STARTINGSIZE;
	dir_que = (dir_queue_t *)malloc(sizeof(dir_queue_t) * dirQueueCurrentSize);
	dir_alphaque = (int *)malloc(sizeof(int) * dirQueueCurrentSize);
}

void Dir_Deinit()
{
	free(dir_que);
	free(dir_alphaque);
}

void Dir_ResetQueue()
{
	dir_filesTotal = 0;
	dir_dirsTotal = 0;
	dir_alphaqueNum = 0;
	for (unsigned int i = 0; i < resetTo; i++)
		dir_que[i].active = 0;
	resetTo = 0;
}

bool Dir_DirExists(char *str)
{
	for (unsigned int i = 0; i < highestDirNum; i++)
	{
		if (dir_que[i].active && dir_que[i].dir && Dir_CompareFilenames(str, dir_que[i].fileName))
			return 1;
	}
	return 0;
}

bool Dir_FileExists(char *str)
{
	for (unsigned int i = 0; i < highestDirNum; i++)
	{
		if (dir_que[i].active && !dir_que[i].dir && Dir_CompareFilenames(str, dir_que[i].fileName))
			return 1;
	}
	return 0;
}

int Dir_FindActiveDirQueue()
{
	for (unsigned int i = 0; i < highestDirNum; i++)
		if (dir_que[i].active && dir_que[i].dirSearch)
			return i;
	return -1;
}

int Dir_FindInactiveQueue() //TODO: This shouldn't be needed. Instead, we should increase a counter by one for each time we add a new file/dir
{
	dirQueCur++;
	if (dirQueCur >= dirQueueCurrentSize)
		Dir_IncreaseQueSize();
	if (highestDirNum < dirQueCur + 1)
		highestDirNum = dirQueCur + 1;
	return dirQueCur;
}

void Dir_AddToQueue(char *startDir, char *searchPath, char *dirName, bool dir, unsigned long long lastModified, unsigned long long fileSize, char *fileNameMustContain)
{
	if ((dirName[0] == '.') || (dirName[0] == '.' && dirName[1] == '.'))
		return; //Skip "." and ".."

	int slot = Dir_FindInactiveQueue(), i = 0, j = 0;
	bool startCopying = 0;

	if (slot == -1)
		return;

	dir_que[slot].archive = -1;
	dir_que[slot].lastModified = lastModified;
	dir_que[slot].fileSize = fileSize;
	dir_que[slot].dir = dir;
	memset(dir_que[slot].fileName, 0, FILENAME_SIZE_BIG);

	if (dir && searchSubDirs) //Set this to true if we're searching through subdirectories
		dir_que[slot].dirSearch = 1;
	else
		dir_que[slot].dirSearch = 0;

	int posOfLastDot = 0;
	int posOfLastSlash = -1;

	//Copy search path to string
	int length = (int)strlen(startDir);
	char *copyStr = &searchPath[length + 1];
	while (copyStr[j] != 0)
	{
		if (copyStr[j] != '*')
		{
			dir_que[slot].fileName[i] = copyStr[j];
			if (dir_que[slot].fileName[i] == '\\' || dir_que[slot].fileName[i] == '/')
				posOfLastSlash = i;
			i++;
		}
		j++;
	}

	j = 0;

	//Copy dirName to string
	while (dirName[j] != 0)
	{
		dir_que[slot].fileName[i] = dirName[j];
		if (dir_que[slot].fileName[i] == '.')
			posOfLastDot = i;
		if (dir_que[slot].fileName[i] == '\\' || dir_que[slot].fileName[i] == '/')
			posOfLastSlash = i;
		i++;
		j++;
	}
	strcpy(dir_que[slot].fileNameWithoutExt, dir_que[slot].fileName);
	if (posOfLastDot != 0 && !dir)
	{
		dir_que[slot].extension = &dir_que[slot].fileName[posOfLastDot + 1];
		dir_que[slot].fileNameWithoutExt[posOfLastDot] = 0;
	}
	else
		dir_que[slot].extension = dir_que[slot].fileName;

	if (posOfLastSlash == -1)
	{
		dir_que[slot].fileNameWithoutDir = dir_que[slot].fileName;
		dir_que[slot].fileNameWithoutDirAndExt = dir_que[slot].fileNameWithoutExt;
	}
	else
	{
		dir_que[slot].fileNameWithoutDir = &dir_que[slot].fileName[posOfLastSlash + 1];
		dir_que[slot].fileNameWithoutDirAndExt = &dir_que[slot].fileNameWithoutExt[posOfLastSlash + 1];
	}

	if (dir == 0 && fileNameMustContain != 0 && ReturnStringPos(fileNameMustContain, dir_que[slot].fileNameWithoutDir) == 0)
		return; //Could not find part of filename we require for this to be added to dir queue

	if (dir_que[slot].fileName[0] != 0)
	{
		dir_que[slot].active = 1;
		if ((addDirs && dir) || (addFiles && !dir))
		{
			if (dir)
				dir_dirsTotal++;
			else
				dir_filesTotal++;
			dir_alphaque[dir_alphaqueNum] = slot;
			dir_alphaqueNum++;
		}
	}
}

static void Scan(char *startDir, char *fileNameMustContain)
{
	//Initial search
	dir_que[0].active = 1;
	dir_que[0].dirSearch = 1;
	dir_que[0].dir = 1;
	dir_que[0].fileSize = 0;
	dir_que[0].fileName[0] = 0;
	dir_que[0].fileNameWithoutExt[0] = 0;
	dir_que[0].extension = 0;
	dir_que[0].fileNameWithoutDir = 0;
	dir_que[0].fileNameWithoutDirAndExt = 0;
	dir_que[0].archive = -1;

	//Remove final slash if it exists in startDir
	int i = 0;
	while (startDir[i] != 0)
		i++;
	if (i != 0 && (startDir[i - 1] == '\\' || startDir[i - 1] == '/'))
		startDir[i - 1] = 0;

	char searchPath[DIR_MAXSTRING];
	while (1) //Cycle through dir queue until it's empty
	{
		int q = Dir_FindActiveDirQueue();
		if (q == -1)
			break;
		if (dir_que[q].fileName[0] == 0)
			sprintf(searchPath, "%s\\*", startDir);
		else
			sprintf(searchPath, "%s\\%s\\*", startDir, dir_que[q].fileName);
		HANDLE hFind = FindFirstFile(searchPath, &findFileData);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			break;
			//TODO: Replace StatusUpdate with another similar command
			//StatusUpdate("FindFirstFile failed (%d)\n", GetLastError());
			return;
		}
		while (1)
		{
			unsigned long long lastModified = (((ULONGLONG)findFileData.ftLastWriteTime.dwHighDateTime) << 32) + findFileData.ftLastWriteTime.dwLowDateTime;

			if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) //This is a directory
				Dir_AddToQueue(startDir, searchPath, findFileData.cFileName, 1, lastModified, 0, fileNameMustContain);
			else //Should be a normal file
				Dir_AddToQueue(startDir, searchPath, findFileData.cFileName, 0, lastModified, ((unsigned long long) findFileData.nFileSizeHigh << 32) + findFileData.nFileSizeLow, fileNameMustContain);

			int success = FindNextFile(hFind, &findFileData);
			if (!success)
				break;
		}
		dir_que[q].dirSearch = 0;
		FindClose(hFind);
	}

	dir_que[0].active = 0; //Disable initial search entry

	//Deactivate any added files or dirs if we're not searching for both dirs and files (this is only necessary for when external functions iterate through entire list, and not only alphaque)
	for (unsigned int i = 0; i < highestDirNum; i++)
	{
		if (dir_que[i].active && ((!addDirs && dir_que[i].dir) || (!addFiles && !dir_que[i].dir)))
			dir_que[i].active = 0;
	}
}

bool Dir_GetSizeAndModifiedDate(char *filePath, unsigned long long *date, unsigned long long *size)
{
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile(filePath, &findFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return 0;
	*date = (((ULONGLONG)findFileData.ftLastWriteTime.dwHighDateTime) << 32) + findFileData.ftLastWriteTime.dwLowDateTime;
	if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) //This is a directory
		*size = 0;
	else //Should be a normal file
		*size = (((ULONGLONG)findFileData.nFileSizeHigh) << 32) + findFileData.nFileSizeLow;
	FindClose(hFind);
	return 1;
}

static void ProcessPointersInQueue(char *fileNameMustContain)
{
	for (unsigned int k = 0; k < highestDirNum; k++)
	{
		if (!dir_que[k].active)
			continue;

		int posOfLastDot = 0;
		int posOfLastSlash = -1;
		int i = 0;
		while (dir_que[k].fileName[i] != 0)
		{
			if (dir_que[k].fileName[i] == '.')
				posOfLastDot = i;
			if (dir_que[k].fileName[i] == '\\' || dir_que[k].fileName[i] == '/')
				posOfLastSlash = i;
			i++;
		}

		if (posOfLastDot != 0 && !dir_que[k].dir)
		{
			dir_que[k].extension = &dir_que[k].fileName[posOfLastDot + 1];
			dir_que[k].fileNameWithoutExt[posOfLastDot] = 0;
		}
		else
			dir_que[k].extension = dir_que[k].fileName;

		if (posOfLastSlash == -1)
		{
			dir_que[k].fileNameWithoutDir = dir_que[k].fileName;
			dir_que[k].fileNameWithoutDirAndExt = dir_que[k].fileNameWithoutExt;
		}
		else
		{
			dir_que[k].fileNameWithoutDir = &dir_que[k].fileName[posOfLastSlash + 1];
			dir_que[k].fileNameWithoutDirAndExt = &dir_que[k].fileNameWithoutExt[posOfLastSlash + 1];
		}

		if (dir_que[k].dir == 0 && fileNameMustContain != 0 && ReturnStringPos(fileNameMustContain, dir_que[k].fileNameWithoutDir) == 0) //Could not find part of filename we require for this to be added to dir queue, so let's skip it
			dir_que[k].active = 0;
	}
}

void CreateFileQueue(char *startDir, bool subdir, bool addDir, bool addFile, char *fileNameMustContain) //Start dir is where it searches. subdir is whether or not we search for subdirectories. addDirs is whether we added directories to the final list of found items. addFiles is the same but for files
{
	searchSubDirs = subdir;
	addDirs = addDir;
	addFiles = addFile;

	Dir_ResetQueue(); //Reset all queue info

	//Actual scan
	highestDirNum = 1;
	dirQueCur = 0;
	Scan(startDir, fileNameMustContain);

	//Manage char cointers within the queue, This has to be done after the scan as these pointers become invalid during any realloc()
	ProcessPointersInQueue(fileNameMustContain);

	//For faster reset next search
	resetTo = highestDirNum;
}