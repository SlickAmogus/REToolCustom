#pragma once

#define MAXPATH 260

enum
{
	FILETYPE_UNKNOWN,
	FILETYPE_PAK,
	FILETYPE_DDS,
	FILETYPE_TEX,
	FILETYPE_MP4,
	FILETYPE_WMV,
	FILETYPE_CHAIN,
	FILETYPE_MSG,
	FILETYPE_MDF,
	FILETYPE_PFB,
	FILETYPE_MESH,
	FILETYPE_MOV,
	FILETYPE_BNK,
	FILETYPE_PCK,
	FILETYPE_MCAMLIST,
	FILETYPE_MOT,
	FILETYPE_MOTFSM2,
	FILETYPE_MOTLIST,
	FILETYPE_MOTBANK,
	FILETYPE_SCN,
	FILETYPE_RCOL,
	FILETYPE_JCNS,
	FILETYPE_CDEF,
	FILETYPE_CMAT,
	FILETYPE_JMAP,
	FILETYPE_UVAR,
	FILETYPE_MCOL,
	FILETYPE_BHVT,
	FILETYPE_FXCT,
	FILETYPE_EFX,
	FILETYPE_USER,
};

int GuessFileTypeByFileData(unsigned char *data, unsigned int dataSize);
void AddExtensionBasedOnFileType(int fileType, char *fileName, unsigned char *data);
int CheckFileType(char *str1);
int LastColon(char *path);
int LastSlash(char *path);
int LastDot(char *path);
int FirstDot(char *path);
bool ReadFile(char *fileName, unsigned char **data, unsigned int *dataSize);
unsigned char CharToHex(const char *bytes, int offset = 0);
void CharByteToData(char *num, unsigned char *c, int size, bool littleEndian); //Note, num size has to be twice the size of "size" otherwise this function fails
bool IsNumber(char *str);
void RemoveLineBreaks(char *str);
int SymbolCountInString(char *str, char symbol);
bool SeparateString(char *str, char *str2, char seperator);
unsigned long long StringToULongLong(char *string);
char *ReturnStringPos(char *find, char *in); //Find position of "find" string within "in" string. Does a lower case comparison.
void RemoveSpacesAtEnd(char *str);
void RemoveSpacesAtStart(char *str);