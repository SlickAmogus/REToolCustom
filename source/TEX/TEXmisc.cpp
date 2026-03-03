#include <stdio.h>
#include <tchar.h>
#include "tex.h"
#include "..\misc.h"
#include "..\retool.h"
#include <math.h>

char *dx10TypeNames[] =
{
	"DXGI_FORMAT_UNKNOWN",
	"DXGI_FORMAT_R32G32B32A32_TYPELESS",
	"DXGI_FORMAT_R32G32B32A32_FLOAT",
	"DXGI_FORMAT_R32G32B32A32_UINT",
	"DXGI_FORMAT_R32G32B32A32_SINT",
	"DXGI_FORMAT_R32G32B32_TYPELESS",
	"DXGI_FORMAT_R32G32B32_FLOAT",
	"DXGI_FORMAT_R32G32B32_UINT",
	"DXGI_FORMAT_R32G32B32_SINT",
	"DXGI_FORMAT_R16G16B16A16_TYPELESS",
	"DXGI_FORMAT_R16G16B16A16_FLOAT",
	"DXGI_FORMAT_R16G16B16A16_UNORM",
	"DXGI_FORMAT_R16G16B16A16_UINT",
	"DXGI_FORMAT_R16G16B16A16_SNORM",
	"DXGI_FORMAT_R16G16B16A16_SINT",
	"DXGI_FORMAT_R32G32_TYPELESS",
	"DXGI_FORMAT_R32G32_FLOAT",
	"DXGI_FORMAT_R32G32_UINT",
	"DXGI_FORMAT_R32G32_SINT",
	"DXGI_FORMAT_R32G8X24_TYPELESS",
	"DXGI_FORMAT_D32_FLOAT_S8X24_UINT",
	"DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS",
	"DXGI_FORMAT_X32_TYPELESS_G8X24_UINT",
	"DXGI_FORMAT_R10G10B10A2_TYPELESS",
	"DXGI_FORMAT_R10G10B10A2_UNORM",
	"DXGI_FORMAT_R10G10B10A2_UINT",
	"DXGI_FORMAT_R11G11B10_FLOAT",
	"DXGI_FORMAT_R8G8B8A8_TYPELESS",
	"DXGI_FORMAT_R8G8B8A8_UNORM",
	"DXGI_FORMAT_R8G8B8A8_UNORM_SRGB",
	"DXGI_FORMAT_R8G8B8A8_UINT",
	"DXGI_FORMAT_R8G8B8A8_SNORM",
	"DXGI_FORMAT_R8G8B8A8_SINT",
	"DXGI_FORMAT_R16G16_TYPELESS",
	"DXGI_FORMAT_R16G16_FLOAT",
	"DXGI_FORMAT_R16G16_UNORM",
	"DXGI_FORMAT_R16G16_UINT",
	"DXGI_FORMAT_R16G16_SNORM",
	"DXGI_FORMAT_R16G16_SINT",
	"DXGI_FORMAT_R32_TYPELESS",
	"DXGI_FORMAT_D32_FLOAT",
	"DXGI_FORMAT_R32_FLOAT",
	"DXGI_FORMAT_R32_UINT",
	"DXGI_FORMAT_R32_SINT",
	"DXGI_FORMAT_R24G8_TYPELESS",
	"DXGI_FORMAT_D24_UNORM_S8_UINT",
	"DXGI_FORMAT_R24_UNORM_X8_TYPELESS",
	"DXGI_FORMAT_X24_TYPELESS_G8_UINT",
	"DXGI_FORMAT_R8G8_TYPELESS",
	"DXGI_FORMAT_R8G8_UNORM",
	"DXGI_FORMAT_R8G8_UINT",
	"DXGI_FORMAT_R8G8_SNORM",
	"DXGI_FORMAT_R8G8_SINT",
	"DXGI_FORMAT_R16_TYPELESS",
	"DXGI_FORMAT_R16_FLOAT",
	"DXGI_FORMAT_D16_UNORM",
	"DXGI_FORMAT_R16_UNORM",
	"DXGI_FORMAT_R16_UINT",
	"DXGI_FORMAT_R16_SNORM",
	"DXGI_FORMAT_R16_SINT",
	"DXGI_FORMAT_R8_TYPELESS",
	"DXGI_FORMAT_R8_UNORM",
	"DXGI_FORMAT_R8_UINT",
	"DXGI_FORMAT_R8_SNORM",
	"DXGI_FORMAT_R8_SINT",
	"DXGI_FORMAT_A8_UNORM",
	"DXGI_FORMAT_R1_UNORM",
	"DXGI_FORMAT_R9G9B9E5_SHAREDEXP",
	"DXGI_FORMAT_R8G8_B8G8_UNORM",
	"DXGI_FORMAT_G8R8_G8B8_UNORM",
	"DXGI_FORMAT_BC1_TYPELESS",
	"DXGI_FORMAT_BC1_UNORM",
	"DXGI_FORMAT_BC1_UNORM_SRGB",
	"DXGI_FORMAT_BC2_TYPELESS",
	"DXGI_FORMAT_BC2_UNORM",
	"DXGI_FORMAT_BC2_UNORM_SRGB",
	"DXGI_FORMAT_BC3_TYPELESS",
	"DXGI_FORMAT_BC3_UNORM",
	"DXGI_FORMAT_BC3_UNORM_SRGB",
	"DXGI_FORMAT_BC4_TYPELESS",
	"DXGI_FORMAT_BC4_UNORM",
	"DXGI_FORMAT_BC4_SNORM",
	"DXGI_FORMAT_BC5_TYPELESS",
	"DXGI_FORMAT_BC5_UNORM",
	"DXGI_FORMAT_BC5_SNORM",
	"DXGI_FORMAT_B5G6R5_UNORM",
	"DXGI_FORMAT_B5G5R5A1_UNORM",
	"DXGI_FORMAT_B8G8R8A8_UNORM",
	"DXGI_FORMAT_B8G8R8X8_UNORM",
	"DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM",
	"DXGI_FORMAT_B8G8R8A8_TYPELESS",
	"DXGI_FORMAT_B8G8R8A8_UNORM_SRGB",
	"DXGI_FORMAT_B8G8R8X8_TYPELESS",
	"DXGI_FORMAT_B8G8R8X8_UNORM_SRGB",
	"DXGI_FORMAT_BC6H_TYPELESS",
	"DXGI_FORMAT_BC6H_UF16",
	"DXGI_FORMAT_BC6H_SF16",
	"DXGI_FORMAT_BC7_TYPELESS",
	"DXGI_FORMAT_BC7_UNORM",
	"DXGI_FORMAT_BC7_UNORM_SRGB",
	"DXGI_FORMAT_AYUV",
	"DXGI_FORMAT_Y410",
	"DXGI_FORMAT_Y416",
	"DXGI_FORMAT_NV12",
	"DXGI_FORMAT_P010",
	"DXGI_FORMAT_P016",
	"DXGI_FORMAT_420_OPAQUE",
	"DXGI_FORMAT_YUY2",
	"DXGI_FORMAT_Y210",
	"DXGI_FORMAT_Y216",
	"DXGI_FORMAT_NV11",
	"DXGI_FORMAT_AI44",
	"DXGI_FORMAT_IA44",
	"DXGI_FORMAT_P8",
	"DXGI_FORMAT_A8P8",
	"DXGI_FORMAT_B4G4R4A4_UNORM",
	"DXGI_FORMAT_P208",
	"DXGI_FORMAT_V208",
	"DXGI_FORMAT_V408",
	"DXGI_FORMAT_FORCE_UINT"
};

