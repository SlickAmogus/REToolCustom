#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "misc.h"
#include "tex\tex.h"

#define TAB 0x09
#define SPACE ' '

//Try to guess a file's extension by looking at file data
int GuessFileTypeByFileData(unsigned char *data, unsigned int dataSize)
{
	if (dataSize < 8)
		return FILETYPE_UNKNOWN;

	unsigned long long *magicLongLong = (unsigned long long *) data;
	unsigned int *magicFirst = (unsigned int *) data;
	unsigned int *magicSecond = (unsigned int *) &data[4];

	if (*magicSecond == 1196641607) return FILETYPE_MSG;
	else if (*magicFirst == TEXMAGIC) return FILETYPE_TEX;
	else if (*magicSecond == 1851877475) return FILETYPE_CHAIN;
	else if (*magicFirst == 4605005) return FILETYPE_MDF;
	else if (*magicFirst == 4343376) return FILETYPE_PFB;
	else if (*magicFirst == 1213416781) return FILETYPE_MESH;
	else if (*magicLongLong == 5742871890 || *magicLongLong == 8104636986402275328) return FILETYPE_MOV;
	else if (*magicFirst == 1145588546) return FILETYPE_BNK;
	else if (*magicFirst == 1263553345) return FILETYPE_PCK;
	else if (*magicSecond == 1953721443) return FILETYPE_MCAMLIST;
	else if (*magicSecond == 544501613) return FILETYPE_MOT;
	else if (*magicSecond == 846423661) return FILETYPE_MOTFSM2;
	else if (*magicSecond == 1953721453) return FILETYPE_MOTLIST;
	else if (*magicSecond == 1802396269) return FILETYPE_MOTBANK;
	else if (*magicFirst == 1095454795) return FILETYPE_PAK;
	else if (*magicFirst == 542327876) return FILETYPE_DDS;
	else if (*magicSecond == 1887007846) return FILETYPE_MP4; //Not sure if this is reliable for all MP4 files. Their headers are kinda funky (I know the fourth byte will typically vary between MP4 files)
	else if (*magicLongLong == 1283357180869879344) return FILETYPE_WMV; //Not tested if this is reliable, but it works for 3 WMV files in DMC5
	else if (*magicFirst == 5129043) return FILETYPE_SCN;
	else if (*magicFirst == 1280262994) return FILETYPE_RCOL;
	else if (*magicSecond == 1936614250) return FILETYPE_JCNS;
	else if (*magicFirst == 1178944579) return FILETYPE_CDEF;
	else if (*magicFirst == 1413565763) return FILETYPE_CMAT;
	else if (*magicSecond == 1885433194) return FILETYPE_JMAP;
	else if (*magicSecond == 1918989941) return FILETYPE_UVAR;
	else if (*magicFirst == 1280262989) return FILETYPE_MCOL;
	else if (*magicFirst == 1414940738) return FILETYPE_BHVT;
	else if (*magicFirst == 1413699654) return FILETYPE_FXCT;
	else if (*magicFirst == 1920493157) return FILETYPE_EFX;
	else if (*magicFirst == 5395285) return FILETYPE_USER;
	else return FILETYPE_UNKNOWN;
}

