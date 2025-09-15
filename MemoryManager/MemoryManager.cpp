#include "MemoryManager.h"

//Memory Manager class functions
MemoryManager::MemoryManager(unsigned wordSize, std::function<int(int, void*)> allocator)
{
	this->wordSize = wordSize;
	this->allocator = allocator;
	this->bytes = 0;
	this->totalWords = 0;
	this->allocated = false;
	this->mem = Memory();
	//number of holes is null until mem is initialized
	this->holes = nullptr; 
}
MemoryManager::~MemoryManager()
{
	//if init, call shutdown
	if (this->bytes != 0)
		shutdown();
}
void MemoryManager::initialize(size_t sizeInWords)
{
	//No larger than 65536 words
	if (sizeInWords > 65536)
		return;
	//Most of your other functions should not work before this is called.
	//They should return the relevant error for the data type, such as void, -1, nullptr, etc.

	//If initialize is called on an already initialized object, call shutdown then reinitialize.
	if (bytes != 0)
		shutdown();

	//Instantiates contiguous array of size(sizeInWords * wordSize) amount of bytes.
	this->totalWords = sizeInWords;
	this->bytes = this->wordSize * this->totalWords;
	this->mem = Memory(this->bytes);
}
void MemoryManager::shutdown()
{
	//If mem isn't initialized, dont perform shutdown
	if (this->bytes == 0)
		return;
	//If mem is initialized, clear all data. Free any heap memory, clear any relevant data structures, reset member variables
	delete [] ((uint8_t*)getMemoryStart());
	this->bytes = 0;
	this->totalWords = 0;
	this->allocated = false;
	this->mem = Memory();
	this->holes = nullptr;
}
void* MemoryManager::allocate(size_t sizeInBytes)
{
	//If mem isn't initialized or if memory is full, dont perform allocate
	if (this->bytes == 0 || !this->mem.getHoleCount())
		return nullptr;

	//allocated flag should turn to true once the first allocation happens (used in getList())
	if (!allocated)
		allocated = true;

	int words;
	if (sizeInBytes % wordSize != 0)
		words = (sizeInBytes / wordSize) + 1;
	else
		words = (sizeInBytes / wordSize);

	//Call algo to figure out where to allocate, convert word offset back to bytes offset
	int holeByteOffset = this->allocator(words, getList()) * wordSize;
	//make sure to free memory from getlist call before allocate terminates
	delete[] this->holes;

	//Allocate bytes in contiguous memory array with value of (uint8_t)1 (not necessary but maybe helpful for hole and block identification)
	void* p = getMemoryStart(); 
	for (int i = holeByteOffset; i < holeByteOffset + sizeInBytes; i++)
		//indexing has a higher precedence than casting, therefore just use parantheses to solve this problem
		((uint8_t*)p)[i] = (uint8_t)1;
	
	//Update block list
	this->mem.setBlock(holeByteOffset, sizeInBytes, ((uint8_t*)p) + holeByteOffset);

	//Update holes list (holds information in bytes)
	for (int i = 0; i < this->mem.getHoles().size(); i++)
	{
		//If its the hole used for current memory allocation
		if (holeByteOffset == this->mem.getHoles()[i].getStartBytes())
		{
			//if you take up the entire hole (sizeInBytes == hole.sizeBytes)
			if (sizeInBytes == this->mem.getHoles()[i].getSizeBytes())
				//delete exisiting hole 
				this->mem.getHoles().erase(this->mem.getHoles().begin() + i);
			else
			{
				//reduce existing hole: update offset (new offset = old startBytes + sizeInBytes) and shrink size by sizeInBytes 
				this->mem.getHoles()[i].setStartBytes(this->mem.getHoles()[i].getStartBytes() + sizeInBytes);
				this->mem.getHoles()[i].setSizeBytes(this->mem.getHoles()[i].getSizeBytes() - sizeInBytes);
			}
			break;
		}
	}
	
	//Returns a pointer somewhere in your memory block to the starting location of the newly allocated space.
	return ((uint8_t*)p) + holeByteOffset;
}
void MemoryManager::free(void* address)
{
	//If mem isn't initialized, dont perform free
	if (this->bytes == 0)
		return;

	//Change data allocated in block to 0, done with address (not necessary but maybe helpful for hole and block identification)
	bool leftAdj = false, rightAdj = false; //keep track of adjacent holes 
	int x = -1, y = -1, blockIndex = -1;
	for (int i = 0; i < this->mem.getBlockCount(); i++)
	{
		//Block found
		if (this->mem.getBlocks()[i].getStartAddress() == address)
		{
			blockIndex = i;
			x = this->mem.getBlocks()[i].getStartBytes();
			y = this->mem.getBlocks()[i].getSizeBytes();
			for (int j = x; j < x + y; j++)
				((uint8_t*)getMemoryStart())[j] = (uint8_t)0;

			//check weather there are any adjacent holes
			if (x > 0 && ((uint8_t*)getMemoryStart())[x - 1] == (uint8_t)0)
				leftAdj = true;
			if ((x + y) < this->bytes && ((uint8_t*)getMemoryStart())[x + y] == (uint8_t)0)
				rightAdj = true; 

			break;
		}
	}
	
	//Figure out which hole is to the left/right of current block 
	int leftHole = -1, rightHole = -1;
	if (leftAdj || rightAdj)
	{
		for (int k = 0; k < this->mem.getHoleCount(); k++)
		{
			//In terms of the starting position of the current hole
			int leftBlockLocation = this->mem.getHoles()[k].getStartBytes() - this->mem.getBlocks()[blockIndex].getSizeBytes();
			int rightBlockLocation = this->mem.getHoles()[k].getStartBytes() + this->mem.getHoles()[k].getSizeBytes();
			//use blockLocation with pointer arithmatic???
			if ((rightHole < 0) && (leftBlockLocation == this->mem.getBlocks()[blockIndex].getStartBytes()))
				rightHole = k;
			else if ((leftHole < 0) && (rightBlockLocation == this->mem.getBlocks()[blockIndex].getStartBytes()))
				leftHole = k;
		}
	}

	//case 1: if none, make new hole
	if (!leftAdj && !rightAdj)
		this->mem.setHole(x, y, ((uint8_t*)getMemoryStart()) + x);
	//case 2: if hole left, keep offset and increase left hole size by block size
	else if (leftAdj && !rightAdj)
	{
		this->mem.getHoles()[leftHole].setSizeBytes(this->mem.getHoles()[leftHole].getSizeBytes() + this->mem.getBlocks()[blockIndex].getSizeBytes());
	}
	//case 3: if hole right, decrease offset by block size and increase right hole size by block size
	else if (!leftAdj && rightAdj)
	{
		this->mem.getHoles()[rightHole].setStartBytes(this->mem.getBlocks()[blockIndex].getStartBytes());
		this->mem.getHoles()[rightHole].setSizeBytes(this->mem.getHoles()[rightHole].getSizeBytes() + this->mem.getBlocks()[blockIndex].getSizeBytes());
	}
	//case 4: if two adjacent holes: keep left hole offset and increase left hole size by block size + right hole size, delete right hole
	else if (leftAdj && rightAdj)
	{
		int newSize = this->mem.getHoles()[leftHole].getSizeBytes() + this->mem.getHoles()[rightHole].getSizeBytes() + this->mem.getBlocks()[blockIndex].getSizeBytes();
		this->mem.getHoles()[leftHole].setSizeBytes(newSize);
		this->mem.getHoles().erase(this->mem.getHoles().begin() + rightHole);
	}

	//delete block
	this->mem.getBlocks().erase(this->mem.getBlocks().begin() + blockIndex);
}
void* MemoryManager::getList()
{
	//If mem isn't initialized or if no memory has been allocated, dont perform getList
	if (this->bytes == 0 || !this->allocated)
		return nullptr;

	//offset = (startBytes / wordSize), length = sizeBytes / wordSize
	//uint16_t* holes dynamically allocates an array of size [Hole object array * 2 + 1] with the information stored in an vector of Hole objects
	int size = (this->mem.getHoleCount() * 2) + 1;
	this->holes = new uint16_t[size];
	this->holes[0] = (uint16_t)this->mem.getHoleCount();

	sort(this->mem.getHoles().begin(), this->mem.getHoles().end());
	
	int j = 0;
	for (int i = 1; i < size; i++)
	{
		//Odd index elements are always the hole offsets
		if (i % 2 != 0)
		{
			uint16_t offset;		
			if (this->mem.getHoles()[j].getStartBytes() % wordSize != 0)
				offset = (this->mem.getHoles()[j].getStartBytes() / wordSize) + 1;
			else
				offset = this->mem.getHoles()[j].getStartBytes() / wordSize;
			this->holes[i] = offset;
		}
		//Even index elements are always the hole lengths
		else
		{
			uint16_t sizeInWords;
			if (this->mem.getHoles()[j].getSizeBytes() % wordSize != 0)
				sizeInWords = (this->mem.getHoles()[j].getSizeBytes() / wordSize) + 1;
			else
				sizeInWords = this->mem.getHoles()[j].getSizeBytes() / wordSize;
			this->holes[i] = sizeInWords;
			j++;
		}
	}

	return this->holes;
}
unsigned MemoryManager::getWordSize()
{
	//Returns wordSize member variable.
	return this->wordSize;
}
void* MemoryManager::getMemoryStart()
{
	//If mem isn't initialized, dont perform getMemoryStart
	if (this->bytes == 0)
		return nullptr;

	//Returns pointer to the start of your contiguous memory array
	return this->mem.getMemStart();
}
unsigned MemoryManager::getMemoryLimit()
{
	//Returns unsigned int(unsigned and unsigned int are the same type) that is the total amount of bytes you can store.
	return this->bytes;
}
void MemoryManager::setAllocator(std::function<int(int, void*)> allocator)
{
	//Just a setter function.Changes your member variable to the new allocator.
	this->allocator = allocator;
}
int MemoryManager::dumpMemoryMap(char* filename)
{ 
	/*
	Prints out current list of holes to a file. You must use POSIX calls, you cannot use fstream objects.
	Use open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777) to create, enable read-write, and truncate
	the file on creation. Remember to call close on the file descriptor before ending the function, or
	your changes may not save.
	*/
	int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777);

	//Error opening file
	if (fd == -1)
		return -1;
	
	//Write data to file (temp.front().c_str(), strlen(temp.front().c_str()))
	getList();
	int length = this->holes[0] * 2;
	string data = "";
	data += "[" + to_string(this->holes[1]) + ", " + to_string(this->holes[2]);
	for (int i = 3; i < length; i += 2)
	{
		data += "] - [" + to_string(this->holes[i]) + ", " + to_string(this->holes[i + 1]);
	}
	//delete dynamic Holes array
	delete[] this->holes;
	data += "]";
	size_t bytesWritten = write(fd, data.c_str(), strlen(data.c_str()));
	
	//Error writing to file
	if (bytesWritten == -1)
	{
		close(fd);
		return -1;
	}
	
	//close the file (-1 if error closing file)
	if (close(fd) == -1)
		return -1;

	return 0;
}
void* MemoryManager::getBitmap()
{
	//use shifting, bitOR, bitAND (<<, |, &) 

	string wordStream = "";
	int byteCountBlock = 0, byteCountHole = 0, words = 0, byteSpace = 0;
	//Converts memory bitstream into wordstream (8-bit intervals) 
	for (int i = 0; i < bytes; i++)
	{
		//If block
		if (unsigned(((uint8_t*)getMemoryStart())[i]))
		{
			//convert hole to words
			if (byteCountHole)
			{
				if (byteCountHole % wordSize != 0)
					words = (byteCountHole / wordSize) + 1;
				else
					words = (byteCountHole / wordSize);

				for (int j = 0; j < words; j++)
				{
					if (byteSpace != 0 && byteSpace % 8 == 0)
						byteSpace = 0;

					wordStream += "0";
					byteSpace++;
				}

				byteCountHole = 0;
			}

			byteCountBlock++;
		}
		//If hole
		else
		{
			//convert block to words
			if (byteCountBlock)
			{
				if (byteCountBlock % wordSize != 0)
					words = (byteCountBlock / wordSize) + 1;
				else
					words = (byteCountBlock / wordSize);

				for (int j = 0; j < words; j++)
				{
					if (byteSpace != 0 && byteSpace % 8 == 0)
						byteSpace = 0;

					wordStream += "1";
					byteSpace++;
				}

				byteCountBlock = 0;
			}

			byteCountHole++;
		}
	}

	//Accounts for last block/hole (skipped over because loop ends)
	if (byteCountBlock)
	{
		if (byteCountBlock % wordSize != 0)
			words = (byteCountBlock / wordSize) + 1;
		else
			words = (byteCountBlock / wordSize);

		for (int j = 0; j < words; j++)
		{
			if (byteSpace != 0 && byteSpace % 8 == 0)
				byteSpace = 0;

			wordStream += "1";
			byteSpace++;
		}

		byteCountBlock = 0;
	}
	else
	{
		if (byteCountHole % wordSize != 0)
			words = (byteCountHole / wordSize) + 1;
		else
			words = (byteCountHole / wordSize);

		for (int j = 0; j < words; j++)
		{
			if (byteSpace != 0 && byteSpace % 8 == 0)
				byteSpace = 0;

			wordStream += "0";
			byteSpace++;
		}

		byteCountHole = 0;
	}

	//Covers the rest of the byte with 0's 
	while (byteSpace < 8)
	{
		wordStream += "0";
		byteSpace++;
	}

	//Mirror each individual wordstream byte
	string mirror = "";
	while (byteSpace <= wordStream.size())
	{
		byteSpace--;
		mirror += wordStream[byteSpace];
		if (byteSpace % 8 == 0)
		{
			byteSpace += 16;
		}
	}

	//Add two size bytes to the front
	string wordStreamSizeBits = bitset<16>(wordStream.size() / 8).to_string();
	//cout << wordStreamSizeBits;

	//TODO: flip wordStreamSizeBits for little-Endian and push it to front of mirror 
	string littleEnd1 = "", littleEnd2 = "";
	for (int i = 0; i < wordStreamSizeBits.size(); i++)
	{
		if (i < 8)
			littleEnd2 += wordStreamSizeBits[i];
		else
			littleEnd1 += wordStreamSizeBits[i];
	}

	string bitMap = littleEnd1 + littleEnd2 + mirror;

	int size = (wordStream.size() / 8) + 2;
	uint8_t* bitWordMap = new uint8_t[size];

	int index = 0;
	for (int i = 0; i < size; i++)
	{
		string temp = bitMap.substr(index, 8);
		bitWordMap[i] = (uint8_t)stoi(temp, nullptr, 2);
		index += 8;
	}

	return bitWordMap;
}

