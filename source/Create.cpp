#include <stdio.h>
#include <direct.h>
#include "pakformat.h"
#include "libdeflate\libdeflate.h"
#include "zstd\zstd.h"
#include "retool.h"
#include "dir.h"
#include "misc.h"
#include <string.h>
#include "PAKFunctions.h"

static bool IsManifestPath(const char *fileName)
{
	// Check if this file is __MANIFEST/MANIFEST.TXT (with either slash direction)
	if(_stricmp(fileName, "__MANIFEST/MANIFEST.TXT") == 0) return true;
	if(_stricmp(fileName, "__MANIFEST\\MANIFEST.TXT") == 0) return true;
	return false;
}

void CreatePAK(char *dirName)
{
	//Check if the path is valid
	if(dirName == 0 || dirName[0] == 0 || strcmp(dirName, ".") == 0)
	{
		printf("Error: Directory path is invalid.\n");
		return;
	}

	//Count files that we'll be adding (skip any existing manifest - we'll generate our own)
	unsigned int fileTotal = 0;
	CreateFileQueue(dirName);
	for(int i = 0; i < dir_alphaqueNum; i++)
	{
		dir_queue_t *dir = &dir_que[dir_alphaque[i]];
		if (!dir->active)
			continue;
		if(dir->fileSize == 0)
			continue;
		if(IsManifestPath(dir->fileName))
			continue;
		fileTotal++;
	}

	unsigned int realFileCount = fileTotal;
	fileTotal++; //Reserve one extra slot for __MANIFEST/MANIFEST.TXT

	//Set up PAK header
	pakHeader_s pakHeader;
	pakHeader.magic = 1095454795;
	pakHeader.flags = 0;
	pakHeader.unknown1 = 0;
	pakHeader.version = 4; //MH Rise has this version number
	pakHeader.entryCount = fileTotal;

	//Open file for writing
	FILE *pakFile = 0;
	char fullPath[MAXPATH] = {0};
	sprintf_s(fullPath, MAXPATH, "%s.pak", dirName);
	fopen_s(&pakFile, fullPath, "wb");
	if(!pakFile)
	{
		printf("Error: Failed to open %s for writing.\n", fullPath);
		return;
	}

	//Setup TOC
	pakEntry_s *pakEntries = new pakEntry_s[fileTotal];

	//Write header and TOC to file so file IO writer is in the correct position for writing file entry data
	fwrite(&pakHeader, sizeof(pakHeader_s), 1, pakFile);
	fwrite(pakEntries, sizeof(pakEntry_s), fileTotal, pakFile); //We'll re-write this later once we've filled in all the correct data
	unsigned long long curOffset = sizeof(pakHeader_s) + (sizeof(pakEntry_s) * fileTotal);

	//Prepare compressor
	libdeflate_compressor *deflateCompressor = libdeflate_alloc_compressor(6);

	//Process all files we'll be adding to the PAK
	unsigned int entryNum = 0;
	for(int i = 0; i < dir_alphaqueNum; i++)
	{
		dir_queue_t *dir = &dir_que[dir_alphaque[i]];
		if (!dir->active)
			continue;
		if(dir->fileSize == 0)
			continue;
		if(IsManifestPath(dir->fileName))
			continue;

		//Convert full file path to murmur hash
		pakEntry_s *pakEntry = &pakEntries[entryNum];
		pakEntry->filenameHashL = ConvertFilePathToMurmurHash(dir->fileName, 1);
		pakEntry->filenameHashU = ConvertFilePathToMurmurHash(dir->fileName, 0);

		//Misc variables for the file entry
		pakEntry->offset = curOffset;
		pakEntry->unknown2 = 0;
		pakEntry->unknown4 = 0;
		pakEntry->flag = 0;

		//Open and read in the file for this entry
		FILE *file = 0;
		char fullEntryPath[MAXPATH] = {0};
		fpos_t entryFileSize = 0;
		sprintf_s(fullEntryPath, MAXPATH, "%s\\%s", dirName, dir->fileName);
		fopen_s(&file, fullEntryPath, "rb");
		if(!file)
		{
			//TODO: Error handling
		}
		fseek(file, 0, SEEK_END);
		fgetpos(file, &entryFileSize);
		fseek(file, 0, SEEK_SET);
		unsigned char *entryFileData = new unsigned char[entryFileSize];
		fread(entryFileData, 1, entryFileSize, file);
		fclose(file);

		//Define entry's real file size
		pakEntry->realSize = entryFileSize;

		//Never compress certain files: modinfo.ini and screenshots in the root directory
		bool tryToCompress = 1;
		if(strcmp(dir->fileName, dir->fileNameWithoutDir) == 0 && (strcmp(dir->fileName, "modinfo.ini") == 0 || strcmp(dir->extension, "jpg") == 0 || strcmp(dir->extension, "png") == 0 || strcmp(dir->extension, "dds") == 0 || strcmp(dir->extension, "jpeg") == 0))
			tryToCompress = 0;
		
		//Try to compress the data
		size_t maxCompressedSize;
		unsigned char *compressedData = 0;
		size_t compressedSize = 0;
		if(tryToCompress)
		{
			if(useZstdCompression)
			{
				maxCompressedSize = ZSTD_compressBound((size_t)entryFileSize);
				compressedData = new unsigned char[maxCompressedSize];
				compressedSize = ZSTD_compress(compressedData, maxCompressedSize, entryFileData, (size_t)entryFileSize, 3);
				if(ZSTD_isError(compressedSize))
					compressedSize = 0; //Treat as compression failure
			}
			else
			{
				maxCompressedSize = libdeflate_deflate_compress_bound(deflateCompressor, (size_t)entryFileSize);
				compressedData = new unsigned char[maxCompressedSize];
				compressedSize = libdeflate_deflate_compress(deflateCompressor, entryFileData, (size_t)entryFileSize, compressedData, maxCompressedSize);
			}
		}

		//Check if we should keep compressed data or uncompressed data
		if(tryToCompress && compressedSize > 0 && compressedSize < (size_t)entryFileSize) //We compressed the data and it's smaller, so keep that
		{
			pakEntry->compressedSize = compressedSize; //Define compressed size
			fwrite(compressedData, 1, compressedSize, pakFile); //Write compressed data to PAK file
			if(useZstdCompression)
			{
				pakEntry->flag |= TOCFLAGS_ZSTD;
				printf("Added %s (zstd)\n", dir->fileName);
			}
			else
			{
				pakEntry->flag |= TOCFLAGS_DEFLATE;
				printf("Added %s (deflate)\n", dir->fileName);
			}
		}
		else //Default to uncompressed data
		{
			pakEntry->compressedSize = entryFileSize; //Compressed size matches real size when uncompressed
			fwrite(entryFileData, 1, (size_t)entryFileSize, pakFile); //Write entry data to PAK file
			printf("Added %s (uncompressed)\n", dir->fileName);
		}

		//Finish
		curOffset += pakEntry->compressedSize;
		delete[]entryFileData;
		if(compressedData)
			delete[]compressedData;
		entryNum++;
	}

	//Generate __MANIFEST/MANIFEST.TXT as the last entry
	{
		// Build manifest text: one file path per line (forward slashes), manifest path as last line
		const char *manifestPath = "__MANIFEST/MANIFEST.TXT";

		// Calculate total manifest size
		size_t manifestSize = 0;
		for(int i = 0; i < dir_alphaqueNum; i++)
		{
			dir_queue_t *d = &dir_que[dir_alphaque[i]];
			if(!d->active || d->fileSize == 0)
				continue;
			if(IsManifestPath(d->fileName))
				continue;
			manifestSize += strlen(d->fileName) + 1; // +1 for newline
		}
		manifestSize += strlen(manifestPath) + 1; // manifest's own path as last line

		unsigned char *manifestData = new unsigned char[manifestSize];
		size_t pos = 0;
		for(int i = 0; i < dir_alphaqueNum; i++)
		{
			dir_queue_t *d = &dir_que[dir_alphaque[i]];
			if(!d->active || d->fileSize == 0)
				continue;
			if(IsManifestPath(d->fileName))
				continue;
			size_t len = strlen(d->fileName);
			memcpy(&manifestData[pos], d->fileName, len);
			// Convert backslashes to forward slashes
			for(size_t j = pos; j < pos + len; j++)
			{
				if(manifestData[j] == '\\')
					manifestData[j] = '/';
			}
			manifestData[pos + len] = '\n';
			pos += len + 1;
		}
		// Add manifest's own path as final line
		size_t mpLen = strlen(manifestPath);
		memcpy(&manifestData[pos], manifestPath, mpLen);
		manifestData[pos + mpLen] = '\n';
		pos += mpLen + 1;

		// Compress manifest
		pakEntry_s *manifestEntry = &pakEntries[entryNum];
		manifestEntry->filenameHashL = ConvertFilePathToMurmurHash((char*)manifestPath, 1);
		manifestEntry->filenameHashU = ConvertFilePathToMurmurHash((char*)manifestPath, 0);
		manifestEntry->offset = curOffset;
		manifestEntry->unknown2 = 0;
		manifestEntry->unknown4 = 0;
		manifestEntry->flag = 0;
		manifestEntry->realSize = manifestSize;

		size_t maxCompSize;
		unsigned char *compManifest = 0;
		size_t compManifestSize = 0;

		if(useZstdCompression)
		{
			maxCompSize = ZSTD_compressBound(manifestSize);
			compManifest = new unsigned char[maxCompSize];
			compManifestSize = ZSTD_compress(compManifest, maxCompSize, manifestData, manifestSize, 3);
			if(ZSTD_isError(compManifestSize))
				compManifestSize = 0;
		}
		else
		{
			maxCompSize = libdeflate_deflate_compress_bound(deflateCompressor, manifestSize);
			compManifest = new unsigned char[maxCompSize];
			compManifestSize = libdeflate_deflate_compress(deflateCompressor, manifestData, manifestSize, compManifest, maxCompSize);
		}

		if(compManifestSize > 0 && compManifestSize < manifestSize)
		{
			manifestEntry->compressedSize = compManifestSize;
			fwrite(compManifest, 1, compManifestSize, pakFile);
			if(useZstdCompression)
				manifestEntry->flag |= TOCFLAGS_ZSTD;
			else
				manifestEntry->flag |= TOCFLAGS_DEFLATE;
		}
		else
		{
			manifestEntry->compressedSize = manifestSize;
			fwrite(manifestData, 1, manifestSize, pakFile);
		}

		curOffset += manifestEntry->compressedSize;
		entryNum++;
		delete[]manifestData;
		if(compManifest)
			delete[]compManifest;
		printf("Added %s\n", manifestPath);
	}

	//Jump back to TOC location and re-write it
	fseek(pakFile, sizeof(pakHeader_s), SEEK_SET);
	fwrite(pakEntries, sizeof(pakEntry_s), fileTotal, pakFile);

	//Finish
	fclose(pakFile);
	delete[]pakEntries;
	libdeflate_free_compressor(deflateCompressor);
	printf("Wrote %s with %u file entries.\n", fullPath, fileTotal);
}