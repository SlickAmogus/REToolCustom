#include <stdio.h>
#include <direct.h>
#include "pakformat.h"
#include "libdeflate\libdeflate.h"
#include "retool.h"
#include "dir.h"
#include "misc.h"
#include <string.h>
#include "PAKFunctions.h"

void InvalidateFilesInPAK(char *dirName, char *pakFilename)
{
	if (alternateInvalidationMethod) //With this method, we need both dirName and pakFilename
	{
		if (dirName == 0 || pakFilename == 0)
		{
			printf("Error: PAK filename or directory filename missing\n");
			return;
		}
	}

	char GeneratedPakFilename[MAXPATH];
	if (pakFilename == 0) //"Guess" PAK file name if it wasn't supplied
	{
		sprintf(GeneratedPakFilename, "%s.pak", dirName);
		pakFilename = GeneratedPakFilename;
	}
	printf("Attempting to invalidate files in %s...\n", pakFilename);

	//Load PAKFile
	pakHeader_s pakHeader;
	pakEntry_s *pakEntries;
	unsigned long long pakSize;
	if(!LoadPAKFile(pakFilename, &pakHeader, &pakEntries, &pakSize))
		return;
	FILE *pakFile;
	fopen_s(&pakFile, pakFilename, "r+b");
	if(!pakFile)
	{
		delete[]pakEntries;
		return;
	}

	if(pakHeader.flags & PAKFLAGS_ENCRYPTED)
	{
		printf("Error: Invalidating paths is not supported in an encrypted PAK: %s\n", pakFilename);
		delete[]pakEntries;
		fclose(pakFile);
		return;
	}

	//Read files from directory path, and start processing files
	CreateFileQueue(dirName);
	unsigned int filesInvalidated = 0;
	for(int i = 0; i < dir_alphaqueNum; i++)
	{
		dir_queue_t *dir = &dir_que[dir_alphaque[i]];
		if(!dir->active)
			continue;

		//Convert full file path to murmur hash
		unsigned int murmurHash_lowercase = 0;
		unsigned int murmurHash_uppercase = 0;
		if (alternateInvalidationMethod) //With this method, we add "natives" to the path (as this method is assumed to be used in a way where "natives" is part of the dirName path
		{
			char newPath[MAXPATH];
			sprintf_s(newPath, MAXPATH, "natives\\%s", dir->fileName);
			murmurHash_lowercase = ConvertFilePathToMurmurHash(newPath, 1);
			murmurHash_uppercase = ConvertFilePathToMurmurHash(newPath, 0);
		}
		else
		{
			murmurHash_lowercase = ConvertFilePathToMurmurHash(dir->fileName, 1);
			murmurHash_uppercase = ConvertFilePathToMurmurHash(dir->fileName, 0);
		}

		//Scan through entry list for this file
		for(unsigned int j = 0; j < pakHeader.entryCount; j++)
		{
			if(pakEntries[j].filenameHashL == murmurHash_lowercase && pakEntries[j].filenameHashU == murmurHash_uppercase) //Found match
			{
				pakEntries[j].filenameHashL = pakEntries[j].filenameHashU = 0;
				filesInvalidated++;
				break;
			}
		}
	}

	//Finish
	if(filesInvalidated == 0)
		printf("Error: Invalidated 0 files in %s\n", pakFilename);
	else
	{
		//Write new entry list
		_fseeki64(pakFile, sizeof(pakHeader_s), SEEK_SET);
		
		if(pakHeader.flags & PAKFLAGS_ENCRYPTED)
		{
			//TODO
		}

		fwrite(pakEntries, sizeof(pakEntry_s) * pakHeader.entryCount, 1, pakFile);
		printf("Invalidated %i files in %s.\n", filesInvalidated, pakFilename);
	}
	fclose(pakFile);
	delete[]pakEntries;
}

