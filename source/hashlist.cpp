#include <stdio.h>
#include <tchar.h>
#include "retool.h"
#include "extract.h"
#include "replace.h"
#include "misc.h"
#include "pakFormat.h"
#include "PAKFunctions.h"

bool ReadHashList(char *filenameHashList)
{
	//TODO: When reading entries ending with "bnk.2" we should automatically add variants named the following:
	//bnk.2.x64.De
	//bnk.2.x64.En
	//bnk.2.x64.Es
	//bnk.2.x64.Fr
	//bnk.2.x64.It
	//bnk.2.x64.Ja
	//bnk.2.x64.PtBR
	//bnk.2.x64.Ru
	//bnk.2.x64.ZhCN
	//bnk.2.x64

	char *data = 0;
	unsigned int dataSize = 0;

	FILE *file;
	fopen_s(&file, filenameHashList, "rb");
	char line[1000];
	if(file)
	{
		//Determine how many lines there are
		bool finalLine = 0;
		int lineCount = 0;
		while(!finalLine)
		{
			finalLine = (fgets(line, 1000, file) == 0);
			lineCount++;
		}

		//Create buffer for hash list
		hashListSize = lineCount;
		hashList = new hashList_s[hashListSize];

		//Process lines
		fseek(file, 0, SEEK_SET);
		finalLine = 0;
		char line2[1000];
		unsigned int cur = 0, skippedDuplicates = 0;
		while(!finalLine)
		{
			line[0] = 0;
			line2[0] = 0;
			finalLine = (fgets(line, 1000, file) == 0);
			RemoveLineBreaks(line);
			RemoveSpacesAtStart(line);
			RemoveSpacesAtEnd(line);
			if(line[0] == 0) //Line is empty
				continue;

			//Defaults
			hashList[cur].path[0] = 0;
			hashList[cur].hash_lowercase = 0;
			hashList[cur].hash_highercase = 0;

			//Check if the first "token" in the string is a hash (if so, we skip the hash)
			char *filePath = 0;
			bool hashFound = 0;
			if(SymbolCountInString(line, ' ') > 0) //Check if there's at least one space character
			{
				char lineCopied[1000];
				strcpy(lineCopied, line);
				SeparateString(lineCopied, line2, ' '); //Separate the line into two lines based on first occurance of a space character
				if(IsNumber(lineCopied)) //If this is a number, then let's assume it's a hash and therefore skip that part of the line
				{
					//TODO: If a line starts with a directory/file name with a space in it which is all supposed to be digits, then this leads to a bug. But I'm not sure how to handle it and that would be extremely obscure anyway
					//TODO: Also, if the line has TWO hashes, that's also a scenario where this fails. But I don't know of any tool which outputs RE Engine file lists like that
					filePath = line2;
					hashFound = 1;
				}
			}
			
			if(hashFound == 0) //We didn't detect any hash, so assume the entire line represents a file path
				filePath = line;

			if(filePath == 0) //We failed to read in a proper path, so let's skip this line
				continue;

			if(IsNumber(filePath)) //If filepath is only numbers, then something probably went wrong (maybe we somehow read in a hash and no filepath)
				continue;
			if(filePath[0] != 'n' && filePath[0] != 'N') //Skip lines that don't start with starting letter in 'natives'
				continue;

			hashList[cur].hash_lowercase = ConvertFilePathToMurmurHash(filePath, 1); //Convert path into hash
			hashList[cur].hash_highercase = ConvertFilePathToMurmurHash(filePath, 0); //Same as above, but for higher case

			//Check if we've already added this hash, in which case we'll skip this line
			bool skip = 0;
			for (unsigned int i = 0; i < cur; i++)
			{
				if (hashList[cur].hash_lowercase == hashList[i].hash_lowercase && hashList[cur].hash_highercase == hashList[i].hash_highercase)
				{
					skip = 1;
					break;
				}
			}
			if (skip)
			{
				skippedDuplicates++;
				continue;
			}

			strcpy(hashList[cur].path, filePath); //Copy over string
			if (printHashListReadingUpdates)
				printf("Added %u / %u\n", hashList[cur].hash_lowercase, hashList[cur].hash_highercase);
			cur++;
		}

		fclose(file);
		hashListSize = cur;
		if (skippedDuplicates)
			printf("Read %i entries from %s (skipped %i duplicates)\n", hashListSize, filenameHashList, skippedDuplicates);
		else
			printf("Read %i entries from %s\n", hashListSize, filenameHashList);
		return 1;
	}
	else
	{
		printf("Failed to read %s\n", filenameHashList);
		return 0;
	}
}

bool OutputHashList(char *filenameHashList)
{
	char newPath[260];
	sprintf(newPath, "%s.new", filenameHashList);
	FILE *file;
	fopen_s(&file, newPath, "wb");
	if (!file)
	{
		printf("Failed to open %s for writing\n", newPath);
		return 0;
	}
	for (unsigned int i = 0; i < hashListSize; i++)
		//fprintf(file, "%I64u %s\r\n", hashList[i].hash, hashList[i].path);
		fprintf(file, "%s\r\n", hashList[i].path);
	fclose(file);
	printf("Wrote %s with %u entries\n", newPath, hashListSize);
	return 1;
}

bool CombineHashLists(char *filenameHashList, hashList_s *altHashList, unsigned int altHashListSize)
{
	char newPath[260];
	sprintf(newPath, "%s.new", filenameHashList);
	FILE *file;
	fopen_s(&file, newPath, "wb");
	if (!file)
	{
		printf("Failed to open %s for writing\n", newPath);
		return 0;
	}
	
	//Add from first hash list
	for (unsigned int i = 0; i < hashListSize; i++)
		//fprintf(file, "%I64u %s\r\n", hashList[i].hash, hashList[i].path);
		fprintf(file, "%s\r\n", hashList[i].path);

	//Add from second hash list (skip duplicates)
	unsigned int skippedDuplicates = 0;
	unsigned int totalPaths = hashListSize;
	for (unsigned int i = 0; i < altHashListSize; i++)
	{
		bool skip = 0;
		for (unsigned int j = 0; j < hashListSize; j++)
		{
			if (hashList[j].hash_lowercase == altHashList[i].hash_lowercase && hashList[j].hash_highercase == altHashList[i].hash_highercase)
			{ 
				skippedDuplicates++;
				skip = 1;
				break;
			}
		}
		if (skip)
			continue;
		//fprintf(file, "%I64u %s\r\n", altHashList[i].hash, altHashList[i].path);
		fprintf(file, "%s\r\n", altHashList[i].path);
		totalPaths++;
	}
	fclose(file);
	printf("Wrote %s with %u entries\nAdded %u new lines (skipped %u duplicates)\n", newPath, totalPaths, hashListSize - skippedDuplicates, skippedDuplicates);
	return 1;
}

char *ReturnFilenameFromHashList(unsigned long long hash_lowercase, unsigned long long hash_highercase)
{
	if(hashList == 0)
		return 0;
	for(unsigned int i = 0; i < hashListSize; i++)
	{
		if(hashList[i].hash_lowercase == hash_lowercase && hashList[i].hash_highercase == hash_highercase && hashList[i].path[0])
			return hashList[i].path;
	}
	return 0;
}