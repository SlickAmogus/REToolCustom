#include <stdio.h>
#include <tchar.h>
#include "tex.h"
#include "TEXmisc.h"
#include "..\misc.h"
#include "DDSInfo.h"
#include "..\dir.h"
#include "..\retool.h"
#include "DDSmisc.h"
#include "gdeflate.h"

//Auto-match sRGB/linear variant within the same format family
static int MatchFormatVariant(int ddsFormat, int texFormat)
{
	//BC1: 71=UNORM, 72=UNORM_SRGB
	if((ddsFormat == 71 || ddsFormat == 72) && (texFormat == 71 || texFormat == 72))
		return texFormat;
	//BC2: 74=UNORM, 75=UNORM_SRGB
	if((ddsFormat == 74 || ddsFormat == 75) && (texFormat == 74 || texFormat == 75))
		return texFormat;
	//BC3: 77=UNORM, 78=UNORM_SRGB
	if((ddsFormat == 77 || ddsFormat == 78) && (texFormat == 77 || texFormat == 78))
		return texFormat;
	//BC7: 98=UNORM, 99=UNORM_SRGB
	if((ddsFormat == 98 || ddsFormat == 99) && (texFormat == 98 || texFormat == 99))
		return texFormat;
	//R8G8B8A8: 28=UNORM, 29=UNORM_SRGB
	if((ddsFormat == 28 || ddsFormat == 29) && (texFormat == 28 || texFormat == 29))
		return texFormat;
	//B8G8R8A8: 87=UNORM, 91=UNORM_SRGB
	if((ddsFormat == 87 || ddsFormat == 91) && (texFormat == 87 || texFormat == 91))
		return texFormat;
	return ddsFormat;
}

//Get base format family for comparison (returns first format in the family)
static int GetFormatFamily(int format)
{
	if(format >= 70 && format <= 72) return 70;  //BC1
	if(format >= 73 && format <= 75) return 73;  //BC2
	if(format >= 76 && format <= 78) return 76;  //BC3
	if(format >= 79 && format <= 81) return 79;  //BC4
	if(format >= 82 && format <= 84) return 82;  //BC5
	if(format >= 94 && format <= 96) return 94;  //BC6H
	if(format >= 97 && format <= 99) return 97;  //BC7
	if(format >= 27 && format <= 32) return 27;  //R8G8B8A8
	if(format >= 87 && format <= 93) return 87;  //B8G8R8A8/X8
	return format;
}

