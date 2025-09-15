/*~~~~~~~~~~~RESOURCES~~~~~~
	https://stackoverflow.com/questions/22746429/c-decimal-to-binary-converting
	https://www.geeksforgeeks.org/cpp-bitset-and-its-application/
	https://stackoverflow.com/questions/23596988/binary-string-to-integer-with-atoi
	https://www.geeksforgeeks.org/cpp-pointer-arithmetic/
	https://man7.org/linux/man-pages/man2/open.2.html
	https://www.oreilly.com/library/view/c-cookbook/0596003390/ch16s02.html#:~:text=Solution&text=Using%20the%20%3E%20%2C,same%20element%20in%20an%20array.
	https://medium.com/@joshuaudayagiri/linux-system-calls-close-97e3ab3bce8#:~:text=When%20you're%20done%20using%20a%20file%2C%20it's%20important%20to,by%20the%20open%20system%20call.
*/

#include <fcntl.h>
#include<cstring>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <functional>
#include <vector>
#include <algorithm>
#include <string>
#include <bitset>
using namespace std;
#pragma once

class MemoryManager
{
public:
	MemoryManager(unsigned wordSize, std::function<int(int, void*)> allocator);
	~MemoryManager();
	void initialize(size_t sizeInWords);
	void shutdown();
	void* allocate(size_t sizeInBytes);
	void free(void* address);
	void* getList();
	unsigned getWordSize();
	void* getMemoryStart();
	unsigned getMemoryLimit();
	void setAllocator(std::function<int(int, void*)> allocator);
	int dumpMemoryMap(char* filename);
	void* getBitmap();
private:
	struct Memory
	{
		struct Hole
		{
			Hole();
			Hole(int startBytes, int sizeBytes, uint8_t* addy);
			bool operator < (const Hole& h) const;
			int getStartBytes();
			int getSizeBytes();
			void setStartBytes(int newStartBytes);
			void setSizeBytes(int newSizeBytes);
			uint8_t* getStartAddress();
			int startBytes;
			int sizeBytes;
			uint8_t* address;
		};

		struct Block
		{
			Block();
			Block(int startBytes, int sizeBytes, uint8_t* addy);
			int getStartBytes();
			int getSizeBytes();
			void setStartBytes(int newStartBytes);
			void setSizeBytes(int newSizeBytes);
			uint8_t* getStartAddress();
			int startBytes;
			int sizeBytes;
			uint8_t* address;
		};

		Memory();
		Memory(int bytes);
		void* getMemStart();
		int getHoleCount();
		int getBlockCount();
		vector<Hole>& getHoles();
		vector<Block>& getBlocks();
		void setHole(int startBytes, int sizeBytes, uint8_t* addy);
		void setBlock(int startBytes, int sizeBytes, uint8_t* addy);
		uint8_t* dynMemory;
		vector<Hole> currHoles;
		vector<Block> currBlocks;
	};
	unsigned wordSize;
	unsigned totalWords;
	unsigned bytes;
	bool allocated;
	Memory mem;
	uint16_t* holes;
	std::function<int(int, void*)> allocator;
};

//Mem Allocation Algorithms
int bestFit(int sizeInWords, void* list);
int worstFit(int sizeInWords, void* list);