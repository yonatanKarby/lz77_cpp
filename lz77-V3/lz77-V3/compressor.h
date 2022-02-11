#pragma once
#include <iostream>

struct compressedBuffer
{
	int length;
	unsigned char* buffer;
};

struct compressedRLE
{
	int position;
	int length;
	unsigned char tail;
	compressedRLE()
	{
		length = -1;
		position = 0;
	}
};

class Icompressor
{
public:
	virtual compressedBuffer compress(unsigned char* unCompressedData, int length) = 0;
	virtual compressedBuffer deCompress(unsigned char* compressed, int length) = 0;
};

