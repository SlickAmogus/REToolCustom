#pragma once

#define TEXMAGIC 5784916 //TEX\0
#define RE9_MIN_PITCH 256 //Minimum pitch alignment for RE9/MHWilds packed mips

/* Texture types:
71 = ?
98 = BC7 (linear?)
99 = BC7 (sRGB?)
*/

/* Flags:
0x01 = Lowres version (if true, the game will try to load version from streaming directory)
*/

enum {
	TEXVERSION_RE7, //RE7, RE2, DMC5, RE3, and Resistance all use the same TEX version
	TEXVERSION_RE8, //RE:Verse and RE8 use a newer version of the format
	TEXVERSION_RE9, //RE9/MHWilds - uses packed mips format
};

enum {
	TEXEXTENSION_RE7 = 8,
	TEXEXTENSION_RE2 = 10,
	TEXEXTENSION_DMC5 = 11,
	TEXEXTENSION_RE3 = 190820018,
	TEXEXTENSION_RESISTANCE = 190820018,
	TEXEXTENSION_MH_RISE = 28, //Image data is swizzled ASTC
	TEXEXTENSION_REVERSE = 30,
	TEXEXTENSION_RE8 = 30,
	TEXEXTENSION_MHWILDS_1 = 241106027,
	TEXEXTENSION_RE9 = 250813143,
	TEXEXTENSION_MHWILDS_2 = 251111100,
};

enum
{
	TEXFLAGS_LOWRES = 0x01,
};

struct textureInfo_s {
	unsigned short width;
	unsigned short height;
	unsigned char mipCount;
	unsigned char imgCount;
	unsigned int flags;
	unsigned int type;
	unsigned int extension;
	int headerSize;
	bool usesPackedMips;
};

#pragma pack(push, 1)
struct mipHeader_s
{
	unsigned long long offsetForImageData;
	unsigned int pitch;
	unsigned int imageDataSize; //Total size of image data of this mip
};

struct packedMipHeader_s
{
	unsigned int size;
	unsigned int offset;
};

struct texHeader_s
{
	unsigned int magic;
	unsigned int extension; //This matches the extension used for the TEX files. Maybe also used as version?
	unsigned short width;
	unsigned short height;
	unsigned char unknown1; //Usually 1
	unsigned char unknown2; //Usually 0
	unsigned char mipCount;
	unsigned char imgCount; //Usually 1
	unsigned int type;
	int unknown6; //Usually -1
	unsigned int unknown7; //Usually 0
	unsigned int flags; //Seen as a lot of different values

	//Series of mipHeader_s for each mip
};

struct texHeader_v30_s
{
	unsigned int magic;
	unsigned int extension; //This matches the extension used for the TEX files. Maybe also used as version?
	unsigned short width;
	unsigned short height;
	unsigned char unknown1; //Usually 1
	unsigned char unknown2; //Usually 0
	unsigned char imgCount;
	unsigned char mipCount;
	unsigned int type;
	int unknown6; //Usually -1
	unsigned int unknown7; //Usually 0
	unsigned int flags; //Seen as a lot of different values

	unsigned long long unknown8;
	//Series of mipHeader_s for each mip
};
#pragma pack(pop)