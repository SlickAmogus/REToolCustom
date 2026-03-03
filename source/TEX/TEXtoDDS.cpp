#include <stdio.h>
#include <tchar.h>
#include "tex.h"
#include "..\misc.h"
#include "..\retool.h"
#include "DDSInfo.h"
#include "DDSHeaderDefaults.h"
#include "TEXmisc.h"
#include "DDSmisc.h"
#include "gdeflate.h"

void TEXtoDDS(char *filename)
{
	//Filename without path
	char *filename_short;
	{
		int lastSlash = -1;
		int pos = 0;
		while(filename[pos] != 0)
		{
			if(filename[pos] == '\\' || filename[pos] == '/')
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
	if((unsigned int &) data[0] != TEXMAGIC)
	{
		delete[]data;
		return;
	}

	//Fill in info from header
	textureInfo_s texInfo = ReadTexHeader(data);

	//Manage headers
	mipHeader_s *mipHeader = (mipHeader_s *) &data[texInfo.headerSize];
	DDS_HEADER ddsHeader;
	memset(&ddsHeader, 0, sizeof(DDS_HEADER));
	DDS_HEADER_DXT10 dx10Header;
	memset(&dx10Header, 0, sizeof(DDS_HEADER_DXT10));
	bool dx10HeaderPresent = 0;

	//Let's assume every texture is DX10 for now
	dx10HeaderPresent = 1;
	SetDDSHeaderDefaults_BC7(&ddsHeader, &dx10Header);
	if(forceBC7unorm)
		dx10Header.dxgiFormat = DXGI_FORMAT_BC7_UNORM;
	else
		dx10Header.dxgiFormat = texInfo.type;

	/*
	//Figure out type
	bool fail = 0;
	switch(texHeader->type)
	{
	case 71:
		printf("Unsupported texture type %u in %s\n", texHeader->type, filename_short);
		fail = 1;
		break;
	case 98:
	case 99:
		dx10HeaderPresent = 1;
		SetDDSHeaderDefaults_BC7(&ddsHeader, &dx10Header);
		dx10Header.dxgiFormat = texHeader->type;
		break;
	default:
		printf("Unsupported texture type %u in %s\n", texHeader->type, filename_short);
		fail = 1;
		break;
	}
	if(fail)
	{
		delete[]data;
		return;
	}
	*/

	//Define misc DDS header data
	ddsHeader.dwHeight = texInfo.height;
	ddsHeader.dwMipMapCount = texInfo.mipCount;
	ddsHeader.dwPitchOrLinearSize = mipHeader->pitch;

	//Some TEX files have padding. In those cases the width is the render width, and the mip header contain the real width of the image data
	int realWidth = texInfo.width;
	if(texInfo.usesPackedMips)
	{
		//For packed mips (RE9/MHWilds), we strip padding during extraction, so use the render width from the header
		realWidth = texInfo.width;
	}
	else
	{
		int bitsPerPixel = BitsPerPixelDX10Format(texInfo.type);
		if(bitsPerPixel < 0)
			printf("Warning: Could not determine bits-per-pixel count for %s. It's possible it will have the wrong width value.\n", filename_short);
		else
			realWidth = (mipHeader->imageDataSize * bitsPerPixel) / (texInfo.height * 2);
	}
	ddsHeader.dwWidth = realWidth;

	//Copy everything to DDS file
	char DDSPath[MAXPATH];
	memset(DDSPath, 0, MAXPATH);
	char filenameWithoutExtension[MAXPATH];
	strcpy(filenameWithoutExtension, filename);
	int firstDot = FirstDot(filenameWithoutExtension);
	if(firstDot != 0)
		filenameWithoutExtension[firstDot] = 0;
	sprintf(DDSPath, "%s.dds", filenameWithoutExtension);
	FILE *file;
	fopen_s(&file, DDSPath, "wb");
	if(!file)
	{
		printf("Error: Couldn't open %s for writing.\n", DDSPath);
		delete[]data;
		return;
	}
	fwrite(&ddsHeader, 1, sizeof(DDS_HEADER), file);
	if(dx10HeaderPresent)
		fwrite(&dx10Header, 1, sizeof(DDS_HEADER_DXT10), file);

	if(texInfo.usesPackedMips && texInfo.mipCount > 0)
	{
		//RE9/MHWilds packed mips format
		packedMipHeader_s *packedMips = (packedMipHeader_s *)&data[mipHeader->offsetForImageData];
		unsigned int packedMipMetaSize = texInfo.mipCount * sizeof(packedMipHeader_s);
		unsigned int payloadOffset = (unsigned int)mipHeader->offsetForImageData + packedMipMetaSize;

		//Check if first mip has gdeflate compression
		bool hasGDeflate = false;
		if(payloadOffset + 2 <= dataSize && texInfo.mipCount > 0)
		{
			unsigned int firstMipOffset = payloadOffset + packedMips[0].offset;
			if(firstMipOffset + 2 <= dataSize)
			{
				unsigned short magic = *(unsigned short *)&data[firstMipOffset];
				if(magic == 0xFB04)
					hasGDeflate = true;
			}
		}

		//Initialize gdeflate if needed
		if(hasGDeflate)
		{
			if(!GDeflate_Init() || !GDeflate_IsAvailable())
			{
				printf("Error: TEX file contains gdeflate-compressed data but libGDeflate.dll could not be loaded.\n");
				printf("Place libGDeflate.dll next to the executable.\n");
				fclose(file);
				remove(DDSPath);
				delete[]data;
				return;
			}
		}

		//Extract each mip's data from packed mip locations
		int curWidth = realWidth;
		int curHeight = texInfo.height;
		for(int i = 0; i < texInfo.mipCount; i++)
		{
			unsigned int mipDataOffset = payloadOffset + packedMips[i].offset;
			unsigned int packedSize = packedMips[i].size;

			//Calculate sizes for padding removal
			int paddedSize = CalculateMipDataSize(curWidth, curHeight, texInfo.type, true);
			int naturalSize = CalculateNaturalMipDataSize(curWidth, curHeight, texInfo.type);
			int naturalPitch = CalculateMipPitch(curWidth, texInfo.type, false);
			int paddedPitch = CalculateMipPitch(curWidth, texInfo.type, true);

			unsigned char *mipData = NULL;
			bool allocatedMipData = false;
			int mipDataLen = 0;

			//Check if this specific mip is gdeflate compressed
			if(hasGDeflate && mipDataOffset + 2 <= dataSize)
			{
				unsigned short magic = *(unsigned short *)&data[mipDataOffset];
				if(magic == 0xFB04)
				{
					//Decompress this mip
					mipData = new unsigned char[paddedSize];
					allocatedMipData = true;
					mipDataLen = paddedSize;
					if(!GDeflate_DecompressMip(&data[mipDataOffset], packedSize, mipData, paddedSize))
					{
						printf("Error: gdeflate decompression failed for mip level %d.\n", i);
						delete[] mipData;
						fclose(file);
						remove(DDSPath);
						delete[]data;
						return;
					}
				}
				else
				{
					//This mip is not compressed
					mipData = &data[mipDataOffset];
					mipDataLen = packedSize;
				}
			}
			else
			{
				//Not compressed
				mipData = &data[mipDataOffset];
				mipDataLen = packedSize;
			}

			//Strip padding and write to DDS
			if(naturalPitch < paddedPitch)
			{
				//Strip row padding
				int rows;
				if(IsBlockCompressed(texInfo.type))
				{
					rows = (curHeight + 3) / 4;
					if(rows < 1) rows = 1;
				}
				else
				{
					rows = curHeight;
					if(rows < 1) rows = 1;
				}

				for(int row = 0; row < rows; row++)
				{
					int srcOffset = row * paddedPitch;
					if(srcOffset + naturalPitch <= mipDataLen)
						fwrite(&mipData[srcOffset], 1, naturalPitch, file);
				}
			}
			else
			{
				//No padding to strip, copy directly
				int writeSize = naturalSize;
				if(writeSize > mipDataLen) writeSize = mipDataLen;
				fwrite(mipData, 1, writeSize, file);
			}

			if(allocatedMipData)
				delete[] mipData;

			curWidth /= 2;
			curHeight /= 2;
			if(curWidth < 1) curWidth = 1;
			if(curHeight < 1) curHeight = 1;
		}

		if(hasGDeflate)
			printf("GDeflate decompressed successfully.\n");
	}
	else
	{
		//Standard format: copy from first mip offset to end of file
		fwrite(&data[mipHeader->offsetForImageData], 1, dataSize - mipHeader->offsetForImageData, file);
	}
	fclose(file);
	char *resString;
	if(texInfo.flags & TEXFLAGS_LOWRES) resString = "low-res texture";
	else resString = "high-res texture";
	printf("TEX info: %s, %ux%u, mipcount=%u (%s)", dx10TypeNames[texInfo.type], realWidth, texInfo.height, texInfo.mipCount, resString);
	if(realWidth != texInfo.height)
		printf(" (unique render width: %u)", texInfo.height);
	printf("\n");
	if (texInfo.imgCount > 1)
		printf("Warning: The TEX just written contains multiple images. These may not be correctly converted by REtool\n");
	printf("Wrote %s\n", DDSPath);
	delete[]data;
}