void DDStoTEX(char *filename)
{
	unsigned char *data = 0, *texData = 0;
	mipHeader_s *mipHeaders = 0;
	bool newMipHeaders = 0;
	unsigned int dataSize;
	if(!ReadFile(filename, &data, &dataSize))
	{
		printf("Failed to open %s for reading.\n", filename);
		goto finish;
	}
	DDS_HEADER *ddsHeader = (DDS_HEADER *)data;
	DDS_HEADER_DXT10 *dx10Header = 0;
	int ddsType;
	if(ddsHeader->ddspf.dwFourCC == 808540228)
	{
		dx10Header = (DDS_HEADER_DXT10 *)&data[sizeof(DDS_HEADER)];
		ddsType = dx10Header->dxgiFormat;
	}
	else
	{
		if(ddsHeader->ddspf.dwFourCC == DXT1)
			ddsType = DXGI_FORMAT_BC1_UNORM;
		else if(ddsHeader->ddspf.dwFourCC == DXT2 || ddsHeader->ddspf.dwFourCC == DXT3)
			ddsType = DXGI_FORMAT_BC2_UNORM;
		else if(ddsHeader->ddspf.dwFourCC == DXT4 || ddsHeader->ddspf.dwFourCC == DXT5)
			ddsType = DXGI_FORMAT_BC3_UNORM;
		else if(ddsHeader->ddspf.dwFourCC == ATI1)
			ddsType = DXGI_FORMAT_BC4_UNORM;
		else if(ddsHeader->ddspf.dwFourCC == ATI2)
			ddsType = DXGI_FORMAT_BC5_UNORM;
		/*
		else if (ddsheader->ddspf.dwRGBBitCount == 24) //RGB 24
			ddsType = DXGI_FORMAT_R8G8B8_TYPELESS;
			*/
		else if(ddsHeader->ddspf.dwRGBBitCount == 32) //RGBA 32
			ddsType = DXGI_FORMAT_R8G8B8A8_TYPELESS;
		else
		{
			printf("Unknown image compression format in %s.\n", filename);
			goto finish;
		}
	}

	/*
	if (dx10Header == 0)
	{
		printf("Missing DX10 header in DDS file %s\n", filename);
		delete[]data;
		return;
	}
	*/

	//Filename without extension
	char filenameWithoutExtension[MAXPATH];
	strcpy(filenameWithoutExtension, filename);
	int firstDot = FirstDot(filenameWithoutExtension);
	if(firstDot != 0)
		filenameWithoutExtension[firstDot] = 0;

	//Path
	char pathOnly[MAXPATH];
	strcpy(pathOnly, filename);
	int lastSlash = LastSlash(filenameWithoutExtension);
	pathOnly[lastSlash] = 0;

	//Filename without extension or path
	char filenameWithoutExtensionAndPath[MAXPATH];
	filenameWithoutExtensionAndPath[0] = 0;
	if((filenameWithoutExtension[lastSlash] == '\\' || filenameWithoutExtension[lastSlash] == '/') && filenameWithoutExtension[lastSlash + 1] != 0)
		memcpy(filenameWithoutExtensionAndPath, &filenameWithoutExtension[lastSlash + 1], strlen(&filenameWithoutExtension[lastSlash + 1]) + 1);
	else
		strcpy(filenameWithoutExtensionAndPath, filenameWithoutExtension);

	//Find TEX file to update
	char searchForFile[MAXPATH];
	searchForFile[0] = 0;
	sprintf(searchForFile, "%s.tex.", filenameWithoutExtensionAndPath);
	if(pathOnly[0] == 0) //If there is no path, then set this to "." so we scan current dir (otherwise we'd be scanning the root)
	{
		pathOnly[0] = '.';
		pathOnly[1] = 0;
	}
	CreateFileQueue(pathOnly, 0, 0, 1, searchForFile);
	if(dir_alphaqueNum != 1)
	{
		if(dir_alphaqueNum == 0)
			printf("Error: Could not find any corresponding TEX file to update.");
		else
			printf("Error: Multiple matching TEX files found. Skipping update.");
		goto finish;
	}

	//We remember the TEX filename as we'll use its name when saving our new TEX
	char texPath[MAXPATH];
	strcpy(texPath, dir_que[dir_alphaque[0]].fileName);

	//Read TEX file
	unsigned int texDataSize;
	if(!ReadFile(dir_que[dir_alphaque[0]].fileName, &texData, &texDataSize))
	{
		printf("Failed to open %s for reading.\n", dir_que[dir_alphaque[0]].fileName);
		goto finish;
	}

	//Check TEX magic
	if((unsigned int &)texData[0] != TEXMAGIC)
	{
		printf("%s is a not valid TEX file.\n", dir_que[dir_alphaque[0]].fileName);
		goto finish;
	}

	//Fill in info from header
	textureInfo_s texInfo = ReadTexHeader(texData);

	//Check if this has more than one image
	if (texInfo.imgCount > 1)
		printf("Warning: This TEX has multiple images. It may not be updated correctly.\n");

	//Compare DDS to original TEX and auto-match where possible
	{
		printf("Original TEX: %s, %ux%u, %u mip(s)\n",
			(texInfo.type < 130) ? dx10TypeNames[texInfo.type] : "Unknown",
			texInfo.width, texInfo.height, texInfo.mipCount);
		printf("Input DDS:    %s, %ux%u, %u mip(s)\n",
			(ddsType < 130) ? dx10TypeNames[ddsType] : "Unknown",
			ddsHeader->dwWidth, ddsHeader->dwHeight, ddsHeader->dwMipMapCount);

		//Auto-match sRGB/linear variant to match original TEX
		int matchedType = MatchFormatVariant(ddsType, texInfo.type);
		if(matchedType != ddsType)
		{
			printf("Auto-matched format: %s -> %s (matching original TEX)\n",
				dx10TypeNames[ddsType], dx10TypeNames[matchedType]);
			ddsType = matchedType;
			if(dx10Header)
				dx10Header->dxgiFormat = matchedType;
		}

		//Warn about format family mismatch
		if(GetFormatFamily(ddsType) != GetFormatFamily(texInfo.type))
			printf("Warning: Format mismatch! Original is %s, DDS is %s. The game may not load this correctly.\n",
				dx10TypeNames[texInfo.type], dx10TypeNames[ddsType]);

		//Trim mip count if DDS has more mips than original
		if(ddsHeader->dwMipMapCount > texInfo.mipCount && texInfo.mipCount > 0)
		{
			printf("Trimming mip count: %u -> %u (matching original TEX)\n",
				ddsHeader->dwMipMapCount, texInfo.mipCount);
			ddsHeader->dwMipMapCount = texInfo.mipCount;
		}
		else if(ddsHeader->dwMipMapCount < texInfo.mipCount)
		{
			printf("Warning: DDS has fewer mips (%u) than original TEX (%u).\n",
				ddsHeader->dwMipMapCount, texInfo.mipCount);
		}

		//Warn about resolution mismatch
		if(ddsHeader->dwWidth != texInfo.width || ddsHeader->dwHeight != texInfo.height)
			printf("Warning: Resolution mismatch. Original: %ux%u, DDS: %ux%u\n",
				texInfo.width, texInfo.height, ddsHeader->dwWidth, ddsHeader->dwHeight);
	}

	//Read TEX header
	if(texInfo.usesPackedMips)
	{
		//RE9/MHWilds: always create new mip headers since we need to recalculate packed mip offsets
		if(ddsHeader->dwMipMapCount)
		{
			mipHeaders = new mipHeader_s[ddsHeader->dwMipMapCount];
			newMipHeaders = 1;
		}
	}
	else if(texInfo.mipCount && ddsHeader->dwMipMapCount == texInfo.mipCount && dx10Header && dx10Header->dxgiFormat == texInfo.type) //Keep mipheaders from TEX file
	{
		mipHeaders = (mipHeader_s *) &texData[texInfo.headerSize];
	}
	else if(ddsHeader->dwMipMapCount) //Create all-new mipheaders
	{
		mipHeaders = new mipHeader_s[ddsHeader->dwMipMapCount];
		newMipHeaders = 1;
	}

	//Update parts of the header
	int originalMipCount = texInfo.mipCount;
	int originalHeight = texInfo.height;
	int originalWidth = texInfo.width;
	int originalType = texInfo.type;
	if(dx10Header)
	{
		bool sourceBC7 = (dx10Header->dxgiFormat == DXGI_FORMAT_BC7_UNORM || dx10Header->dxgiFormat == DXGI_FORMAT_BC7_UNORM_SRGB);
		bool destBC7 = (texInfo.type == DXGI_FORMAT_BC7_UNORM || texInfo.type == DXGI_FORMAT_BC7_UNORM_SRGB);
		if(!keepBC7typeDuringDDStoTEX || sourceBC7 != destBC7) //If keepBC7typeDuringDDStoTEX is true, then we'll not change BC7 type if input DDS and output TEX are both BC7
			texInfo.type = dx10Header->dxgiFormat;
	}
	else
	{
		//No DX10 header - update type from detected DDS format
		texInfo.type = ddsType;
	}
	texInfo.mipCount = (unsigned char) ddsHeader->dwMipMapCount;
	if(!keepWidthDuringDDStoTEX) //If keepWidthDuringDDStoTEX is true then we keep original width in TEX without changing it
	{
		if(forcedWidthDuringDDStoTEX > -1) //Use user-defined width
			texInfo.width = (unsigned short) forcedWidthDuringDDStoTEX;
		else //Use width from DDS
			texInfo.width = (unsigned short) ddsHeader->dwWidth;
	}
	texInfo.height = (unsigned short) ddsHeader->dwHeight;

	//Update mip headers
	if(newMipHeaders && mipHeaders)
	{
		int pixelBitSize = BitsPerPixelDX10Format(texInfo.type);
		if(pixelBitSize == -1)
		{
			printf("Error: Could not determine pixel bit depth for %s due to invalid texture type.\n", dir_que[dir_alphaque[0]].fileName);
			goto finish;
		}

		if(texInfo.usesPackedMips)
		{
			//RE9/MHWilds: calculate padded sizes for regular mip headers
			int curWidth = ddsHeader->dwWidth;
			int curHeight = ddsHeader->dwHeight;
			//Regular mip offsets start at: header + mip headers area (packed mip meta is at mip[0].offset)
			int packedMipMetaStart = texInfo.headerSize + (sizeof(mipHeader_s) * ddsHeader->dwMipMapCount);
			int curVirtualOffset = packedMipMetaStart;
			for(int i = 0; i < texInfo.mipCount; i++)
			{
				int paddedSize = CalculateMipDataSize(curWidth, curHeight, texInfo.type, true);
				int paddedPitch = CalculateMipPitch(curWidth, texInfo.type, true);
				mipHeaders[i].offsetForImageData = curVirtualOffset;
				mipHeaders[i].pitch = paddedPitch;
				mipHeaders[i].imageDataSize = paddedSize;
				curVirtualOffset += paddedSize;
				curWidth /= 2;
				curHeight /= 2;
				if(curWidth < 1) curWidth = 1;
				if(curHeight < 1) curHeight = 1;
			}
		}
		else
		{
			//Standard format: original mip header calculation
			int curWidth = ddsHeader->dwWidth;
			int curHeight = ddsHeader->dwHeight;
			int curOffset = texInfo.headerSize + (sizeof(mipHeader_s) * ddsHeader->dwMipMapCount);
			for(int i = 0; i < texInfo.mipCount; i++)
			{
				mipHeaders[i].imageDataSize = (curWidth * curHeight * pixelBitSize) / 8;
				mipHeaders[i].offsetForImageData = curOffset;
				if(pixelBitSize == 32) //Pixel size * width (we assume this is RGBA)
					mipHeaders[i].pitch = curWidth * 4;
				else if(pixelBitSize == 24) //Pixel size * width (we assume this is RGB)
					mipHeaders[i].pitch = curWidth * 3;
				else //Pixel size + 4x4 grid * width (aka block compression)
					mipHeaders[i].pitch = (4 * curWidth * pixelBitSize) / 8;
				curWidth /= 2;
				curHeight /= 2;
				curOffset += mipHeaders[i].imageDataSize;

				//For block compression, we should behave as if either axis never go below 4 (since the smallest chunk of image data can't be smaller than 4x4)
				if(pixelBitSize != 24 && pixelBitSize != 32)
				{
					if(curWidth < 4)
						curWidth = 4;
					if(curHeight < 4)
						curHeight = 4;
				}
			}
		}
	}

	//Open TEX file for writing
	FILE *file;
	fopen_s(&file, texPath, "wb");
	if(!file)
	{
		printf("Error: Couldn't open %s for writing.\n", texPath);
		goto finish;
	}

	unsigned int totalDDSHeaderSize;
	if(dx10Header)
		totalDDSHeaderSize = sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10);
	else
		totalDDSHeaderSize = sizeof(DDS_HEADER);

	//Write TEX file. Starting with header
	if(TexVersion(texInfo.extension) == TEXVERSION_RE7)
	{
		texHeader_s *texHeader = (texHeader_s *)texData;
		texHeader->width = texInfo.width;
		texHeader->height = texInfo.height;
		texHeader->mipCount = texInfo.mipCount;
		texHeader->flags = texInfo.flags;
		texHeader->type = texInfo.type;
		fwrite(texHeader, sizeof(texHeader_s), 1, file);
	}
	else
	{
		texHeader_v30_s *texHeader = (texHeader_v30_s *)texData;
		texHeader->width = texInfo.width;
		texHeader->height = texInfo.height;
		//TODO: Print warning if mipcount is more than 256? That can't be contained in one byte
		texHeader->mipCount = texInfo.mipCount * 16;
		texHeader->flags = texInfo.flags;
		texHeader->type = texInfo.type;
		fwrite(texHeader, sizeof(texHeader_v30_s), 1, file);
	}
	if(texInfo.mipCount)
		fwrite(mipHeaders, sizeof(mipHeader_s), texInfo.mipCount, file);

	if(texInfo.usesPackedMips && texInfo.mipCount > 0)
	{
		//RE9/MHWilds: build padded mip data, compress with gdeflate, then write
		unsigned char *ddsImageData = &data[totalDDSHeaderSize];
		unsigned int ddsImageDataSize = dataSize - totalDDSHeaderSize;

		//Initialize gdeflate compression
		bool useGDeflate = false;
		if(GDeflate_Init())
		{
			useGDeflate = true;
		}
		else
		{
			printf("Error: RE9/MHWilds TEX files require gdeflate compression.\n");
			printf("Place libGDeflate.dll next to the executable and try again.\n");
			fclose(file);
			goto finish;
		}

		//Calculate padded and natural sizes for each mip
		int *paddedSizes = new int[texInfo.mipCount];
		int *naturalSizes = new int[texInfo.mipCount];
		{
			int w = ddsHeader->dwWidth;
			int h = ddsHeader->dwHeight;
			for(int i = 0; i < texInfo.mipCount; i++)
			{
				paddedSizes[i] = CalculateMipDataSize(w, h, texInfo.type, true);
				naturalSizes[i] = CalculateNaturalMipDataSize(w, h, texInfo.type);
				w /= 2; if(w < 1) w = 1;
				h /= 2; if(h < 1) h = 1;
			}
		}

		//Build padded mip data in memory for each mip level
		unsigned char **paddedMipData = new unsigned char*[texInfo.mipCount];
		{
			int curWidth = ddsHeader->dwWidth;
			int curHeight = ddsHeader->dwHeight;
			unsigned int ddsDataOffset = 0;

			for(int i = 0; i < texInfo.mipCount; i++)
			{
				paddedMipData[i] = new unsigned char[paddedSizes[i]];
				memset(paddedMipData[i], 0, paddedSizes[i]);

				int naturalPitch = CalculateMipPitch(curWidth, texInfo.type, false);
				int paddedPitch = CalculateMipPitch(curWidth, texInfo.type, true);
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

				if(naturalPitch < paddedPitch)
				{
					//Copy with row padding
					for(int row = 0; row < rows; row++)
					{
						unsigned int srcOffset = ddsDataOffset + row * naturalPitch;
						if(srcOffset + naturalPitch <= ddsImageDataSize)
							memcpy(&paddedMipData[i][row * paddedPitch], &ddsImageData[srcOffset], naturalPitch);
						//Padding bytes are already zero from memset
					}
				}
				else
				{
					//No padding needed, copy directly
					int copySize = naturalSizes[i];
					if(ddsDataOffset + copySize > ddsImageDataSize)
						copySize = (int)(ddsImageDataSize - ddsDataOffset);
					if(copySize > 0)
						memcpy(paddedMipData[i], &ddsImageData[ddsDataOffset], copySize);
				}

				ddsDataOffset += naturalSizes[i];
				curWidth /= 2; if(curWidth < 1) curWidth = 1;
				curHeight /= 2; if(curHeight < 1) curHeight = 1;
			}
		}

		//Compress each mip with gdeflate (fallback to uncompressed for mips where compression fails)
		unsigned char **compressedMipData = new unsigned char*[texInfo.mipCount];
		int *compressedMipSizes = new int[texInfo.mipCount];
		bool *mipIsCompressed = new bool[texInfo.mipCount];
		int totalUncompressed = 0, totalCompressed = 0;

		for(int i = 0; i < texInfo.mipCount; i++)
		{
			compressedMipData[i] = GDeflate_CompressMip(paddedMipData[i], paddedSizes[i], &compressedMipSizes[i]);
			if(compressedMipData[i])
			{
				mipIsCompressed[i] = true;
			}
			else
			{
				//Fallback: store this mip uncompressed (game handles per-mip compression detection)
				compressedMipData[i] = NULL; //Will use paddedMipData directly
				compressedMipSizes[i] = paddedSizes[i];
				mipIsCompressed[i] = false;
			}
			totalUncompressed += paddedSizes[i];
			totalCompressed += compressedMipSizes[i];
		}

		//Write packed mip headers (compressed sizes and cumulative offsets)
		unsigned int packedDataOffset = 0;
		for(int i = 0; i < texInfo.mipCount; i++)
		{
			packedMipHeader_s pmh;
			pmh.size = compressedMipSizes[i];
			pmh.offset = packedDataOffset;
			fwrite(&pmh, sizeof(packedMipHeader_s), 1, file);
			packedDataOffset += compressedMipSizes[i];
		}

		//Write mip data (compressed or uncompressed fallback per mip)
		for(int i = 0; i < texInfo.mipCount; i++)
		{
			if(mipIsCompressed[i])
				fwrite(compressedMipData[i], 1, compressedMipSizes[i], file);
			else
				fwrite(paddedMipData[i], 1, paddedSizes[i], file);
		}

		printf("GDeflate compressed: %d -> %d bytes (%.1f%% ratio)\n",
			totalUncompressed, totalCompressed, totalCompressed * 100.0 / totalUncompressed);

		//Cleanup
		for(int i = 0; i < texInfo.mipCount; i++)
		{
			delete[] paddedMipData[i];
			if(compressedMipData[i])
				delete[] compressedMipData[i];
		}
		delete[] paddedMipData;
		delete[] compressedMipData;
		delete[] compressedMipSizes;
		delete[] mipIsCompressed;
		delete[] paddedSizes;
		delete[] naturalSizes;
	}
	else
	{
		//Standard format: copy DDS image data directly
		fwrite(&data[totalDDSHeaderSize], 1, dataSize - totalDDSHeaderSize, file);
	}
	fclose(file);
	printf("Updated %s with DDS image data.\n", texPath);

	/*
	//Various warnings if there is a mismatch in data
	if(texHeader.height != ddsHeader->dwHeight || texHeader.width != ddsHeader->dwWidth)
		printf("Warning: Resolution mismatch (it should be: %ux%u)\n", texHeader.width, texHeader.height);
	if (texHeader.mipCount < ddsHeader->dwMipMapCount)
		printf("Warning: Too many mipmaps (the count should be: %u)\n", texHeader.mipCount);
	if (texHeader.mipCount > ddsHeader->dwMipMapCount)
		printf("Warning: Too few mipmaps (the count should be: %u)\n", texHeader.mipCount);
		*/

	//Messages if we updated size or type
	{
		if(keepWidthDuringDDStoTEX)
		{
			if(ddsHeader->dwWidth == texInfo.width)
				printf("Width of %u kept from original TEX which matches the width of the image data.", texInfo.width);
			else
				printf("Width of %u kept from original TEX which is different than the width of %u used for the image data.", texInfo.width, ddsHeader->dwWidth);
		}
		else if(forcedWidthDuringDDStoTEX != -1)
		{
			if(forcedWidthDuringDDStoTEX == texInfo.width && forcedWidthDuringDDStoTEX == ddsHeader->dwWidth)
				printf("Wrote the user-defined width %i kept to the TEX header which matches the width in the original TEX and image data.", forcedWidthDuringDDStoTEX);
			else if(forcedWidthDuringDDStoTEX == texInfo.width)
				printf("Wrote the user-defined width %i kept to the TEX header which matches the width in the original TEX.", forcedWidthDuringDDStoTEX);
			else if(forcedWidthDuringDDStoTEX == ddsHeader->dwWidth)
				printf("Wrote the user-defined width %i kept to the TEX header which matches the width in the image data.", forcedWidthDuringDDStoTEX);
			else
				printf("Wrote the user-defined width %i kept to the TEX header which is different than both the original TEX header and image data.", forcedWidthDuringDDStoTEX);
		}
		else
		{
			if(originalHeight != texInfo.height || originalWidth != texInfo.width)
				printf("Resolution changed from %ux%u to %ux%u\n", originalWidth, originalHeight, texInfo.width, texInfo.height);
		}
	}
	
	if(originalMipCount != texInfo.mipCount)
		printf("Mip count changed from %u to %u\n", originalMipCount, texInfo.mipCount);
	if(originalType != texInfo.type)
		printf("Type changed from %u to %u\n", originalType, texInfo.type);

	//Free allocated memory
finish:
	if(data)
		delete[]data;
	if(texData)
		delete[]texData;
	if(newMipHeaders && mipHeaders)
		delete[]mipHeaders;
}