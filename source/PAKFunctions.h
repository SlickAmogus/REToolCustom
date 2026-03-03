#pragma once

bool LoadPAKFile(char *pakPath, pakHeader_s *pakHeader, pakEntry_s **pakEntries, unsigned long long *pakSize = 0);
unsigned int ConvertFilePathToMurmurHash(char *filePath, bool lower = 1, bool changeCase = 1, bool keepNullTerminator = 0);
unsigned int ConvertAnsiStringMurmurHash(char *str, bool keepNullTerminator = 0);