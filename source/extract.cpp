#include <stdio.h>
#include <tchar.h>
#include <direct.h>
#include <string.h>
#include "pakformat.h"
#include "libdeflate\libdeflate.h"
#include "retool.h"
#include "hashlist.h"
#include "PAKFunctions.h"
#include "zstd\zstd.h"
#include "misc.h"
#include "TEX\TEXtoDDS.h"

//These are the 128 crypto bytes in the PAK file after the TOC: C8399C72D1A39B08A0EB1867B9BF051344A230786A74DE6FB6F37B8B05621F1529BF4365A8A2D106ACCBC6F9FD89BCCE87E0CB2891B837A10805F463C17F13416DDB6B74F94326185ABB0FBBA95816E34C8CE7F477D327368116087E11FDB4F9096A314A30A1B16F4C327CA98ADF1CE88606EADC228BDB4E95127042952E3798

static void FilenameVariantWithoutPathAndExtension(char *to, char *from)
{
	int lastSlash = LastSlash(from), copyFrom = 0;

	if ((from[lastSlash] == '\\' || from[lastSlash] == '/') && from[lastSlash + 1] != 0)
		copyFrom = lastSlash + 1;
	else
	{
		lastSlash = LastColon(from);
		if (from[lastSlash] == ':' && from[lastSlash + 1] != 0)
			copyFrom = lastSlash + 1;
	}

	int lastDot = LastDot(from), copyTo = (int) strlen(from);

	if (from[lastDot] == '.' && lastDot > 0)
		copyTo = lastDot - 1;

	memcpy(to, &from[copyFrom], (copyTo - copyFrom) + 1);
	to[(copyTo - copyFrom) + 1] = 0;
}

