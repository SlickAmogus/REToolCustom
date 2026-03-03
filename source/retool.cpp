#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include "retool.h"
#include "extract.h"
#include "create.h"
#include "replace.h"
#include "misc.h"
#include "hashlist.h"
#include "pakformat.h"
#include "PAKFunctions.h"
#include "TEX\TEXtoDDS.h"
#include "TEX\DDStoTEX.h"
#include "TEX\tex.h"
#include "TEX\TEXmisc.h"
#include "dir.h"

//TODO: Add support for noise_cubemap.tex.11 (it's in the TODO directory). This is a DXT1 texture (I think from DMC5)

//Use relative paths for zstd library
#if defined(_DEBUG)
#pragma comment(lib, "..\\source\\zstd\\bin\\x64_Debug\\libzstd_static.lib")
#else
#pragma comment(lib, "..\\source\\zstd\\bin\\x64_Release\\libzstd_static.lib")
#endif

enum
{
	MODE_NOTHING,
	MODE_EXTRACT,
	MODE_REPACK,
	MODE_REPLACE,
	MODE_INVALIDATE,
	MODE_LISTFILES,
	MODE_MURMUR,
	MODE_SINGLEFILE_TEX,
	MODE_SINGLEFILE_TEX_OUTPUTINFO,
	MODE_SINGLEFILE_TEX_REDUCESIZE,
	MODE_SINGLEFILE_DDS,
	MODE_COMBINEHASHLISTS,
};

hashList_s *hashList = 0;
unsigned int hashListSize = 0;
bool skipFilesWithUnknownPath = 0;
bool outputNewHashList = 0;
bool printHashListReadingUpdates = 0;
bool convertTEXtoDDS = 0;
bool dontOverwriteFiles = 0;
bool forceBC7unorm = 0;
bool keepBC7typeDuringDDStoTEX = 0;
bool keepWidthDuringDDStoTEX = 0;
bool uniqueExtractionDir = 1;
bool alternateInvalidationMethod = 0;
bool outputTrimmedList = 0;
bool replaceTexDuringDownscale = 0;
int forcedWidthDuringDDStoTEX = -1;
bool useZstdCompression = 0;

void HelpText()
{
	printf("RE Engine PAK tool v0.221 (by youtube.com/user/FluffyQuack)\n\n");
	printf("usage: retool [options]\n");
	printf("  -x [pak]		Unpack PAK container file\n");
	printf("  -c [folder]		Create a PAK file from directory\n");
	printf("  -i [path]		Invalidate entries in PAK container\n");
	printf("  -iAlt [path] [pak]	Alternate method for PAK entry invalidation\n");
	//printf("  -r [path]		Replace file data in PAK container\n");
	printf("  -l [pak]		List files in PAK container\n");
	printf("  -h [hashList]		Supply list of hash filename for PAK extraction/listing\n");
	printf("  -skipUnknowns		Skip files with unknown path while unpacking PAK\n");
	printf("  -tex			Convert TEX files to DDS while unpacking PAK\n");
	printf("  -keepBC7type		Don't change BC7 type during DDStoTEX\n");
	printf("  -keepWidth		Don't change saved width during DDStoTEX\n");
	printf("  -noExtractDir		Extract PAK data into current folder\n");
	printf("  -tex [file]		Convert one TEX file to DDS\n");
	printf("  -dds [file]		Convert one DDS file to TEX\n");
	printf("  -murmur [string]	Convert string to Murmur3 hash\n");
	printf("  -outputHashList	Creates new hash list without duplicates\n");
	printf("  -combineHashlists	Combines two lists and removes duplicates\n");
	printf("  -dontOverwrite	Skip extracting files that exist\n");
	printf("  -texInfo [file]	Output info about a TEX file\n");
	printf("  -texReduce [file]	Reduce resolution of a TEX file\n");
	printf("  -texReduceBy [#]	Divide TEX resolution by this (default is 4)\n");
	printf("  -trimList		Output file list with only matching paths\n");
	printf("  -zstd			Use ZSTD compression when creating PAK\n");
	printf("  -savedWidth [width]	Defined a forced width for the TEX header\n");
}