int divideTexBy = 4;

int TexVersion(int extension)
{
	if(extension == TEXEXTENSION_RE7 || extension == TEXEXTENSION_RE2 || extension == TEXEXTENSION_DMC5 || extension == TEXEXTENSION_RE3 || extension == TEXEXTENSION_RESISTANCE)
		return TEXVERSION_RE7;
	else if(extension == TEXEXTENSION_MHWILDS_1 || extension == TEXEXTENSION_RE9 || extension == TEXEXTENSION_MHWILDS_2)
		return TEXVERSION_RE9;
	else
		return TEXVERSION_RE8;
}

textureInfo_s ReadTexHeader(unsigned char *data)
{
	//Fill in info from header
	textureInfo_s texInfo;
	int textureHeaderSize = 0;
	int extension = (unsigned int &)data[4];
	if(TexVersion(extension) == TEXVERSION_RE7)
	{
		texHeader_s *texHeader = (texHeader_s *)data;
		texInfo.width = texHeader->width;
		texInfo.height = texHeader->height;
		texInfo.mipCount = texHeader->mipCount;
		texInfo.flags = texHeader->flags;
		texInfo.type = texHeader->type;
		texInfo.imgCount = texHeader->imgCount;
		texInfo.extension = texHeader->extension;
		texInfo.headerSize = sizeof(texHeader_s);
		texInfo.usesPackedMips = false;
	}
	else
	{
		texHeader_v30_s *texHeader = (texHeader_v30_s *)data;
		texInfo.width = texHeader->width;
		texInfo.height = texHeader->height;
		texInfo.mipCount = texHeader->mipCount / 16;
		texInfo.flags = texHeader->flags;
		texInfo.type = texHeader->type;
		texInfo.imgCount = texHeader->imgCount;
		texInfo.extension = texHeader->extension;
		texInfo.headerSize = sizeof(texHeader_v30_s);
		texInfo.usesPackedMips = (TexVersion(extension) == TEXVERSION_RE9);
	}
	return texInfo;
}

