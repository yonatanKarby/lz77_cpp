#include "compressor.h"
#include <iostream>
#include <vector>

class compressor : Icompressor
{
public:
	virtual compressedBuffer compress(unsigned char* unCompressedData, int length)
	{
		_inBuffer = unCompressedData;
		_inBufferLength = length;

		initCompression();
		return compressionLoop();
	}
	virtual compressedBuffer decompress(unsigned char* compressedData, int length)
	{
		_inBuffer = compressedData;
		_inBufferLength = length;

		initCompression();
		return decompressLoop();
	}

private:

	char d = 0x32;
	int _position;
	#define _dictonaryLength 255//max size for the RLE position "unsiged char"
	#define _maxRLELength  255 //max size for the RLE length "unsiged char"

	unsigned char* _inBuffer;
	int _inBufferLength;

	void initCompression()
	{
		_position = 0;
	}
	//compress
	compressedBuffer compressionLoop()
	{
		std::vector<compressedRLE> compressed;
		while (_position < _inBufferLength)
		{
			compressedRLE RLE = compressedCycle();
			_position += (RLE.length + 1);
			compressed.push_back(RLE);
		}
		return writeCompressedRLE(compressed);
	}

	compressedRLE RLEForPosition
	(int startIndex)
	{
		compressedRLE RLE;
		RLE.position = startIndex;
		startIndex++;//start index 0 stants for the first position on the left
		int searchPosition = _position - startIndex;
		if (searchPosition < 0) 
			return RLE;

		int maxSearch = _maxRLELength > (startIndex << 1) ? (startIndex << 1) : _maxRLELength;//this gets the max posible RLE length in this position
		
		if (searchPosition + maxSearch > _inBufferLength - 1)
			maxSearch = _inBufferLength - searchPosition;
		
		for (int i = 0; i<maxSearch; i++)
		{
			if (_inBuffer[searchPosition + i] != _inBuffer[_position + i])
			{
				RLE.length = i;
				RLE.tail = _inBuffer[_position + i];
				return RLE;
			}
		}

		RLE.length = maxSearch;
		RLE.tail = _inBuffer[searchPosition + maxSearch + 1];
		return RLE;
	}

	compressedRLE compressedCycle()
	{
		compressedRLE RLE;
		RLE.tail = _inBuffer[_position];
		for (int i = 0; i < _dictonaryLength; i++)
		{
			compressedRLE current = RLEForPosition(i);
			if (current.length > RLE.length)
				RLE = current;
		}

		if (RLE.length < 2)
		{
			RLE.length = 0;
			RLE.position = 0;
			RLE.tail = _inBuffer[_position];
		}
		return RLE;
	}

	compressedBuffer writeCompressedRLE(std::vector<compressedRLE> compressed)
	{
		int RLECount = compressed.size();
		int size = getCompressRLEWriteSize(compressed);
		unsigned char* outBuff = new unsigned char[size];
		int bitIndex = 0;
		for (int i = 0; i < RLECount; i++)
		{
			bitIndex = writeCompressedRLE(outBuff, compressed[i], bitIndex);
		}
		int bytes = bitIndex / 8;
		compressedBuffer buf;
		buf.buffer = outBuff;
		buf.length = size;
		return buf;
	} 

	compressedBuffer writeOld(std::vector<compressedRLE> compressed)
	{
		int size = compressed.size();
		int length = size * 3;
		unsigned char* buff = new unsigned char[length];

		for (int i = 0; i < length; i+=3)
		{
			int index = i / 3;
			buff[i] = compressed[index].length;
			buff[i + 1] = compressed[index].position;
			buff[i + 2] = compressed[index].tail;
		}
		compressedBuffer c;
		c.buffer = buff;
		c.length = length;
		return c;
	}

	std::vector<compressedRLE> readOld()
	{
		std::vector<compressedRLE> compressed;

		for (int i = 0; i < _inBufferLength; i+=3)
		{
			compressedRLE c;
			c.length = _inBuffer[i];
			c.position = _inBuffer[i + 1];
			c.tail = _inBuffer[i + 2];
			compressed.push_back(c);
		}
		return compressed;
	}

