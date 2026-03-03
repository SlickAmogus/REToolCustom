#pragma once

bool ReadHashList(char *filenameHashList);
bool OutputHashList(char *filenameHashList);
bool CombineHashLists(char *filenameHashList, hashList_s *altHashList, unsigned int altHashListSize);
char *ReturnFilenameFromHashList(unsigned long long hash_lowercase, unsigned long long hash_highercase);