void OutputTEXInformation(char *filename)
{
	//Filename without path
	char *filename_short;
	{
		int lastSlash = -1;
		int pos = 0;
		while(filename[pos] != 0)
		{
			if (filename[pos] == '\\' || filename[pos] == '/')
				lastSlash = pos;
			pos++;
		}

		if(lastSlash > 0)
		{
			if(filename[lastSlash + 1] != 0)
				filename_short = &filename[lastSlash + 1];
			else
				filename_short = filename;
		}
		else
			filename_short = filename;
	}

	//Open file
	unsigned char *data;
	unsigned int dataSize;
	if(!ReadFile(filename, &data, &dataSize))
	{
		printf("Failed to open %s for reading.\n", filename_short);
		return;
	}

	//Check magic
	if((unsigned int &)data[0] != TEXMAGIC)
	{
		printf("Error: %s is not a TEX file.\n", filename_short);
		delete[]data;
		return;
	}

	//Attain version
	unsigned int extension = (unsigned int &)data[4];

	//Output header info
	int textureHeaderSize = 0;
	if(TexVersion(extension) == TEXVERSION_RE7)
	{
		texHeader_s *texHeader = (texHeader_s *)data;

		//Manage headers
		mipHeader_s *mipHeader = (mipHeader_s *)&data[sizeof(texHeader_s)];

		//Output info
		printf("%s\n", filename_short);
		printf("texHeader->magic: %u\n", texHeader->magic);
		printf("texHeader->extension: %u\n", texHeader->extension);
		printf("texHeader->height: %u\n", texHeader->height);
		printf("texHeader->unknown1: %u\n", texHeader->unknown1);
		printf("texHeader->unknown2: %u\n", texHeader->unknown2);
		printf("texHeader->mipCount: %u\n", texHeader->mipCount);
		printf("texHeader->imgCount: %u\n", texHeader->imgCount);
		printf("texHeader->type: %u (%s)\n", texHeader->type, dx10TypeNames[texHeader->type]);
		printf("texHeader->unknown6: %i\n", texHeader->unknown6);
		printf("texHeader->unknown7: %u\n", texHeader->unknown7);
		printf("texHeader->flags: %u", texHeader->flags & 1);
		for(int i = 1; i < 32; i++)
		{
			printf("%u", (texHeader->flags & 1 << i) != 0);
		}
		printf("\n");
		printf("mipHeader->offsetForImageData: %llu\n", mipHeader->offsetForImageData);
		printf("mipHeader->pitch: %u\n", mipHeader->pitch);
		printf("mipHeader->imageDataSize: %u\n", mipHeader->imageDataSize);
	}
	else
	{
		texHeader_v30_s *texHeader = (texHeader_v30_s *)data;

		//Manage headers
		mipHeader_s *mipHeader = (mipHeader_s *)&data[sizeof(texHeader_v30_s)];

		//Output info
		printf("%s\n", filename_short);
		printf("texHeader->magic: %u\n", texHeader->magic);
		printf("texHeader->extension: %u\n", texHeader->extension);
		printf("texHeader->height: %u\n", texHeader->height);
		printf("texHeader->unknown1: %u\n", texHeader->unknown1);
		printf("texHeader->unknown2: %u\n", texHeader->unknown2);
		printf("texHeader->mipCount: %u\n", texHeader->mipCount);
		printf("texHeader->imgCount: %u\n", texHeader->imgCount);
		printf("texHeader->type: %u (%s)\n", texHeader->type, dx10TypeNames[texHeader->type]);
		printf("texHeader->unknown6: %i\n", texHeader->unknown6);
		printf("texHeader->unknown7: %u\n", texHeader->unknown7);
		printf("texHeader->unknown8: %llu\n", texHeader->unknown8);
		printf("texHeader->flags: %u", texHeader->flags & 1);
		for(int i = 1; i < 32; i++)
		{
			printf("%u", (texHeader->flags & 1 << i) != 0);
		}
		printf("\n");
		printf("mipHeader->offsetForImageData: %llu\n", mipHeader->offsetForImageData);
		printf("mipHeader->pitch: %u\n", mipHeader->pitch);
		printf("mipHeader->imageDataSize: %u\n", mipHeader->imageDataSize);
	}

	//Finish
	delete[]data;
}

