#pragma once

char *dx10TypeNames[];
extern int divideTexBy;

int TexVersion(int extension);
textureInfo_s ReadTexHeader(unsigned char *data);
void OutputTEXInformation(char *filename);
void ReduceTEXSize(char *filename);