int _tmain(int argc, _TCHAR *argv[])
{
	//Defaults
	int mode = MODE_NOTHING;
	bool createdNewHashList = 0;

	//Process command line arguments
	int i = 1, strcount = 0;
	char *str1 = 0, *str2 = 0, *filenameHashList = 0;
	while(argc > i)
	{
		if(argv[i][0] == '-')
		{
			if(_stricmp(argv[i], "-x") == 0)
				mode = MODE_EXTRACT;
			else if(_stricmp(argv[i], "-c") == 0)
				mode = MODE_REPACK;
			else if(_stricmp(argv[i], "-r") == 0)
				mode = MODE_REPLACE;
			else if(_stricmp(argv[i], "-l") == 0)
				mode = MODE_LISTFILES;
			else if(_stricmp(argv[i], "-i") == 0)
				mode = MODE_INVALIDATE;
			else if(_stricmp(argv[i], "-iAlt") == 0)
			{
				mode = MODE_INVALIDATE;
				alternateInvalidationMethod = 1;
			}
			else if(_stricmp(argv[i], "-h") == 0 || _stricmp(argv[i], "-hashList") == 0)
			{
				if (argc > i)
				{
					filenameHashList = argv[i + 1];
					i++;
				}
			}
			else if(_stricmp(argv[i], "-murmur") == 0)
				mode = MODE_MURMUR;
			else if(_stricmp(argv[i], "-skipUnknowns") == 0)
				skipFilesWithUnknownPath = 1;
			else if(_stricmp(argv[i], "-dontOverwrite") == 0 || _stricmp(argv[i], "-skipExisting") == 0)
				dontOverwriteFiles = 1;
			else if(_stricmp(argv[i], "-outputHashList") == 0)
			{
				outputNewHashList = 1;
				printHashListReadingUpdates = 1;
			}
			else if(_stricmp(argv[i], "-combineHashlists") == 0)
			{
				mode = MODE_COMBINEHASHLISTS;
				//printHashListReadingUpdates = 1;
			}
			else if(_stricmp(argv[i], "-tex") == 0)
				convertTEXtoDDS = 1;
			else if(_stricmp(argv[i], "-noExtractDir") == 0)
				uniqueExtractionDir = 0;
			else if(_stricmp(argv[i], "-bc7_unorm") == 0)
				forceBC7unorm = 1;
			else if(_stricmp(argv[i], "-keepBC7type") == 0)
				keepBC7typeDuringDDStoTEX = 1;
			else if(_stricmp(argv[i], "-texInfo") == 0)
				mode = MODE_SINGLEFILE_TEX_OUTPUTINFO;
			else if(_stricmp(argv[i], "-texReduce") == 0 || _stricmp(argv[i], "-reduceTex") == 0)
				mode = MODE_SINGLEFILE_TEX_REDUCESIZE;
			else if(_stricmp(argv[i], "-texReduceBy") == 0 || _stricmp(argv[i], "-reduceTexBy") == 0)
			{
				if(argc > i)
				{
					divideTexBy = atoi(argv[i + 1]);
					i++;
				}
			}
			else if(_stricmp(argv[i], "-trimList") == 0)
				outputTrimmedList = 1;
			else if(_stricmp(argv[i], "-replaceTex") == 0)
				replaceTexDuringDownscale = 1;
			else if(_stricmp(argv[i], "-keepWidth") == 0)
				keepWidthDuringDDStoTEX = 1;
			else if(_stricmp(argv[i], "-zstd") == 0)
			useZstdCompression = 1;
		else if(_stricmp(argv[i], "-savedWidth") == 0)
			{
				if(argc > i)
				{
					forcedWidthDuringDDStoTEX = atoi(argv[i + 1]);
					i++;
				}
			}
		}
		else
		{
			if(strcount == 0)
				str1 = argv[i];
			else if(strcount == 1)
				str2 = argv[i];
			strcount++;
		}
		i++;
	}

	Dir_Init();
	if(filenameHashList && mode != MODE_COMBINEHASHLISTS) //If the filename has been defined for a hash list, then load that
	{
		ReadHashList(filenameHashList);
		if(outputNewHashList)
			createdNewHashList = OutputHashList(filenameHashList);
	}

	if(mode == MODE_NOTHING && str1) //Try to determine mode if no mode is specified but we have one string (potentially file or directory name)
	{
		int fileType = CheckFileType(str1);

		if(fileType == FILETYPE_PAK)
			mode = MODE_EXTRACT;
		else if(fileType == FILETYPE_TEX)
			mode = MODE_SINGLEFILE_TEX;
		else if(fileType == FILETYPE_DDS)
			mode = MODE_SINGLEFILE_DDS;
		else if(fileType == FILETYPE_UNKNOWN && SymbolCountInString(str1, '.') == 0) //Assume this is a directory name
			mode = MODE_REPACK;
	}

	if(mode == MODE_EXTRACT && str1)
	{
		ExtractPAK(str1);
	}
	else if(mode == MODE_REPACK && str1)
	{
		CreatePAK(str1);
	}
	else if(mode == MODE_REPLACE && str1)
	{
		ReplaceFilesInPAK(str1);
	}
	else if(mode == MODE_INVALIDATE && str1)
	{
		InvalidateFilesInPAK(str1, str2);
	}
	else if(mode == MODE_LISTFILES && str1)
	{
		ListFilesInPAK(str1);
	}
	else if(mode == MODE_MURMUR && str1)
	{
		unsigned int hashNum = ConvertFilePathToMurmurHash(str1, 1);
		unsigned int hashNum_upper = ConvertFilePathToMurmurHash(str1, 0);
		unsigned int hashNum_normalcase = ConvertFilePathToMurmurHash(str1, 0, 0);
		unsigned int hashNum_ansi = ConvertAnsiStringMurmurHash(str1);
		printf("Murmur3 hashes for %s (converted to wide string and ignoring null terminator):\n", str1);
		printf("Lower case: %u\n", hashNum);
		printf("Upper case: %u\n", hashNum_upper);
		printf("Non-modified case: %u\n", hashNum_normalcase);
		printf("Non-modified case (ansi): %u\n", hashNum_ansi);

		hashNum = ConvertFilePathToMurmurHash(str1, 1, 1, 1);
		hashNum_upper = ConvertFilePathToMurmurHash(str1, 0, 1, 1);
		hashNum_normalcase = ConvertFilePathToMurmurHash(str1, 0, 0, 1);
		hashNum_ansi = ConvertAnsiStringMurmurHash(str1, 1);
		printf("\nMurmur3 hashes for %s (converted to wide string and keeping null terminator):\n", str1);
		printf("Lower case: %u\n", hashNum);
		printf("Upper case: %u\n", hashNum_upper);
		printf("Non-modified case: %u\n", hashNum_normalcase);
		printf("Non-modified case (ansi): %u\n", hashNum_ansi);
	}
	else if(mode == MODE_SINGLEFILE_TEX && str1)
	{
		TEXtoDDS(str1);
	}
	else if(mode == MODE_SINGLEFILE_DDS && str1)
	{
		DDStoTEX(str1);
	}
	else if(mode == MODE_COMBINEHASHLISTS && str1 && str2)
	{
		if(ReadHashList(str2))
		{
			hashList_s *altHashList = hashList;
			unsigned int altHashListSize = hashListSize;
			hashList = 0;
			hashListSize = 0;

			if(ReadHashList(str1))
				CombineHashLists(str2, altHashList, altHashListSize);
			delete[]altHashList;
			altHashList = 0;
		}
	}
	else if(mode == MODE_SINGLEFILE_TEX_OUTPUTINFO && str1)
	{
		OutputTEXInformation(str1);
	}
	else if(mode == MODE_SINGLEFILE_TEX_REDUCESIZE && str1)
	{
		ReduceTEXSize(str1);
	}
	else
		mode = MODE_NOTHING;

	if(mode == MODE_NOTHING && createdNewHashList == 0)
		HelpText();

	if(hashList)
	{
		delete[]hashList;
		hashList = 0;
	}
	
	Dir_Deinit();
	return 1;
}