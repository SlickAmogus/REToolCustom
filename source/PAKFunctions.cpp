#include <stdio.h>
//#include <direct.h>
#include "pakformat.h"
//#include "libdeflate\libdeflate.h"
//#include "re7pak.h"
//#include "dir.h"
//#include "misc.h"
#include <string.h>
#include <ctype.h>
#include "murmurhash3\murmurhash3.h"
#include "Hash\sha3.h"
#include "misc.h"

extern "C" void _transform_crypto_key(unsigned char* output, unsigned char* input128); //From mhr_pak_decrypt_algo.asm

//unsigned char pakKey[32] = {0x66, 0xBF, 0x3E, 0xAA, 0xE9, 0xB0, 0x82, 0x86, 0xE2, 0xDE, 0x8F, 0x9D, 0x21, 0x99, 0x3E, 0x78, 0xC7, 0xAE, 0xF6, 0xDF, 0x06, 0x93, 0x47, 0x94, 0x2E, 0x1D, 0x0F, 0xCA, 0xAC, 0x81, 0x7A, 0x67}; //Key used for Monster Hunter Rise PAKs

/*
* MHR seems to do this in regards to PAK encryption:
* - Load 128-byte hash: C8399C72D1A39B08A0EB1867B9BF051344A230786A74DE6FB6F37B8B05621F1529BF4365A8A2D106ACCBC6F9FD89BCCE87E0CB2891B837A10805F463C17F13416DDB6B74F94326185ABB0FBBA95816E34C8CE7F477D327368116087E11FDB4F9096A314A30A1B16F4C327CA98ADF1CE88606EADC228BDB4E95127042952E3798)
* - Generate 32-byte key from the hash (32 bytes: 66BF3EAAE9B08286E2DE8F9D21993E78C7AEF6DF069347942E1D0FCAAC817A67)
* - The key is also the SHA3-256 checksum of the decrypted TOC data
* - After decrypting the data, it generates a new checksum to verify it matches the key (aka checksum)
* - It's not known how the 128-byte hash is generated. It's probably generated from only the checksum
* - This is a disassembled version of the function that attains the 32-byte checksum from the 128-byte hash: https://gist.github.com/Andoryuuta/8c9d4f31c036e0b8bb69d3e8834ec6d9
* - This is a disassembled version of the function that decrypts TOC: https://cdn.discordapp.com/attachments/932430563099439184/932436274135068682/unknown.png
*/

bool Crypt(unsigned char *data, unsigned long long dataSize, unsigned long long keyIndex, unsigned char *keyHash, unsigned int entryCount, unsigned long long fileSize) //Decrypt or encrypt data using the MHR encryption algorithm and key
{
	//Attain key from key hash thing
	unsigned char pakKey[32];
	_transform_crypto_key(pakKey, keyHash);
	//unsigned int key = pow(keyHash, CRYPTO_HASH_KEY_0) % CRYPTO_HASH_KEY_1;

	for(unsigned long long i = 0; i < dataSize; i++, keyIndex++)
	{
		unsigned char b0 = pakKey[keyIndex % 32];
		unsigned char b1 = pakKey[keyIndex % 29];
		unsigned char xor_byte = (unsigned char) keyIndex + ((b0 * b1) & 0xFF);
		data[i] ^= xor_byte;
	}

	//Dumb-ish way of doing an integrity check. Should be reliable unless we ever process a PAK file with only a tiny amount of files, then this code might fail to detect the decryption as invalid.
	if(dataSize)
	{
		pakEntry_s *pakEntries = (pakEntry_s *) data;
		for(unsigned int i = 0; i < entryCount; i++)
		{
			if(pakEntries[i].offset + pakEntries[i].compressedSize > fileSize)
				return 0;
		}
	}

	return 1;
}

