#pragma once

int BitsPerPixelDX10Format(int format);
bool IsBlockCompressed(int format);
int GetBlockSizeBytes(int format);
int CalculateMipPitch(int width, int format, bool padToMinimum);
int CalculateMipDataSize(int width, int height, int format, bool padToMinimum);
int CalculateNaturalMipDataSize(int width, int height, int format);