void AddExtensionBasedOnFileType(int fileType, char *fileName, unsigned char *data)
{
	int stringLength = strlen(fileName);
	char *stringFromNull = &fileName[stringLength];
	if(fileType == FILETYPE_UNKNOWN) sprintf_s(stringFromNull, MAXPATH - stringLength, ".bin");
	else if (fileType == FILETYPE_PAK) sprintf_s(stringFromNull, MAXPATH - stringLength, ".pak");
	else if (fileType == FILETYPE_DDS) sprintf_s(stringFromNull, MAXPATH - stringLength, ".dds");
	else if (fileType == FILETYPE_TEX) sprintf_s(stringFromNull, MAXPATH - stringLength, ".tex.%u", (unsigned int &)data[4]);
	else if (fileType == FILETYPE_MP4) sprintf_s(stringFromNull, MAXPATH - stringLength, ".mp4");
	else if (fileType == FILETYPE_WMV) sprintf_s(stringFromNull, MAXPATH - stringLength, ".wmv");
	else if (fileType == FILETYPE_CHAIN) sprintf_s(stringFromNull, MAXPATH - stringLength, ".chain.21");
	else if (fileType == FILETYPE_MSG) sprintf_s(stringFromNull, MAXPATH - stringLength, ".msg.%u", (unsigned int &)data[0]);
	else if (fileType == FILETYPE_MDF) sprintf_s(stringFromNull, MAXPATH - stringLength, ".mdf2.10");
	else if (fileType == FILETYPE_PFB) sprintf_s(stringFromNull, MAXPATH - stringLength, ".pfb.16");
	else if (fileType == FILETYPE_MESH) sprintf_s(stringFromNull, MAXPATH - stringLength, ".mesh.%u", (unsigned int &)data[4]);
	else if (fileType == FILETYPE_MOV) sprintf_s(stringFromNull, MAXPATH - stringLength, ".mov.1");
	else if (fileType == FILETYPE_BNK) sprintf_s(stringFromNull, MAXPATH - stringLength, ".bnk.2.x64");
	else if (fileType == FILETYPE_PCK) sprintf_s(stringFromNull, MAXPATH - stringLength, ".pck.3.x64");
	else if (fileType == FILETYPE_MCAMLIST) sprintf_s(stringFromNull, MAXPATH - stringLength, ".mcamlist.%u", (unsigned int &)data[0]);
	else if (fileType == FILETYPE_MOT) sprintf_s(stringFromNull, MAXPATH - stringLength, ".mot.%u", (unsigned int &)data[0]);
	else if (fileType == FILETYPE_MOTFSM2) sprintf_s(stringFromNull, MAXPATH - stringLength, ".motfsm2.%u", (unsigned int &)data[0]);
	else if (fileType == FILETYPE_MOTLIST) sprintf_s(stringFromNull, MAXPATH - stringLength, ".motlist.%u", (unsigned int &)data[0]);
	else if (fileType == FILETYPE_MOTBANK) sprintf_s(stringFromNull, MAXPATH - stringLength, ".motbank.%u", (unsigned int &)data[0]);
	else if (fileType == FILETYPE_SCN) sprintf_s(stringFromNull, MAXPATH - stringLength, ".scn.19");
	else if (fileType == FILETYPE_RCOL) sprintf_s(stringFromNull, MAXPATH - stringLength, ".rcol.10");
	else if (fileType == FILETYPE_JCNS) sprintf_s(stringFromNull, MAXPATH - stringLength, ".jcns.11");
	else if (fileType == FILETYPE_CDEF) sprintf_s(stringFromNull, MAXPATH - stringLength, ".cdef.3");
	else if (fileType == FILETYPE_CMAT) sprintf_s(stringFromNull, MAXPATH - stringLength, ".cmat.3");
	else if (fileType == FILETYPE_JMAP) sprintf_s(stringFromNull, MAXPATH - stringLength, ".jmap.10");
	else if (fileType == FILETYPE_UVAR) sprintf_s(stringFromNull, MAXPATH - stringLength, ".uvar.%u", (unsigned int &)data[0]);
	else if (fileType == FILETYPE_MCOL) sprintf_s(stringFromNull, MAXPATH - stringLength, ".mcol.3017");
	else if (fileType == FILETYPE_BHVT) sprintf_s(stringFromNull, MAXPATH - stringLength, ".bhvt.30");
	else if (fileType == FILETYPE_FXCT) sprintf_s(stringFromNull, MAXPATH - stringLength, ".fxct.1");
	else if (fileType == FILETYPE_EFX) sprintf_s(stringFromNull, MAXPATH - stringLength, ".efx.1769672");
	else if (fileType == FILETYPE_USER) sprintf_s(stringFromNull, MAXPATH - stringLength, ".user.2");
}

//Check file format
int CheckFileType(char *str1)
{
	int dotPos = FirstDot(str1);
	if (dotPos != 0)
	{
		dotPos += 1;
		if (str1[dotPos] != 0)
		{
			//Get variant of filename with only first part of extension
			char ext[MAXPATH];
			sprintf(ext, "%s", &str1[dotPos]);
			int dotPos2 = FirstDot(ext);
			if (dotPos2 != 0)
				ext[dotPos2] = 0;

			if (_stricmp(ext, "pak") == 0) //Looks like a filename with pak extension
				return FILETYPE_PAK;
			else if (_stricmp(ext, "tex") == 0) //Texture
				return FILETYPE_TEX;
			else if (_stricmp(ext, "dds") == 0) //DDS
				return FILETYPE_DDS;
		}
	}
	return FILETYPE_UNKNOWN;
}

int LastColon(char *path)
{
	int i = 0, colon = 0;
	while (1)
	{
		if (path[i] == 0)
			break;
		if (path[i] == ':' && path[i + 1] != 0)
			colon = i;
		i++;
	}
	return colon;
}

int LastSlash(char *path)
{
	int i = 0, slash = 0;
	while (1)
	{
		if (path[i] == 0)
			break;
		if ((path[i] == '/' || path[i] == '\\') && path[i + 1] != 0)
			slash = i;
		i++;
	}
	return slash;
}

int LastDot(char *path)
{
	int i = 0, dot = 0;
	while(1)
	{
		if(path[i] == 0)
			break;
		if(path[i] == '.' && path[i + 1] != 0)
			dot = i;
		i++;
	}
	return dot;
}

int FirstDot(char *path) //This finds the first dot after the last slash (to ensure we don't find the dot as part of a directory rather than file)
{
	int i = 0, dot = 0, slash = 0;
	while(1)
	{
		if(path[i] == '\\' || path[i] == '/')
		{
			slash = i;
			dot = 0;
		}
		if(path[i] == 0)
			break;
		if(path[i] == '.' && path[i + 1] != 0 && i > slash && dot == 0)
			dot = i;
		i++;
	}
	return dot;
}

