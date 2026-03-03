#pragma once

/*
List of platform names (these are used as part of file paths in PAK archives):
X64 (PC for RE Engine games before RE3)
STM (PC games which are RE3 and newer, probably stands for Steam)
PS4 (Playstation 4)
XB1 (Xbox One)
NSW (Nintendo Switch)
UWP (Universal Windows Platform app?)
WGM (?)
UBT (?)
*/

/*
Version numbers:
4 (DMC5, RE3, RE8)
*/

#define PAKMAGIC 1095454795

enum
{
	PAKFLAGS_ENCRYPTED = (1 << 3),
};

enum
{
	TOCFLAGS_DEFLATE = (1 << 0),
	TOCFLAGS_ZSTD = (1 << 1),
};

#pragma pack(push, 1)
struct pakHeader_s
{
   unsigned int magic;
   unsigned short version;
   unsigned short flags;
   unsigned int entryCount;
   unsigned int unknown1; //File integrity hash?
};

struct pakEntry_s
{
   unsigned int filenameHashL; //Hash of filename in lower case
   unsigned int filenameHashU; //Hash of filename in upper case
   unsigned long long offset;
   unsigned long long compressedSize;
   unsigned long long realSize;
   unsigned long long flag;
   unsigned int unknown2; //?
   unsigned int unknown4; //?
};
#pragma pack(pop)