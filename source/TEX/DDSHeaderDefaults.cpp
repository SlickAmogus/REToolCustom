#include "DDSInfo.h"

void SetDDSHeaderDefaults_DXT5(DDS_HEADER *ddsheader)
{
	ddsheader->dwMagicWord = 542327876;
	ddsheader->dwSize = 124;
	ddsheader->dwFlags = 135175;
	ddsheader->dwHeight = 1024;
	ddsheader->dwWidth = 1024;
	ddsheader->dwPitchOrLinearSize = 0;
	ddsheader->dwDepth = 0;
	ddsheader->dwMipMapCount = 11;
	//ddsheader->dwReserved1 = 0x00419218;
	ddsheader->ddspf.dwSize = 32;
	ddsheader->ddspf.dwFlags = 4;
	ddsheader->ddspf.dwFourCC = DXT5;
	ddsheader->ddspf.dwRGBBitCount = 0;
	ddsheader->ddspf.dwRBitMask = 0;
	ddsheader->ddspf.dwGBitMask = 0;
	ddsheader->ddspf.dwBBitMask = 0;
	ddsheader->ddspf.dwABitMask = 0;
	ddsheader->dwCaps = 4198408;
	ddsheader->dwCaps2 = 0;
	ddsheader->dwCaps3 = 0;
	ddsheader->dwCaps4 = 0;
	ddsheader->dwReserved2 = 0;
}

void SetDDSHeaderDefaults_DXT1(DDS_HEADER *ddsheader)
{
	ddsheader->dwMagicWord = 542327876;
	ddsheader->dwSize = 124;
	ddsheader->dwFlags = 135175;
	ddsheader->dwHeight = 1024;
	ddsheader->dwWidth = 1024;
	ddsheader->dwPitchOrLinearSize = 0;
	ddsheader->dwDepth = 0;
	ddsheader->dwMipMapCount = 11;
	//ddsheader->dwReserved1 = 0x00419218;
	ddsheader->ddspf.dwSize = 32;
	ddsheader->ddspf.dwFlags = 4;
	ddsheader->ddspf.dwFourCC = DXT1;
	ddsheader->ddspf.dwRGBBitCount = 0;
	ddsheader->ddspf.dwRBitMask = 0;
	ddsheader->ddspf.dwGBitMask = 0;
	ddsheader->ddspf.dwBBitMask = 0;
	ddsheader->ddspf.dwABitMask = 0;
	ddsheader->dwCaps = 4096;
	ddsheader->dwCaps2 = 0;
	ddsheader->dwCaps3 = 0;
	ddsheader->dwCaps4 = 0;
	ddsheader->dwReserved2 = 0;
}

void SetDDSHeaderDefaults_RGBA(DDS_HEADER *ddsheader)
{
	ddsheader->dwMagicWord = 542327876;
	ddsheader->dwSize = 124;
	ddsheader->dwFlags = 4111;
	ddsheader->dwHeight = 32;
	ddsheader->dwWidth = 32;
	ddsheader->dwPitchOrLinearSize = 128;
	ddsheader->dwDepth = 0;
	ddsheader->dwMipMapCount = 11;
	//dds_header.dwReserved1 = 0x00419218;
	ddsheader->ddspf.dwSize = 32;
	ddsheader->ddspf.dwFlags = 65;
	ddsheader->ddspf.dwFourCC = 0;
	ddsheader->ddspf.dwRGBBitCount = 32;
	ddsheader->ddspf.dwRBitMask = 16711680;
	ddsheader->ddspf.dwGBitMask = 65280;
	ddsheader->ddspf.dwBBitMask = 255;
	ddsheader->ddspf.dwABitMask = 4278190080;
	ddsheader->dwCaps = 4096;
	ddsheader->dwCaps2 = 0;
	ddsheader->dwCaps3 = 0;
	ddsheader->dwCaps4 = 0;
	ddsheader->dwReserved2 = 0;
}

