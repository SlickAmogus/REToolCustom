#pragma once

#define MAXPATH 260

struct hashList_s
{
	unsigned int hash_lowercase;
	unsigned int hash_highercase;
	char path[MAXPATH];
};

extern hashList_s *hashList;
extern unsigned int hashListSize;
extern bool skipFilesWithUnknownPath;
extern bool printHashListReadingUpdates;
extern bool convertTEXtoDDS;
extern bool dontOverwriteFiles;
extern bool forceBC7unorm;
extern bool keepBC7typeDuringDDStoTEX;
extern bool keepWidthDuringDDStoTEX;
extern bool uniqueExtractionDir;
extern bool alternateInvalidationMethod;
extern bool outputTrimmedList;
extern bool replaceTexDuringDownscale;
extern int forcedWidthDuringDDStoTEX;
extern bool useZstdCompression;