	int writeCompressedRLE(unsigned char* buff, compressedRLE RLE,int index)
	{
		int byteIndex = index / 8;
		int bitIndex = index % 8;
		if (RLE.length == 0)
		{
			buff[byteIndex] = changeBit(buff[byteIndex], bitIndex, 0);//write bit 0 for RLE copy too short
			index++;
			writeChar(buff, index, RLE.tail);
			index += 8;
		}
		else
		{
			buff[byteIndex] = changeBit(buff[byteIndex], bitIndex, 1);//write bit 1 for RLE copy
			index++;
			writeChar(buff, index, RLE.tail);
			index += 8;
			writeChar(buff, index, RLE.length);
			index += 8;
			writeChar(buff, index, RLE.position);
			index += 8;
		}
		return index;
	}

	int getCompressRLEWriteSize(std::vector<compressedRLE> compressed)
	{
		int bits = 0;
		int length = compressed.size();

		int fullRLESize = (8 * 3) + 1;
		int smallRLESize = 9;

		for (int i = 0; i < length; i++)
		{
			bits += compressed[i].length == 0 ? smallRLESize : fullRLESize;
		}
		float bytes = bits / 8.0;
		int bytesRounded = (bytes + 0.9);
		return bytesRounded;
	}

	//byte read and write commands
	void writeChar(unsigned char* buff, int index, unsigned char c)
	{
		for (int i = 0; i < 8; i++)
		{
			int dex = ( i + index );// the bit index in the buffer
			int byteIndex = dex / 8;//the byte index
			dex = dex % 8;//this is the index within the byte
			int bit = getBitIndex(c, i);
			buff[byteIndex] = changeBit(buff[byteIndex], dex, bit);
		}
	}

	int getBitIndex(unsigned char c, int index)
	{
		return (c >> index) & 0x01;
	}

	int getBitIndex(unsigned char* data, int index)
	{
		return (data[index / 8] >> (index % 8)) & 0x01;
	}

	unsigned char changeBit(unsigned char c, int index, int bit)
	{
		switch (bit)
		{
		case 0:
			return (c & ~(0x1 << (index)));
		case 1:
			return (c | (0x1 << (index)));
		default:
			break;
		}
		return -1;
	}

	unsigned char readByte(unsigned char* buff, int startIndexBit)
	{
		unsigned char c = ' ';
		for (int i = 0; i < 8; i++)
		{
			int newBit = getBitIndex(buff, startIndexBit + i);
			c = changeBit(c, i, newBit);
		}
		return c;
	}
	//decompress
	void uncompressCycle(compressedRLE current, std::vector<unsigned char>* unCompressed)
	{
		int startIndex = unCompressed->size() - current.position - 1;
		int runLength = current.length;
		for (int i = 0; i < runLength; i++)
		{
			int index = startIndex + i;
			if (index < 0)
				break;
			unCompressed->push_back((*unCompressed)[index]);
		}
		unCompressed->push_back(current.tail);
	}

	compressedBuffer decompressLoop()
	{
		std::vector<compressedRLE> compressed = readCompressed();
		std::vector<unsigned char> unCompressed;
		unCompressed.push_back(compressed[0].tail);
		for (int i = 1; i < compressed.size(); i++)
		{
			uncompressCycle(compressed[i], &unCompressed);
		}

		int length = unCompressed.size();
		unsigned char* buffer = new unsigned char[length];
		for (int i=0; i<length; i++)
			buffer[i] = unCompressed[i];

		compressedBuffer uncompressedBuffer;
		uncompressedBuffer.buffer = buffer;
		uncompressedBuffer.length = unCompressed.size();
		return uncompressedBuffer;
	}

	std::vector<compressedRLE> readCompressed()
	{
		std::vector<compressedRLE> compressed;
		int length = _inBufferLength;
		int bitIndex = 0;
		int byteIndex = 0;
		while (byteIndex < length)
		{
			byteIndex = bitIndex / 8;

			compressedRLE current;
			if (getBitIndex(_inBuffer, bitIndex) == 0)
			{
				bitIndex++;
				current.tail = readByte(_inBuffer, bitIndex);
				current.length = 0;
				current.position = 0;
				bitIndex += 8;//increment the bit index by one byte
			}
			else
			{
				bitIndex++;
				current.tail = readByte(_inBuffer, bitIndex);
				bitIndex += 8;
				current.length = readByte(_inBuffer, bitIndex);
				bitIndex += 8;
				current.position = readByte(_inBuffer, bitIndex);
				bitIndex += 8;
			}
			compressed.push_back(current);
		}
		return compressed;
	}
};