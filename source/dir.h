#pragma once

#define DIRQUE_STARTINGSIZE 100
#define DIRQUE_INCREASESIZEBY 100
#define FILENAME_SIZE_BIG 260
#define DIR_MAXSTRING 300

typedef struct dir_queue_s
{
	unsigned long long lastModified; //Taken from a file's "FILETIME ftLastWriteTime"
	char fileName[FILENAME_SIZE_BIG]; //Filename, includes directory structure
	char fileNameWithoutExt[FILENAME_SIZE_BIG]; //Filename, includes directory structure but no extension
	char *extension; //Filename extension
	char *fileNameWithoutDir; //Filename without dir
	char *fileNameWithoutDirAndExt; //Filename without dir and ext
	bool active;
	unsigned int fileSize;
	bool dir; //Whether or not this is a dir
	bool dirSearch; //Whether or not this is a subdir we're gonna search through
	int archive; //If not -1, this signifies what specific package file is in
} dir_queue_t;

bool Dir_CompareFilenames(char *file1, char *file2);
void Dir_Init();
void Dir_Deinit();
void Dir_ResetQueue();
bool Dir_DirExists(char *str);
bool Dir_FileExists(char *str);
int Dir_FindActiveDirQueue();
int Dir_FindInactiveQueue();
void Dir_AddToQueue(char *startDir, char *searchPath, char *dirName, bool dir, unsigned long long lastModified = 0, unsigned long long fileSize = 0, char *fileNameMustContain = 0);
bool Dir_GetSizeAndModifiedDate(char *filePath, unsigned long long *date, unsigned long long *size);
void CreateFileQueue(char *startDir, bool subdir = 1, bool addDir = 0, bool addFile = 1, char *fileNameMustContain = 0);

extern dir_queue_t *dir_que;
extern unsigned int dir_filesTotal;
extern unsigned int dir_dirsTotal;
extern int *dir_alphaque;
extern int dir_alphaqueNum;