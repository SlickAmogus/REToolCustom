#pragma once

//GDeflate compression/decompression via libGDeflate.dll (dynamically loaded)
//Used for RE9/MHWilds TEX files which require gdeflate-compressed mip data

bool GDeflate_Init();      //Load DLL and resolve functions. Returns true if successful.
void GDeflate_Shutdown();  //Unload DLL
bool GDeflate_IsAvailable(); //Returns true if DLL is loaded and functions resolved

//Compress a single mip level's data into gdeflate tile stream format.
//Returns allocated buffer (caller must delete[]), or NULL on failure.
//compressedStreamSize receives the total size of the returned stream.
//level: compression level 1-12 (default 12 = max compression)
unsigned char *GDeflate_CompressMip(const unsigned char *data, int dataSize, int *compressedStreamSize, int level = 12);

//Decompress a single mip level's gdeflate tile stream.
//compressedStream points to the start of the TileStreamHeader.
//outputData must be pre-allocated with outputSize bytes.
//Returns true on success.
bool GDeflate_DecompressMip(const unsigned char *compressedStream, int compressedStreamSize, unsigned char *outputData, int outputSize);