bool LoadPAKFile(char *pakPath, pakHeader_s *pakHeader, pakEntry_s **pakEntries, unsigned long long *pakSize)
{
	//Open file
	FILE *pakFile;
	fopen_s(&pakFile, pakPath, "rb");
	if(!pakFile)
	{
		printf("Error: Failed to open %s for reading.\n", pakPath);
		return 0;
	}

	if(pakSize) //Acquire file size
	{
		fpos_t fpos;
		_fseeki64(pakFile, 0, SEEK_END);
		fgetpos(pakFile, &fpos);
		_fseeki64(pakFile, 0, SEEK_SET);
		*pakSize = fpos;
	}

	//Read header
	fread(pakHeader, sizeof(pakHeader_s), 1, pakFile);
	if(pakHeader->magic != PAKMAGIC)
	{
		printf("Error: PAK magic identifier is not as expected in %s.\n", pakPath);
		return 0;
	}

	//Read entry list
	*pakEntries = new pakEntry_s[pakHeader->entryCount];
	fread(*pakEntries, sizeof(pakEntry_s), pakHeader->entryCount, pakFile);

	//Debug: Write header and entry list to file
	if(0)
	{
		FILE *newFile = 0;
		char newPath[260];
		sprintf_s(newPath, 260, "%s.tiny", pakPath);
		fopen_s(&newFile, newPath, "wb");
		if(newFile)
		{
			fwrite(pakHeader, sizeof(pakHeader_s), 1, newFile);
			fwrite(*pakEntries, sizeof(pakEntry_s), pakHeader->entryCount, newFile);
			fclose(newFile);
			printf("Wrote output.pak");
		}
	}

	//Decrypt
	if(pakHeader->flags & PAKFLAGS_ENCRYPTED)
	{
		//Read 128-byte hash encryption key thingie
		unsigned char keyHash[128];
		fread(keyHash, 1, 128, pakFile);

		printf("PAK file is encrypted.\n");

		if(!Crypt((unsigned char *) *pakEntries, sizeof(pakEntry_s) * pakHeader->entryCount, 0, keyHash, pakHeader->entryCount, *pakSize))
		{
			printf("Error: Failed to decrypt PAK TOC in %s.\n", pakPath);
			delete[]*pakEntries;
			fclose(pakFile);
			return 0;
		}
	}
	fclose(pakFile);

	return 1;
}

unsigned int ConvertFilePathToMurmurHash(char *filePath, bool lower, bool changeCase, bool keepNullTerminator)
{
	//We'll need a UTF-16 variant of the filepath for this
	unsigned int wideLength = (unsigned int)((strlen(filePath) + 1) * 2);
	char *wideString = new char[wideLength];
	memset(wideString, 0, wideLength);
	unsigned int oldPos = 0, newPos = 0;
	while (1)
	{
		if (filePath[oldPos] == '\\') filePath[oldPos] = '/';
		if (changeCase)
		{
			if (lower) wideString[newPos] = tolower(filePath[oldPos]);
			else wideString[newPos] = toupper(filePath[oldPos]);
		}
		else
			wideString[newPos] = filePath[oldPos];
		wideString[newPos + 1] = 0;
		if (filePath[oldPos] == 0)
			break;
		newPos += 2;
		oldPos++;
	}
	unsigned int murmurHash = 0;
	if(keepNullTerminator == 0)
		MurmurHash3_x86_32(wideString, wideLength - 2, -1, &murmurHash);
	else
		MurmurHash3_x86_32(wideString, wideLength, -1, &murmurHash);
	delete[]wideString;
	return murmurHash;
}

unsigned int ConvertAnsiStringMurmurHash(char *str, bool keepNullTerminator)
{
	unsigned int murmurHash = 0;
	if (keepNullTerminator == 0)
		MurmurHash3_x86_32(str, strlen(str), -1, &murmurHash);
	else
		MurmurHash3_x86_32(str, strlen(str) + 1, -1, &murmurHash);
	return murmurHash;
}