void ReplaceFilesInPAK(char *dirname)
{
	char pakFilename[MAXPATH];
	sprintf(pakFilename, "%s.pak", dirname);
	printf("Attempting to replace files in %s...\n", pakFilename);

	//Load PAKFile
	pakHeader_s pakHeader;
	pakEntry_s *pakEntries;
	unsigned long long pakSize;
	if(!LoadPAKFile(pakFilename, &pakHeader, &pakEntries, &pakSize))
		return;
	FILE *pakFile;
	fopen_s(&pakFile, pakFilename, "r+b");
	if(!pakFile)
	{
		delete[]pakEntries;
		return;
	}

	if(pakHeader.flags & PAKFLAGS_ENCRYPTED)
	{
		printf("Error: Invalidating paths is not supported in an encrypted PAK: %s\n", pakFilename);
		delete[]pakEntries;
		fclose(pakFile);
		return;
	}

	//Prepare compressor
	libdeflate_compressor *compressor = libdeflate_alloc_compressor(6);

	//Read files from directory path, and start processing files
	CreateFileQueue(dirname);
	unsigned int filesReplaced = 0;
	for(int i = 0; i < dir_alphaqueNum; i++)
	{
		dir_queue_t *dir = &dir_que[dir_alphaque[i]];
		if(!dir->active)
			continue;

		//Convert full file path to murmur hash
		unsigned int murmurHash_lowercase = ConvertFilePathToMurmurHash(dir->fileName, 1);
		unsigned int murmurHash_uppercase = ConvertFilePathToMurmurHash(dir->fileName, 0);

		//Scan through entry list for this file
		for(unsigned int j = 0; j < pakHeader.entryCount; j++)
		{
			if (pakEntries[j].filenameHashL == murmurHash_lowercase && pakEntries[j].filenameHashU == murmurHash_uppercase) //Found match
			{
				//Compress file data
				unsigned char *data;
				unsigned int dataSize;

				{
					char loadPath[MAXPATH];
					sprintf_s(loadPath, MAXPATH, "%s/%s", dirname, dir->fileName);
					if (!ReadFile(loadPath, &data, &dataSize))
					{
						printf("Error: Failed to open %s for reading\n", loadPath);
						break;
					}
				}

				unsigned char *compressedData = new unsigned char[dataSize];
				unsigned int compressedSize = libdeflate_deflate_compress(compressor, data, dataSize, compressedData, dataSize);
				unsigned char *copyData;
				unsigned int copySize;
				if(compressedSize != 0 && compressedSize < dataSize)
				{
					copyData = compressedData;
					copySize = compressedSize;
					pakEntries[j].flag = pakEntries[j].flag & ~2;
					pakEntries[j].flag |= 1;
				}
				else //compressedSize is larger than the real data size, or compressedSize is 0, which means compression failed or the compressed data didn't fit into the allocated space
				{
					copyData = data;
					copySize = dataSize;
					pakEntries[j].flag = pakEntries[j].flag & ~2;
					pakEntries[j].flag = pakEntries[j].flag & ~1;
				}

				if(copySize > pakEntries[j].compressedSize) //Write data at end of file if new file data is larger than old. If it's smaller, we can save it at the original offset
				{
					pakEntries[j].offset = pakSize;
					pakSize += copySize;
				}

				//Write data
				_fseeki64(pakFile, pakEntries[j].offset, SEEK_SET);
				fwrite(copyData, copySize, 1, pakFile);
				pakEntries[j].compressedSize = copySize;
				pakEntries[j].realSize = dataSize;

				//Finish
				if(pakEntries[j].flag & 1)
					printf("Replaced data for %s (compressed)\n", dir->fileNameWithoutDir);
				else
					printf("Replaced data for %s (copied)\n", dir->fileNameWithoutDir);
				filesReplaced++;
				delete[]data;
				delete[]compressedData;
				break;
			}
		}
	}

	//Finish
	if(filesReplaced == 0)
		printf("Error: Replaced 0 files in PAK container files.\n");
	else
	{
		//Write new entry list
		_fseeki64(pakFile, sizeof(pakHeader_s), SEEK_SET);

		if(pakHeader.flags & PAKFLAGS_ENCRYPTED)
		{
			//TODO
		}

		fwrite(pakEntries, sizeof(pakEntry_s) * pakHeader.entryCount, 1, pakFile);
		printf("Replaced %i files in %s.\n", filesReplaced, pakFilename);
	}
	libdeflate_free_compressor(compressor);
	fclose(pakFile);
	delete[]pakEntries;
}