void ReduceTEXSize(char *filename)
{
	//Check if this is a valid number to divide by
	int mipReduction = -1;
	for(int i = 1; i < 32; i++)
	{
		if(1 << i == divideTexBy)
		{
			mipReduction = i;
			break;
		}
	}
	if(mipReduction == -1)
	{
		printf("%u is an invalid number to divide resolution by.\n", divideTexBy);
		return;
	}

	//Filename without path
	char *filename_short;
	{
		int lastSlash = -1;
		int pos = 0;
		while(filename[pos] != 0)
		{
			if (filename[pos] == '\\' || filename[pos] == '/')
				lastSlash = pos;
			pos++;
		}

		if(lastSlash > 0)
		{
			if (filename[lastSlash + 1] != 0)
				filename_short = &filename[lastSlash + 1];
			else
				filename_short = filename;
		}
		else
			filename_short = filename;
	}

	//New filename
	char newFilename[MAXPATH];
	if(replaceTexDuringDownscale)
	{
		sprintf_s(newFilename, filename_short);
	}
	else
	{
		char pathWithOutExtensions[MAXPATH];
		char *extension;
		int lastSlash = 0, firstDotAfterLastSlash = 0, pos = 0;
		while(1)
		{
			if (filename[pos] == 0)
				break;
			if (filename[pos] == '\\' || filename[pos] == '/')
				lastSlash = pos;
			if (filename[pos] == '.' && (firstDotAfterLastSlash == 0 || firstDotAfterLastSlash < lastSlash))
				firstDotAfterLastSlash = pos;
			pos++;
		}

		if(firstDotAfterLastSlash == 0)
		{
			printf("Invalid filename.\n");
			return;
		}

		memcpy(pathWithOutExtensions, filename, firstDotAfterLastSlash);
		pathWithOutExtensions[firstDotAfterLastSlash] = 0;
		extension = &filename[firstDotAfterLastSlash + 1];
		sprintf_s(newFilename, "%s-small.%s", pathWithOutExtensions, extension);
	}

	//Open file
	unsigned char *data;
	unsigned int dataSize;
	if(!ReadFile(filename, &data, &dataSize))
	{
		printf("Failed to open %s for reading.\n", filename_short);
		return;
	}

	//Check magic
	if((unsigned int&)data[0] != TEXMAGIC)
	{
		delete[]data;
		return;
	}

	//Fill in info from header
	textureInfo_s texInfo = ReadTexHeader(data);

	//Manage headers
	mipHeader_s *mipHeader = (mipHeader_s *)&data[texInfo.headerSize];
	if(texInfo.mipCount < mipReduction + 1)
	{
		printf("Error: Too few mips in %s for resolution reduction.\n", filename_short);
		delete[]data;
		return;
	}

	//Edit headers
	texInfo.height /= divideTexBy;
	texInfo.width /= divideTexBy;
	unsigned int offsetReduction = sizeof(mipHeader_s) * mipReduction;
	unsigned long long imageDataStart = 0;
	for(int i = 0; i < texInfo.mipCount; i++)
	{
		if (i == mipReduction)
			imageDataStart = mipHeader[i].offsetForImageData;
		if (i < mipReduction)
			offsetReduction += mipHeader[i].imageDataSize;
		else
			mipHeader[i].offsetForImageData -= offsetReduction;
	}
	texInfo.mipCount -= mipReduction;
	texInfo.flags |= TEXFLAGS_LOWRES; //Define this as a lowres texture

	//Open TEX file for writing
	FILE *file;
	fopen_s(&file, newFilename, "wb");
	if(!file)
	{
		printf("Error: Couldn't open %s for writing.\n", newFilename);
		delete[]data;
		return;
	}

	//Write TEX file (start with TEX header)
	if(TexVersion(texInfo.extension) == TEXVERSION_RE7)
	{
		texHeader_s* texHeader = (texHeader_s*)data;
		texHeader->flags = texInfo.flags;
		texHeader->mipCount = texInfo.mipCount;
		texHeader->height = texInfo.height;
		texHeader->width = texInfo.width;
		fwrite(texHeader, sizeof(texHeader_s), 1, file);
	}
	else
	{
		texHeader_v30_s* texHeader = (texHeader_v30_s*)data;
		texHeader->width = texInfo.width;
		texHeader->height = texInfo.height;
		texHeader->mipCount = texInfo.mipCount * 16;
		texHeader->flags = texInfo.flags;
		fwrite(texHeader, sizeof(texHeader_v30_s), 1, file);
	}

	//Mip headers
	if(texInfo.mipCount) //Mip headers (we skip the first 2)
		fwrite(&mipHeader[mipReduction], sizeof(mipHeader_s), texInfo.mipCount, file);

	//Image data
	fwrite(&data[imageDataStart], dataSize - imageDataStart, 1, file);

	//Close file
	fclose(file);
	printf("Reduced %s to %ux%u and set ""lowres texture"" flag.\n", filename_short, texInfo.width, texInfo.height);
	printf("Wrote %s\n", newFilename);

	//Finish
	delete[]data;
}