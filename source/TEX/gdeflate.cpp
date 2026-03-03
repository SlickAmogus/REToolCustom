#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "gdeflate.h"

#define GDEFLATE_TILE_SIZE 0x10000   //64KB per tile
#define GDEFLATE_MAX_TILES 0xFFFF
#define GDEFLATE_ID 4
#define GDEFLATE_MAGIC 0xFB          //0xFF ^ 0x04

#pragma pack(push, 1)
struct TileStreamHeader
{
	unsigned char id;            //TileStreamCompressor enum (4 = GDeflate)
	unsigned char magic;         //0xFF ^ id (validation)
	unsigned short numTiles;     //Number of 64KB tiles
	unsigned int flags;          //Bits 0-1: TileSizeIndex, Bits 2-19: LastTileSize
};
#pragma pack(pop)

//Native GDeflatePage struct - must match the native library's layout
struct GDeflatePage
{
	void *data;   //Pointer to data buffer
	int size;     //Size of data
};

//Function pointer types for native DLL functions
typedef void * (__cdecl *pfn_alloc_compressor)(int level);
typedef size_t (__cdecl *pfn_compress)(void *compressor, const void *src, size_t srcSize, GDeflatePage *pages, size_t numPages);
typedef void (__cdecl *pfn_free_compressor)(void *compressor);
typedef void * (__cdecl *pfn_alloc_decompressor)();
typedef int (__cdecl *pfn_decompress)(void *decompressor, GDeflatePage *pages, size_t numPages, void *dst, size_t dstSize, size_t *bytesOut);
typedef void (__cdecl *pfn_free_decompressor)(void *decompressor);

static HMODULE gDeflateDLL = NULL;
static pfn_alloc_compressor fn_alloc_compressor = NULL;
static pfn_compress fn_compress = NULL;
static pfn_free_compressor fn_free_compressor = NULL;
static pfn_alloc_decompressor fn_alloc_decompressor = NULL;
static pfn_decompress fn_decompress = NULL;
static pfn_free_decompressor fn_free_decompressor = NULL;
static bool gDeflateInitialized = false;

bool GDeflate_Init()
{
	if(gDeflateInitialized)
		return gDeflateDLL != NULL;
	gDeflateInitialized = true;

	//Try loading from same directory as exe
	gDeflateDLL = LoadLibraryA("libGDeflate.dll");

	if(!gDeflateDLL)
	{
		//Try from exe directory explicitly
		char path[MAX_PATH];
		GetModuleFileNameA(NULL, path, MAX_PATH);
		char *lastSlash = strrchr(path, '\\');
		if(lastSlash)
		{
			lastSlash[1] = 0;
			strcat_s(path, MAX_PATH, "libGDeflate.dll");
			gDeflateDLL = LoadLibraryA(path);
		}
	}

	if(!gDeflateDLL)
		return false;

	fn_alloc_compressor = (pfn_alloc_compressor)GetProcAddress(gDeflateDLL, "libdeflate_alloc_gdeflate_compressor");
	fn_compress = (pfn_compress)GetProcAddress(gDeflateDLL, "libdeflate_gdeflate_compress");
	fn_free_compressor = (pfn_free_compressor)GetProcAddress(gDeflateDLL, "libdeflate_free_gdeflate_compressor");
	fn_alloc_decompressor = (pfn_alloc_decompressor)GetProcAddress(gDeflateDLL, "libdeflate_alloc_gdeflate_decompressor");
	fn_decompress = (pfn_decompress)GetProcAddress(gDeflateDLL, "libdeflate_gdeflate_decompress");
	fn_free_decompressor = (pfn_free_decompressor)GetProcAddress(gDeflateDLL, "libdeflate_free_gdeflate_decompressor");

	if(!fn_alloc_compressor || !fn_compress || !fn_free_compressor ||
	   !fn_alloc_decompressor || !fn_decompress || !fn_free_decompressor)
	{
		printf("Error: libGDeflate.dll loaded but missing required functions.\n");
		FreeLibrary(gDeflateDLL);
		gDeflateDLL = NULL;
		return false;
	}

	return true;
}

void GDeflate_Shutdown()
{
	if(gDeflateDLL)
	{
		FreeLibrary(gDeflateDLL);
		gDeflateDLL = NULL;
	}
	gDeflateInitialized = false;
	fn_alloc_compressor = NULL;
	fn_compress = NULL;
	fn_free_compressor = NULL;
	fn_alloc_decompressor = NULL;
	fn_decompress = NULL;
	fn_free_decompressor = NULL;
}

bool GDeflate_IsAvailable()
{
	return gDeflateDLL != NULL;
}