//Hole class functions (nested within Memory)
MemoryManager::Memory::Hole::Hole()
{

}
MemoryManager::Memory::Hole::Hole(int startBytes, int sizeBytes, uint8_t* addy)
{
	this->startBytes = startBytes;
	this->sizeBytes = sizeBytes;
	this->address = addy;
}
bool MemoryManager::Memory::Hole::operator < (const Hole& h) const
{
	return (this->startBytes < h.startBytes);
}
int MemoryManager::Memory::Hole::getStartBytes()
{
	return this->startBytes;
}
int MemoryManager::Memory::Hole::getSizeBytes()
{
	return this->sizeBytes;
}
void MemoryManager::Memory::Hole::setStartBytes(int newStartBytes)
{
	this->startBytes = newStartBytes;
}
void MemoryManager::Memory::Hole::setSizeBytes(int newSizeBytes)
{
	this->sizeBytes = newSizeBytes;
}
uint8_t* MemoryManager::Memory::Hole::getStartAddress()
{
	return this->address;
}

//Block class functions (nested within Memory)
MemoryManager::Memory::Block::Block()
{

}
MemoryManager::Memory::Block::Block(int startBytes, int sizeBytes, uint8_t* addy)
{
	this->startBytes = startBytes;
	this->sizeBytes = sizeBytes;
	this->address = addy;
}
int MemoryManager::Memory::Block::getStartBytes()
{
	return this->startBytes;
}
int MemoryManager::Memory::Block::getSizeBytes()
{
	return this->sizeBytes;
}
void MemoryManager::Memory::Block::setStartBytes(int newStartBytes)
{
	this->startBytes = newStartBytes;
}
void MemoryManager::Memory::Block::setSizeBytes(int newSizeBytes)
{
	this->sizeBytes = newSizeBytes;
}
uint8_t* MemoryManager::Memory::Block::getStartAddress()
{
	return this->address;
}