bool ReadFile(char *fileName, unsigned char **data, unsigned int *dataSize)
{
	//Open file
	FILE *file;
	fopen_s(&file, fileName, "rb");
	if(!file)
		return 0;
	fpos_t fpos = 0;
	_fseeki64(file, 0, SEEK_END);
	fgetpos(file, &fpos);
	_fseeki64(file, 0, SEEK_SET);
	*dataSize = (unsigned int) fpos;
	*data = new unsigned char[*dataSize];
	fread(*data, *dataSize, 1, file);
	fclose(file);
	return 1;
}

unsigned char CharToHex(const char *bytes, int offset)
{
	unsigned char value = 0;
	unsigned char finValue = 0;

	for(int i = 0; i < 2; i++)
	{
		char c = bytes[i + offset];
		if( c >= '0' && c <= '9' )
			value = c - '0';
        else if( c >= 'A' && c <= 'F' )
            value = 10 + (c - 'A');
        else if( c >= 'a' && c <= 'f' )
            value = 10 + (c - 'a');

		if(i == 0 && value > 0)
			value *= 16;

		finValue += value;
	}
	return finValue;
}

void CharByteToData(char *num, unsigned char *c, int size, bool littleEndian) //Note, num size has to be twice the size of "size" otherwise this function fails
{
	int j = 0;
	int i;
	if(littleEndian)
		i = 0;
	else
		i = size - 1;
	while(1)
	{
		c[i] = CharToHex(num, j);
		j += 2;
		if(littleEndian)
		{
			if(i == size - 1)
				break;
			i++;
		}
		else
		{
			if(i == 0)
				break;
			i--;
		}
	}
}

bool IsNumber(char *str)
{
	int i = 0;
	while (str[i] != 0)
	{
		if (!(str[i] >= '0' && str[i] <= '9'))
		{
			return 0;
		}
		i++;
	}
	return 1;
}

void RemoveLineBreaks(char *str)
{
	int i = 0;
	while (str[i] != 0)
	{
		if (str[i] == '\r' || str[i] == '\n')
		{
			str[i] = 0;
			return;
		}
		i++;
	}
	return;
}

int SymbolCountInString(char *str, char symbol)
{
	int count = 0, i = 0;
	while (str[i] != 0)
	{
		if (str[i] == symbol)
			count++;
		i++;
	}
	return count;
}

//Separate a string into 2 based on the first occurence of a symbol (it does not keep the separator in either string). Returns true if it found separator
bool SeparateString(char *str, char *str2, char seperator)
{
	bool copy = 0;
	int i = 0, j = 0;
	while(str[i] != 0)
	{
		if(copy)
			str2[j] = str[i], j++;
		if(str[i] == seperator)
			copy = 1, str[i] = 0;
		i++;
	}
	str2[j] = 0;
	return copy;
}

unsigned long long StringToULongLong(char *string)
{
	unsigned long long number = 0, num = 1;
	int length = (int) strlen(string);
	for(int i = length - 1; i >= 0; i--)
	{
		number += (string[i] - '0') * num;
		num *= 10;
	}
	return number;
}

char *ReturnStringPos(char *find, char *in) //Find position of "find" string within "in" string. Does a lower case comparison.
{
	int findPos = 0, inPos = 0, returnPos = 0;
	while (in[inPos] != 0)
	{
		if (tolower(in[inPos]) == tolower(find[findPos]))
		{
			if (findPos == 0)
				returnPos = inPos;
			findPos++;
			if (find[findPos] == 0)
				return &in[returnPos];
		}
		else
		{
			findPos = 0;
			if (returnPos > 0)
			{
				inPos = returnPos + 1;
				returnPos = 0;
			}
		}
		inPos++;
	}
	return 0;
}

//If the final characters are spaces, they are removed. If final characters are not space(s), nothing happens.
void RemoveSpacesAtEnd(char *str)
{
	int l,i=0,lastS=0;
	char c=str[i];
	while(c!=0)
	{
		if(c==SPACE)
			lastS=i;
		i++;
		c=str[i];
	}
	l=i;
	if(lastS==l-1)
	{
		for(i=l-1;i>0;i--)
		{
			if(str[i]==SPACE)
				str[i]=0;
			else
				break;
		}
	}
}

//Removes all spaces and tabs at the start of a string
void RemoveSpacesAtStart(char *str)
{
	if(str[0]!=SPACE && str[0]!=TAB) //If there's no space or tab at the beginning, we don't need to fix string
		return;

	int i=0,j=0;
	bool copyRest=0;
	while(str[i]!=0)
	{
		if(!copyRest)
		{
			if(str[i]==SPACE || str[i]==TAB)
			{
				i++;
				continue;
			}
			else
				copyRest=1;
		}
		if(copyRest)
		{
			str[j]=str[i];
			i++;
			j++;
		}
	}
	str[j]=0;
}