void SetDDSHeaderDefaults_RGB(DDS_HEADER *ddsheader) //TODO: Update based on a real DDS header
{
	ddsheader->dwMagicWord = 542327876;
	ddsheader->dwSize = 124;
	ddsheader->dwFlags = 4111;
	ddsheader->dwHeight = 32;
	ddsheader->dwWidth = 32;
	ddsheader->dwPitchOrLinearSize = 128;
	ddsheader->dwDepth = 0;
	ddsheader->dwMipMapCount = 11;
	//dds_header.dwReserved1 = 0x00419218;
	ddsheader->ddspf.dwSize = 32;
	ddsheader->ddspf.dwFlags = 65;
	ddsheader->ddspf.dwFourCC = 0;
	ddsheader->ddspf.dwRGBBitCount = 24;
	ddsheader->ddspf.dwRBitMask = 16711680;
	ddsheader->ddspf.dwGBitMask = 65280;
	ddsheader->ddspf.dwBBitMask = 255;
	ddsheader->ddspf.dwABitMask = 4278190080;
	ddsheader->dwCaps = 4096;
	ddsheader->dwCaps2 = 0;
	ddsheader->dwCaps3 = 0;
	ddsheader->dwCaps4 = 0;
	ddsheader->dwReserved2 = 0;
}

void SetDDSHeaderDefaults_3Dc(DDS_HEADER *ddsheader) //Aka ATI2
{
	ddsheader->dwMagicWord = 542327876;
	ddsheader->dwSize = 124;
	ddsheader->dwFlags = 659463;
	ddsheader->dwHeight = 1024;
	ddsheader->dwWidth = 512;
	ddsheader->dwPitchOrLinearSize = 524288;
	ddsheader->dwDepth = 0;
	ddsheader->dwMipMapCount = 10;
	//ddsheader->dwReserved1 = 0x001C0060;
	ddsheader->ddspf.dwSize = 32;
	ddsheader->ddspf.dwFlags = 2147483652;
	ddsheader->ddspf.dwFourCC = 843666497;
	ddsheader->ddspf.dwRGBBitCount = 1498952257;
	ddsheader->ddspf.dwRBitMask = 0;
	ddsheader->ddspf.dwGBitMask = 0;
	ddsheader->ddspf.dwBBitMask = 0;
	ddsheader->ddspf.dwABitMask = 0;
	ddsheader->dwCaps = 4198408;
	ddsheader->dwCaps2 = 0;
	ddsheader->dwCaps3 = 0;
	ddsheader->dwCaps4 = 0;
	ddsheader->dwReserved2 = 0;
}

void SetDDSHeaderDefaults_BC7(DDS_HEADER *ddsheader, DDS_HEADER_DXT10 *dx10Header)
{
	ddsheader->dwMagicWord = 542327876;
	ddsheader->dwSize = 124;
	ddsheader->dwFlags = 6;
	ddsheader->dwHeight = 1024;
	ddsheader->dwWidth = 1024;
	ddsheader->dwPitchOrLinearSize = 4096;
	ddsheader->dwDepth = 0;
	ddsheader->dwMipMapCount = 1;
	//ddsheader->dwReserved1 = 0x001C0060;
	ddsheader->ddspf.dwSize = 32;
	ddsheader->ddspf.dwFlags = 4;
	ddsheader->ddspf.dwFourCC = BC7;
	ddsheader->ddspf.dwRGBBitCount = 0;
	ddsheader->ddspf.dwRBitMask = 0;
	ddsheader->ddspf.dwGBitMask = 0;
	ddsheader->ddspf.dwBBitMask = 0;
	ddsheader->ddspf.dwABitMask = 0;
	ddsheader->dwCaps = 4096;
	ddsheader->dwCaps2 = 0;
	ddsheader->dwCaps3 = 0;
	ddsheader->dwCaps4 = 0;
	ddsheader->dwReserved2 = 0;

	dx10Header->dxgiFormat = DXGI_FORMAT_BC7_UNORM_SRGB;
	dx10Header->resourceDimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
	dx10Header->miscFlag = 0;
	dx10Header->arraySize = 1;
	dx10Header->miscFlags2 = 0;
}