//Memory class functions
MemoryManager::Memory::Memory()
{

}
MemoryManager::Memory::Memory(int bytes)
{
	this->dynMemory = new uint8_t[bytes]();
	this->currHoles = vector<Hole>(1, Hole(0, bytes, dynMemory));
	this->currBlocks = vector<Block>();
}
void* MemoryManager::Memory::getMemStart()
{
	return this->dynMemory;
}
int MemoryManager::Memory::getHoleCount()
{
	return this->currHoles.size();
}
int MemoryManager::Memory::getBlockCount()
{
	return this->currBlocks.size();
}
vector<MemoryManager::Memory::Hole>& MemoryManager::Memory::getHoles()
{
	return this->currHoles;
}
vector<MemoryManager::Memory::Block>& MemoryManager::Memory::getBlocks()
{
	return this->currBlocks;
}
void MemoryManager::Memory::setHole(int startBytes, int sizeBytes, uint8_t* addy)
{
	this->currHoles.push_back(Hole(startBytes, sizeBytes, addy));
}
void MemoryManager::Memory::setBlock(int startBytes, int sizeBytes, uint8_t* addy)
{
	this->currBlocks.push_back(Block(startBytes, sizeBytes, addy));
}

//Mem Allocation Algorithms
int bestFit(int sizeInWords, void* list)
{
	/*
	Allocator function, can be written inside MemoryManager.cpp but does not belong to MemoryManager class.
	List will be structured like the output from getList.
	Finds a hole in the list that best fits the given sizeInWords, meaning it selects the smallest possible hole that still fits sizeInWords.
	Returns word offset to the start of that hole.
	*/
	
	int length = ((uint16_t*)list)[0] * 2;
	int bestOffset = -1, smallestSize = 65537; //No larger than 65536 words, set initial smallest size to one higher for first comparison reasons
	for (int i = 2; i <= length; i += 2)
	{
		//If the current hole has a smaller size than the previous smallestSize and current hole size >= sizeInWords 
		if (((uint16_t*)list)[i] < smallestSize && ((uint16_t*)list)[i] >= sizeInWords)
		{
			smallestSize = ((uint16_t*)list)[i];
			bestOffset = ((uint16_t*)list)[i - 1];
		}
	}

	return bestOffset;
}
int worstFit(int sizeInWords, void* list)
{
	//Same as above, but finds largest possible hole instead.
	int length = ((uint16_t*)list)[0] * 2;
	int worstOffset = -1, largestSize = 0; //No smaller than 1 word, set initial largest size to one lower for first comparison reasons
	for (int i = 2; i <= length; i += 2)
	{
		//If the current hole has a larger size than the previous largestSize and current hole size >= sizeInWords 
		if (((uint16_t*)list)[i] > largestSize && ((uint16_t*)list)[i] >= sizeInWords)
		{
			largestSize = ((uint16_t*)list)[i];
			worstOffset = ((uint16_t*)list)[i - 1];
		}
	}

	return worstOffset;
}