void DataToHex(char *to, char *from, int size) 
{
	char const hextable[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	int i, j = 0;
	for(i = 0; i < size; i++)
	{
		to[j] = hextable[from[i] >> 4 & 0xf];
		to[j + 1] = hextable[from[i] & 0xf];
		j += 2;
	}
}

void MakeDirectory(char *fullpath)
{
	int pos = 0;
	char path[MAXPATH];
	memset(path, 0, MAXPATH);

	while(1)
	{
		if(fullpath[pos] == 0)
			return;
		path[pos] = fullpath[pos];
		pos++;
		if(fullpath[pos] == '\\' || fullpath[pos] == '/')
			_mkdir(path);
	}
}

// Auto-read __MANIFEST/MANIFEST.TXT embedded in PAK to resolve filenames
void LoadManifestFromPAK(char *pakFilename, pakHeader_s *pakHeader, pakEntry_s *pakEntries)
{
	// Known hashes for "__MANIFEST/MANIFEST.TXT"
	const unsigned int manifestHashL = 1578120220;
	const unsigned int manifestHashU = 538292905;

	// Find the manifest entry
	int manifestIdx = -1;
	for(unsigned int i = 0; i < pakHeader->entryCount; i++)
	{
		if(pakEntries[i].filenameHashL == manifestHashL && pakEntries[i].filenameHashU == manifestHashU)
		{
			manifestIdx = i;
			break;
		}
	}
	if(manifestIdx < 0)
		return;

	pakEntry_s *entry = &pakEntries[manifestIdx];

	// Open PAK and read compressed manifest data
	FILE *pakFile;
	fopen_s(&pakFile, pakFilename, "rb");
	if(!pakFile)
		return;

	unsigned char *compressedData = new unsigned char[(size_t)entry->compressedSize];
	_fseeki64(pakFile, entry->offset, SEEK_SET);
	fread(compressedData, 1, (size_t)entry->compressedSize, pakFile);
	fclose(pakFile);

	// Decompress
	unsigned char *manifestData = 0;
	unsigned long long manifestSize = 0;

	if(entry->flag & TOCFLAGS_DEFLATE)
	{
		manifestData = new unsigned char[(size_t)entry->realSize];
		libdeflate_decompressor *decompressor = libdeflate_alloc_decompressor();
		int result = libdeflate_deflate_decompress(decompressor, compressedData, (size_t)entry->compressedSize, manifestData, (size_t)entry->realSize, 0);
		libdeflate_free_decompressor(decompressor);
		if(result != 0)
		{
			printf("Warning: Failed to decompress manifest (deflate).\n");
			delete[]compressedData;
			delete[]manifestData;
			return;
		}
		manifestSize = entry->realSize;
		delete[]compressedData;
	}
	else if(entry->flag & TOCFLAGS_ZSTD)
	{
		manifestData = new unsigned char[(size_t)entry->realSize];
		size_t dSize = ZSTD_decompress(manifestData, (size_t)entry->realSize, compressedData, (size_t)entry->compressedSize);
		if(dSize != entry->realSize)
		{
			printf("Warning: Failed to decompress manifest (zstd).\n");
			delete[]compressedData;
			delete[]manifestData;
			return;
		}
		manifestSize = entry->realSize;
		delete[]compressedData;
	}
	else // Uncompressed
	{
		manifestData = compressedData;
		manifestSize = entry->compressedSize;
	}

	// Count lines in manifest
	unsigned int lineCount = 0;
	for(unsigned long long i = 0; i < manifestSize; i++)
	{
		if(manifestData[i] == '\n')
			lineCount++;
	}
	if(manifestSize > 0 && manifestData[manifestSize - 1] != '\n')
		lineCount++; // Last line without newline

	if(lineCount == 0)
	{
		delete[]manifestData;
		return;
	}

	// Build new hashList merging existing entries + manifest entries
	unsigned int newCapacity = hashListSize + lineCount;
	hashList_s *newHashList = new hashList_s[newCapacity];

	// Copy existing entries
	if(hashList && hashListSize > 0)
	{
		memcpy(newHashList, hashList, sizeof(hashList_s) * hashListSize);
		delete[]hashList;
	}

	// Parse manifest lines and add new entries
	unsigned int added = 0;
	unsigned long long lineStart = 0;
	for(unsigned long long i = 0; i <= manifestSize; i++)
	{
		bool endOfLine = (i == manifestSize) || (manifestData[i] == '\n') || (manifestData[i] == '\r');
		if(!endOfLine)
			continue;

		unsigned long long lineLen = i - lineStart;

		// Skip \r\n: if current is \r and next is \n, the \n iteration will be a zero-length line
		if(lineLen == 0)
		{
			lineStart = i + 1;
			continue;
		}

		// Copy line to null-terminated buffer
		if(lineLen >= MAXPATH)
			lineLen = MAXPATH - 1;
		char lineBuf[MAXPATH];
		memcpy(lineBuf, &manifestData[lineStart], (size_t)lineLen);
		lineBuf[lineLen] = 0;

		// Trim trailing \r if present
		if(lineLen > 0 && lineBuf[lineLen - 1] == '\r')
		{
			lineBuf[lineLen - 1] = 0;
			lineLen--;
		}

		lineStart = i + 1;

		if(lineBuf[0] == 0)
			continue;

		// Compute hashes
		unsigned int hL = ConvertFilePathToMurmurHash(lineBuf, 1);
		unsigned int hU = ConvertFilePathToMurmurHash(lineBuf, 0);

		// Check for duplicates in existing + newly added entries
		bool duplicate = 0;
		for(unsigned int j = 0; j < hashListSize + added; j++)
		{
			if(newHashList[j].hash_lowercase == hL && newHashList[j].hash_highercase == hU)
			{
				duplicate = 1;
				break;
			}
		}
		if(duplicate)
			continue;

		// Add entry
		hashList_s *e = &newHashList[hashListSize + added];
		e->hash_lowercase = hL;
		e->hash_highercase = hU;
		strncpy(e->path, lineBuf, MAXPATH - 1);
		e->path[MAXPATH - 1] = 0;
		added++;
	}

	delete[]manifestData;

	hashList = newHashList;
	hashListSize = hashListSize + added;
	if(added > 0)
		printf("Loaded %u file paths from embedded manifest.\n", added);
}

void ListFilesInPAK(char *filename)
{
	printf("Attempting to list files in %s...\n", filename);

	//Load PAKFile
	pakHeader_s pakHeader;
	pakEntry_s *pakEntries;
	unsigned long long pakSize;
	if(!LoadPAKFile(filename, &pakHeader, &pakEntries, &pakSize))
		return;

	//Auto-read embedded manifest for filename resolution
	LoadManifestFromPAK(filename, &pakHeader, pakEntries);

	//Load file for trimmed list
	FILE *trimmedList = 0;
	int trimmedListEntryCount = 0;
	if(outputTrimmedList)
		fopen_s(&trimmedList, "trimmed.list", "wb");

	//Make variant of pak filename without any extension
	char pakDirName[MAXPATH];
	FilenameVariantWithoutPathAndExtension(pakDirName, filename);
	
	//Text file
	char listFilename[MAXPATH];
	sprintf_s(listFilename, MAXPATH, "%s.txt", pakDirName);
	FILE *file;
	fopen_s(&file, listFilename, "wb");
	if(!file)
	{
		printf("Could not open %s for writing\n", listFilename);
		return;
	}

	//List all files
	printf("Listing files...\n");
	unsigned int uncompressed = 0, compressedWithDeflate = 0, compressedWithZstd = 0, knownFilenames = 0, unknownFilenames = 0, unknownCompression = 0;
	for(unsigned int i = 0; i < pakHeader.entryCount; i++)
	{
		char *hashName = ReturnFilenameFromHashList(pakEntries[i].filenameHashL, pakEntries[i].filenameHashU);
		if(pakEntries[i].flag & 1) compressedWithDeflate++;
		else if(pakEntries[i].flag & 2) compressedWithZstd++;
		else uncompressed++;

		if(hashName && trimmedList) //Add matching path to trimmed file list if we're making one
		{
			trimmedListEntryCount++;
			fprintf(trimmedList, "%s\n", hashName);
		}

		if(hashName)
		{
			fprintf(file, "%s (compSize: %I64u, realSize: %I64u, flag: %I64u, offset: %I64u, unk2: %u, unk4: %u)\r\n", hashName, pakEntries[i].compressedSize, pakEntries[i].realSize, pakEntries[i].flag, pakEntries[i].offset, pakEntries[i].unknown2, pakEntries[i].unknown4); //Verbose
			//fprintf(file, "%s\r\n", hashName);
			printf("%s\n", hashName);
			knownFilenames++;

			/* //Debug
			//Calculation test
			unsigned int murmurHash = ConvertFilePathToMurmurHash(hashName);
			if(murmurHash != pakEntries[i].filenameHashL)
				printf("Warning: Mismatch between hash calculations.\n");
				*/
		}
		else
		{
			if(!skipFilesWithUnknownPath)
			{
				//Generate filename based on hash data
				char finalFilepath[MAXPATH];
				sprintf(finalFilepath, "%u-%u", pakEntries[i].filenameHashL, pakEntries[i].filenameHashU);

				//Write filename
				fprintf(file, "%s (compSize: %I64u, realSize: %I64u, flag: %I64u, offset: %I64u, unk2: %u, unk4: %u)\r\n", finalFilepath, pakEntries[i].compressedSize, pakEntries[i].realSize, pakEntries[i].flag, pakEntries[i].offset, pakEntries[i].unknown2, pakEntries[i].unknown4); //Verbose
				//fprintf(file, "%s\r\n", finalFilepath);
				printf("%s\n", finalFilepath);
			}
			unknownFilenames++;
		}
	}

	/*
	//Count duplicate lower case hashes
	unsigned int duplicateHashes = 0;
	for (unsigned int i = 0; i < pakHeader.entryCount; i++)
	{
		for (unsigned int j = 0; j < pakHeader.entryCount; j++)
		{
			if (i == j)
				continue;
			if (pakEntries[i].filenameHashL == pakEntries[j].filenameHashL)
				duplicateHashes++;
		}
	}
	*/

	//Finish
	delete[]pakEntries;
	fclose(file);
	if(trimmedList)
	{
		fclose(trimmedList);
		printf("Wrote trimmed.list with %i entries.\n", trimmedListEntryCount);
	}
	printf("Wrote %s\n", listFilename);
	printf("Listed %i files from %s\n", pakHeader.entryCount, filename);
	printf("%u (%u%%) files stored uncompressed\n", uncompressed, (unsigned int) (( (float) uncompressed / (float) pakHeader.entryCount) * 100));
	printf("%u (%u%%) files stored compressed with deflate\n", compressedWithDeflate, (unsigned int) (( (float) compressedWithDeflate / (float) pakHeader.entryCount) * 100));
	printf("%u (%u%%) files stored compressed with zstd\n", compressedWithZstd, (unsigned int) (( (float) compressedWithZstd / (float) pakHeader.entryCount) * 100));
	printf("%u (%u%%) known filenames\n", knownFilenames, (unsigned int) (( (float) knownFilenames / (float) pakHeader.entryCount) * 100));
	printf("%u (%u%%) unknown filenames\n", unknownFilenames, (unsigned int) (( (float) unknownFilenames / (float) pakHeader.entryCount) * 100));
	/*printf("%u duplicate lower case hashes\n", duplicateHashes);*/
}

void ExtractPAK(char *filename)
{
	printf("Attempting to unpack %s...\n", filename);

	//Load PAKFile
	pakHeader_s pakHeader;
	pakEntry_s *pakEntries;
	unsigned long long pakSize;
	if(!LoadPAKFile(filename, &pakHeader, &pakEntries, &pakSize))
		return;

	//Auto-read embedded manifest for filename resolution
	LoadManifestFromPAK(filename, &pakHeader, pakEntries);

	//Check if there's any invalidated entries, and if so, refuse to extract the PAK
	{
		bool moddedPAK = 0;
		for(unsigned int i = 0; i < pakHeader.entryCount; i++)
		{
			if(pakEntries[i].filenameHashL == 0 && pakEntries[i].filenameHashU == 0)
			{
				moddedPAK = 1;
				break;
			}
		}
		if(moddedPAK)
		{
			printf("Extraction cancelled: There are invalidated entries in the PAK file. Try again with a non-modified PAK archive.\n");
			delete[]pakEntries;
			return;
		}
	}

	FILE *pakFile;
	fopen_s(&pakFile, filename, "rb");
	if(!pakFile)
	{
		delete[]pakEntries;
		return;
	}

	//Load file for trimmed list
	FILE *trimmedList = 0;
	int trimmedListEntryCount = 0;
	if(outputTrimmedList)
		fopen_s(&trimmedList, "trimmed.list", "wb");

	////Save header and TOC
	//{
	//	FILE *file;
	//	fopen_s(&file, "headerandtoc.bin", "wb");
	//	fwrite(&pakHeader, sizeof(pakHeader_s), 1, file);
	//	fwrite(pakEntries, sizeof(pakEntry_s), pakHeader.entryCount, file);
	//	fclose(file);
	//}

	//Make variant of pak filename without any extension
	char pakDirName[MAXPATH];
	FilenameVariantWithoutPathAndExtension(pakDirName, filename);

	//Prepare decompressor
	libdeflate_decompressor *decompressor = libdeflate_alloc_decompressor();

	//Extract all files
	printf("Extracting files...\n");
	bool anyErrors = 0;
	unsigned int filesExtracted = 0, filesSkipped = 0, filesFailed = 0, filesCopied = 0, filesWithDeflate = 0, filesWithZstd = 0;
	for(unsigned int i = 0; i < pakHeader.entryCount; i++)
	{
		char *hashName = ReturnFilenameFromHashList(pakEntries[i].filenameHashL, pakEntries[i].filenameHashU);
		if(hashName == 0 && skipFilesWithUnknownPath)
		{
			filesSkipped++;
			continue;
		}

		if(hashName && trimmedList) //Add matching path to trimmed file list if we're making one
		{
			trimmedListEntryCount++;
			fprintf(trimmedList, "%s\n", hashName);
		}

		char finalFilepath[MAXPATH];
		if(uniqueExtractionDir)
		{
			if(hashName)
				sprintf(finalFilepath, "%s\\%s", pakDirName, hashName);
			else //Generate filename based on hash data (this is a temporary name we update with an extension once we've read file data)
				sprintf(finalFilepath, "%s\\%u-%u", pakDirName, pakEntries[i].filenameHashL, pakEntries[i].filenameHashU);
		}
		else
		{
			if(hashName)
				sprintf(finalFilepath, "%s", hashName);
			else //Generate filename based on hash data (this is a temporary name we update with an extension once we've read file data)
				sprintf(finalFilepath, "%u-%u", pakEntries[i].filenameHashL, pakEntries[i].filenameHashU);
		}
		
		if(pakEntries[i].realSize >= 1000000000 || pakEntries[i].compressedSize >= 1000000000)
		{
			printf("Error: Abnormally high file size for entry %s\n", finalFilepath);
			filesFailed++;
			anyErrors = 1;
			continue;
		}

		if(pakEntries[i].flag & 65536) 
		{
			//This is a new way of stored compressed data in PAK files for MH Rise.
			//Its compressedSize value is way higher than realSize
			//The compressed data starts with a 64-bit value which I suspect is the real "compressedSize" value
			//And then it proceeds with chunks of 64 bytes each, last of which are null bytes (usually the last 32 to 20 bytes)
			printf("Error: Unsupported compressed data type. Skipping entry %s\n", finalFilepath);
			filesFailed++;
			continue;
		}

		if(dontOverwriteFiles)
		{
			//Check if file exists already (in which case we skip it)
			//TODO: If hashName is false we should check if finalFilepath exists with any extension exists (aka finalFilepath.*)
			FILE *pFile = fopen(finalFilepath, "rb");
			if (pFile)
			{
				fclose(pFile);
				filesSkipped++;
				continue;
			}
		}

		//Read data
		unsigned char *compressedData = new unsigned char[pakEntries[i].compressedSize];
		_fseeki64(pakFile, pakEntries[i].offset, SEEK_SET);
		fread(compressedData, 1, pakEntries[i].compressedSize, pakFile);

		bool extractError = 0;
		unsigned char *outData;
		int compressed = 0;
		unsigned long long outSize;
		if(pakEntries[i].flag & 1) //Compressed with deflate
		{
			compressed = 1;
			int uncompressedSize = (int) pakEntries[i].realSize;
			unsigned char *newData = new unsigned char[pakEntries[i].realSize];
			int result = libdeflate_deflate_decompress(decompressor, compressedData, pakEntries[i].compressedSize, newData, pakEntries[i].realSize, 0);
			extractError = result != 0;
			if(result > 0)
			{
				if(result == LIBDEFLATE_BAD_DATA)
					printf("Error: Failed to uncompress %s (deflate). Bad data. Offset: %I64u\n", finalFilepath, pakEntries[i].offset);
				else if(result == LIBDEFLATE_INSUFFICIENT_SPACE)
					printf("Error: Failed to uncompress %s (deflate). Not enough allocated memory. Offset: %I64u\n", finalFilepath, pakEntries[i].offset);
				else if(result == LIBDEFLATE_SHORT_OUTPUT)
					printf("Error: Failed to uncompress %s (deflate). Short output. Offset: %I64u\n", finalFilepath, pakEntries[i].offset);
				else if(result != 0)
					printf("Error: Failed to uncompress %s (deflate). Offset: %I64u\n", finalFilepath, pakEntries[i].offset);
				delete[]newData;
				delete[]compressedData;
				filesFailed++;
				continue;
			}
			outData = newData;
			delete[]compressedData;
			outSize = pakEntries[i].realSize;
		}
		else if(pakEntries[i].flag & 2) //Compressed with zstd
		{
			compressed = 2;
			unsigned char *buffer = new unsigned char[pakEntries[i].realSize];
			size_t dSize = ZSTD_decompress(buffer, pakEntries[i].realSize, compressedData, pakEntries[i].compressedSize);
			if(dSize != pakEntries[i].realSize)
			{
				printf("Error: Failed to uncompress %s (zstd). Offset: %I64u\n", finalFilepath, pakEntries[i].offset);
				delete[]buffer;
				delete[]compressedData;
				filesFailed++;
				continue;
			}

			outData = buffer;
			outSize = pakEntries[i].realSize;
			delete[]compressedData;
		}
		else //Non-compressed
		{
			outData = compressedData;
			outSize = pakEntries[i].compressedSize;
		}

		if(extractError)
			anyErrors = 1;
		if(!extractError)
		{
			filesExtracted++;

			//If we didn't get the real filename of this asset, then try to guess its extension based on the file data's magic
			int fileType = FILETYPE_UNKNOWN;
			if (hashName == 0)
			{
				fileType = GuessFileTypeByFileData(outData, outSize);
				AddExtensionBasedOnFileType(fileType, finalFilepath, outData);
			}
			
			//Write file
			MakeDirectory(finalFilepath);
			FILE *newFile;
			fopen_s(&newFile, finalFilepath, "wb");
			if(newFile)
			{
				fwrite(outData, outSize, 1, newFile);
				fclose(newFile);
				if(compressed == 0)
				{
					printf("Wrote %s (copied)\n", finalFilepath);
					filesCopied++;
				}
				else if(compressed == 1)
				{
					printf("Wrote %s (deflate)\n", finalFilepath);
					filesWithDeflate++;
				}
				else if(compressed == 2)
				{
					printf("Wrote %s (zstd)\n", finalFilepath);
					filesWithZstd++;
				}

				//Check if we should convert this file
				if(fileType == FILETYPE_UNKNOWN)
					fileType = CheckFileType(finalFilepath);
				if(convertTEXtoDDS && fileType == FILETYPE_TEX)
					TEXtoDDS(finalFilepath);
			}
			else
				printf("Error: Failed to open %s for writing.\n", finalFilepath);
			
		}
		delete[]outData;
	}

	//Finish
	libdeflate_free_decompressor(decompressor);
	delete[]pakEntries;
	fclose(pakFile);
	if(trimmedList)
	{
		fclose(trimmedList);
		printf("Wrote trimmed.list with %i entries.\n", trimmedListEntryCount);
	}
	printf("Extracted %i files from %s\n", filesExtracted, filename);
	if(filesSkipped)
		printf("Skipped %i files\n", filesSkipped);
	if(filesFailed)
		printf("Failed to extract %i files\n", filesFailed);
	if(filesCopied)
		printf("Extracted %i files with zero compression\n", filesCopied);
	if(filesWithDeflate)
		printf("Extracted %i files compressed with deflate\n", filesWithDeflate);
	if(filesWithZstd)
		printf("Extracted %i files compressed with zstd\n", filesWithZstd);
	if(anyErrors)
		printf("Errors were encountered.\n");
}