unsigned char *GDeflate_CompressMip(const unsigned char *data, int dataSize, int *compressedStreamSize, int level)
{
	if(!GDeflate_IsAvailable() || !data || dataSize <= 0 || !compressedStreamSize)
		return NULL;

	if(level < 1) level = 1;
	if(level > 12) level = 12;

	//Build tile stream header
	TileStreamHeader tileHeader;
	memset(&tileHeader, 0, sizeof(tileHeader));
	tileHeader.id = GDEFLATE_ID;
	tileHeader.magic = GDEFLATE_MAGIC;
	tileHeader.numTiles = (unsigned short)((dataSize + GDEFLATE_TILE_SIZE - 1) / GDEFLATE_TILE_SIZE);
	if(tileHeader.numTiles < 1) tileHeader.numTiles = 1;
	if(tileHeader.numTiles > GDEFLATE_MAX_TILES) tileHeader.numTiles = GDEFLATE_MAX_TILES;

	int lastTileSize = dataSize % GDEFLATE_TILE_SIZE;
	//Flags: bits 0-1 = TileSizeIndex (1), bits 2-19 = LastTileSize
	tileHeader.flags = 1 | ((lastTileSize & 0x3FFFF) << 2);

	int headerSize = sizeof(TileStreamHeader) + tileHeader.numTiles * sizeof(int);

	//Allocate output buffer (worst case: header + 2x input data)
	int maxOutputSize = headerSize + dataSize * 2;
	unsigned char *output = new unsigned char[maxOutputSize];
	memset(output, 0, maxOutputSize);

	//Write tile stream header
	memcpy(output, &tileHeader, sizeof(TileStreamHeader));
	int *tileOffsets = (int *)&output[sizeof(TileStreamHeader)];

	//Compress each tile
	void *compressor = fn_alloc_compressor(level);
	if(!compressor)
	{
		delete[] output;
		return NULL;
	}

	int compressedOffset = 0;
	int uncompressedOffset = 0;
	GDeflatePage page;
	bool success = true;

	for(int tileIndex = 0; tileIndex < tileHeader.numTiles; tileIndex++)
	{
		int tileInputSize = dataSize - uncompressedOffset;
		if(tileInputSize > GDEFLATE_TILE_SIZE)
			tileInputSize = GDEFLATE_TILE_SIZE;

		int outputBufRemaining = maxOutputSize - (headerSize + compressedOffset);
		if(outputBufRemaining <= 0)
		{
			success = false;
			break;
		}

		page.data = &output[headerSize + compressedOffset];
		page.size = outputBufRemaining;

		size_t compressedSize = fn_compress(compressor,
			(const void *)&data[uncompressedOffset], (size_t)tileInputSize,
			&page, 1);

		if(compressedSize == 0)
		{
			success = false;
			break;
		}

		uncompressedOffset += tileInputSize;
		compressedOffset += (int)compressedSize;

		if(tileIndex < tileHeader.numTiles - 1)
		{
			tileOffsets[tileIndex + 1] = compressedOffset;
		}
		else
		{
			//Last tile: store its compressed size in tileOffsets[0]
			tileOffsets[0] = (int)compressedSize;
		}
	}

	fn_free_compressor(compressor);

	if(!success)
	{
		delete[] output;
		return NULL;
	}

	//Copy to exact-sized buffer
	*compressedStreamSize = headerSize + compressedOffset;
	unsigned char *result = new unsigned char[*compressedStreamSize];
	memcpy(result, output, *compressedStreamSize);
	delete[] output;

	return result;
}

bool GDeflate_DecompressMip(const unsigned char *compressedStream, int compressedStreamSize, unsigned char *outputData, int outputSize)
{
	if(!GDeflate_IsAvailable() || !compressedStream || compressedStreamSize < (int)sizeof(TileStreamHeader) || !outputData || outputSize <= 0)
		return false;

	//Read and validate tile stream header
	const TileStreamHeader *tileHeader = (const TileStreamHeader *)compressedStream;
	if(tileHeader->id != GDEFLATE_ID || tileHeader->magic != GDEFLATE_MAGIC)
		return false;
	if(tileHeader->numTiles == 0)
		return false;

	int headerSize = sizeof(TileStreamHeader) + tileHeader->numTiles * sizeof(int);
	if(compressedStreamSize < headerSize)
		return false;

	const int *tileOffsets = (const int *)&compressedStream[sizeof(TileStreamHeader)];

	//Build page array for all tiles
	GDeflatePage *pages = new GDeflatePage[tileHeader->numTiles];

	for(int i = 0; i < tileHeader->numTiles; i++)
	{
		int tileOffset = (i > 0) ? tileOffsets[i] : 0;
		int tileSize;

		if(i < tileHeader->numTiles - 1)
			tileSize = tileOffsets[i + 1] - tileOffset;
		else
			tileSize = tileOffsets[0]; //Last tile size stored in [0]

		pages[i].data = (void *)&compressedStream[headerSize + tileOffset];
		pages[i].size = tileSize;
	}

	//Decompress all tiles at once
	void *decompressor = fn_alloc_decompressor();
	if(!decompressor)
	{
		delete[] pages;
		return false;
	}

	memset(outputData, 0, outputSize);
	size_t bytesOut = 0;
	int result = fn_decompress(decompressor, pages, (size_t)tileHeader->numTiles,
		outputData, (size_t)outputSize, &bytesOut);

	fn_free_decompressor(decompressor);
	delete[] pages;

	return result == 0;
}
