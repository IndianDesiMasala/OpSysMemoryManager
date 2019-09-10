//
// Created by pspam on 2/22/2019.
//
#include <fstream>
#include <cstring>
#include "MemoryManager.h"
using namespace std;



int bestFit(int sizeInWords, void *pList)
{
	if (sizeInWords <= 0)
		return -1;	// Invalid word size

	unsigned short stream_length = *((unsigned short *)pList);

	if (stream_length == 0)
		return -1;	// No holes available

	unsigned short * p_offset = (unsigned short *)pList;
	++p_offset;

	unsigned short * p_cur_best_fit = nullptr;

	// Loop through the entire holes list
	while (stream_length > 0)
	{
		if (p_offset[1] >= sizeInWords && (p_cur_best_fit == nullptr || p_offset[1] < p_cur_best_fit[1]))
			p_cur_best_fit = p_offset;	// Found a hole that fits and is smaller than any other such holes encountered so far
		p_offset += 2;
		stream_length -= 4;
	}

	if (p_cur_best_fit == nullptr)
		return -1;	// All available holes too small

	return *p_cur_best_fit;
}

// Worst fit : The memory manager places a process in the largest block of unallocated memory available.
// The idea is that this placement will create the largest hold after the allocations, thus increasing the possibility
// that, compared to best fit, another process can use the remaining space. Using the same example as above, worst fit
// will allocate 12KB of the 19KB block to the process, leaving a 7KB block for future use.
int worstFit(int sizeInWords, void *pList)
{
	if (sizeInWords <= 0)
		return -1;	// Invalid word size

	unsigned short stream_length = *((unsigned short *)pList);

	if (stream_length == 0)
		return -1;	// No holes available

	unsigned short * p_offset = (unsigned short *)pList;
	++p_offset;

	unsigned short * p_cur_worst_fit = nullptr;
	// Loop through the entire holes list
	while (stream_length > 0)
	{
		if (p_offset[1] >= sizeInWords && (p_cur_worst_fit == nullptr || p_offset[1] > p_cur_worst_fit[1]))
			p_cur_worst_fit = p_offset;	// Found a hole that fits and is smaller than any other such holes encountered so far
		p_offset += 2;
		stream_length -= 4;
	}

	if (p_cur_worst_fit == nullptr)
		return -1;	// All available holes too small

	return *p_cur_worst_fit;
}


// Sets native word size (for alignment) and default allocator function for finding a memory hole.
MemoryManager::MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator)
	: memory_array(nullptr), word_size(wordSize), hole_finder(allocator), bitmap(2,0), memLimit(0)
{
	hole_list.push_back(0);
	filled_list.push_back(0);
}

//Releases all memory allocated by this object without leaking memory.
MemoryManager::~MemoryManager()
{
	shutdown();
}

//Instantiates a new block of memory of requested size, no larger than 65536. Hint: you may use new uint8_t[].
void MemoryManager::initialize(size_t sizeInWords)
{
	if ( (sizeInWords > (MAX_MEMORY_ARRAY_SIZE_IN_BYTES / word_size)) || sizeInWords == 0)
		return;	// Deny initialize request if requested size is too large or too small

	shutdown();

	memory_array = new (std::nothrow) uint8_t[sizeInWords * word_size];	// If the allocation fails, memory_array will be nullptr

	if (memory_array != nullptr)
	{
		hole_list.at(0) = 4;
		hole_list.push_back(0);
		hole_list.push_back((uint16_t)sizeInWords);

		sizeInWords += 7;
		sizeInWords /= 8;
		bitmap.resize(2 + sizeInWords, 0);
		bitmap.at(0) = uint8_t(sizeInWords % 256);
		bitmap.at(1) = uint8_t(sizeInWords / 256);

		memLimit = sizeInWords * 8 * word_size;
	}
}

//Releases memory block acquired during initialization.
void MemoryManager::shutdown()
{
	if (memory_array != nullptr)
	{
		delete[] memory_array;
		memory_array = nullptr;
		hole_list.clear();
		hole_list.push_back(0);
		filled_list.clear();
		filled_list.push_back(0);
		bitmap.clear();
		bitmap.resize(2, 0);
		memLimit = 0;
	}
}

//Allocates a block of memory. If no memory is available or size is invalid, return nullptr.
void * MemoryManager::allocate(size_t sizeInBytes)
{
	int sizeInWords = (sizeInBytes + word_size - 1) / word_size; // Convert size in bytes to size in words, rounding up if necessary
	int offset = hole_finder(sizeInWords, getList());

	if (offset == -1)
		return nullptr;	// No holes large enough to accomodate requested size

	// Adjust the bitmap
	toggleBits(offset, sizeInWords);

	// Get the index in the hole list of the block found by the hole finder function
	size_t i = 1;
	for (; hole_list.at(i) != offset; i += 2);

	// Adjust the hole list
	if (hole_list.at(i + 1) == sizeInWords)
	{	// Filling hole up completely
		hole_list.erase(hole_list.begin() + i, hole_list.begin() + i + 2);
		hole_list.at(0) -= 4;
	}
	else
	{	// Only filling hole partially
		hole_list.at(i) += sizeInWords;
		hole_list.at(i + 1) -= sizeInWords;
	}

	// Get the index in the filled list where the block will be inserted
	size_t j = 1;
	for (; j < filled_list.size() && filled_list.at(j) < offset; j += 2);

	// Adjust the filled list
	filled_list.insert(filled_list.begin() + j, 2, offset);
	filled_list.at(j + 1) = sizeInWords;
	filled_list.at(0) += 4;

	return memory_array + offset * word_size;
}

//Frees the memory block within the memory manager so that it can be reused.
void MemoryManager::free(void * address)
{
	if (filled_list.size() < 2)
		return;	// Cannot free any memory block, because there aren't any allocated blocks!

	// Validate the address argument
	int offset = ( (uint8_t*)address - memory_array ) / word_size;
	if (offset < 0)
		return;	// Invalid address specified because requested address is outside the memory array

	// Search the filled list for the specified offset
	size_t i = 1;
	for (; i < filled_list.size() && filled_list.at(i) < offset; i += 2);

	if (i >= filled_list.size() || filled_list.at(i) != offset)
		return;	// Invalid address specified because requested offset does not match exactly one of the offsets in the filled list.

	// Adjust the bitmap
	toggleBits(filled_list.at(i), filled_list.at(i + 1));

	// Find the entry in the hole list where the new hole will be placed before
	size_t j = 1;
	for (; j < hole_list.size() && hole_list.at(j) < offset; j += 2);
	
	// Check if predeccesor hole (if any) is adjacent to the hole that's opening up now, and coalesce if so.
	if (j > 1 && hole_list.at(j - 2) + hole_list.at(j - 1) == offset)
	{
		hole_list.at(j - 1) += filled_list.at(i + 1);	// coalesce
		j -= 2;
	}
	else
	{	// Not coalescing, so just insert
		hole_list.insert(hole_list.begin() + j, 2, filled_list.at(i));
		hole_list.at(j + 1) = filled_list.at(i + 1);
		hole_list.at(0) += 4;
	}

	// Check if successor hole (if any) is adjacent to the hole that's opening up now, and coalesce if so.
	if (j + 2 < hole_list.size() && hole_list.at(j) + hole_list.at(j + 1) == hole_list.at(j + 2))
	{
		hole_list.at(j + 1) += hole_list.at(j + 3);
		hole_list.erase(hole_list.begin() + j + 2, hole_list.begin() + j + 4);
		hole_list.at(0) -= 4;
	}

	// Remove the entry from the filled list
	filled_list.erase(filled_list.begin() + i, filled_list.begin() + i + 2);
	filled_list.at(0) -= 4;
}

//Changes the allocation algorithm to identifying the memory hole to use for allocation.
void MemoryManager::setAllocator(std::function<int(int, void *)> allocator)
{
	hole_finder = allocator;
}

//Uses standard POSIX calls to write hole list to filename as text, returning -1 on error and 0 if successful.
//Format: "[START, LENGTH] - [START, LENGTH] ...", e.g., "[0, 10] - [12, 2] - [20, 6]"
int MemoryManager::dumpMemoryMap(char *filename)
{
	ofstream fout (filename);

	if (!fout.is_open())
		return -1;

	if (hole_list.size() <= 1)
		return 0;	// return -1 ?

	fout << '[' << hole_list.at(1) << ", " << hole_list.at(2) << ']';

	for (size_t i = 3; i < hole_list.size(); i += 2)
		fout << " - [" << hole_list.at(i) << ", " << hole_list.at(i + 1) << ']';

	fout.close();

	return 0;
}

//Returns a byte-stream of information (in binary) about holes for use by the allocator function (little-Endian)
void * MemoryManager::getList()
{
	uint16_t * hole_list_copy = new uint16_t[hole_list.size()];
	memcpy(hole_list_copy, hole_list.data(), hole_list.size() * 2);
	return hole_list_copy;
}

//Returns a bit-stream of bits representing whether words are used (1) or free (0). The first two bytes are the
//size of the bitmap (little-Endian); the rest is the bitmap, word-wise.
void * MemoryManager::getBitmap()
{	
	uint8_t * bitmap_copy = new uint8_t[bitmap.size()];
	memcpy(bitmap_copy, bitmap.data(), bitmap.size());
	for (size_t i = 2; i < bitmap.size(); ++i)
	{
		uint8_t rev_byte = 0;
		rev_byte |= (bitmap_copy[i] & (1 << 0)) << 7;
		rev_byte |= (bitmap_copy[i] & (1 << 1)) << 5;
		rev_byte |= (bitmap_copy[i] & (1 << 2)) << 3;
		rev_byte |= (bitmap_copy[i] & (1 << 3)) << 1;
		rev_byte |= (bitmap_copy[i] & (1 << 4)) >> 1;
		rev_byte |= (bitmap_copy[i] & (1 << 5)) >> 3;
		rev_byte |= (bitmap_copy[i] & (1 << 6)) >> 5;
		rev_byte |= (bitmap_copy[i] & (1 << 7)) >> 7;
	}
	return bitmap_copy;
}
//Returns the word size used for alignment.
unsigned MemoryManager::getWordSize()
{
	return word_size;
}

//Returns the byte-wise memory address of the beginning of the memory block.
void * MemoryManager::getMemoryStart()
{
	return memory_array;
}

//Returns the byte limit of the current memory block
unsigned MemoryManager::getMemoryLimit()
{
	return memLimit;
}

// Toggle the specified bits in the bitmap (1s to 0s, 0s to 1s)
void MemoryManager::toggleBits(uint16_t offset, uint16_t length)
{
	if (offset % 8 + length > 8)
	{	// Bits to be toggled overlaps at least 2 uint8_ts
		size_t i = offset / 8 + 2;
		length += offset - 1;
		size_t j = length / 8 + 2;
		offset %= 8;
		offset = 8 - offset - 1;
		length %= 8;
		length = 8 - length - 1;

		// Toggle the bits in the first uint8_t
		uint8_t c = (uint8_t)1;
		for (uint8_t b = (uint8_t)(1 << offset); offset > 0; --offset)
		{
			c |= b;
			b >>= 1;
		}
		bitmap.at(i) ^= c;

		// Toggle the bits in the middle uint8_ts (if any)
		while ( ++i < j )
			bitmap.at(i) ^= 0xff;

		// Toggle the bits in the last uint8_t
		c = (uint8_t)0;
		for (uint8_t b = (uint8_t)(1 << length); length < 8; ++length)
		{
			c |= b;
			b <<= 1;
		}
		bitmap.at(j) ^= c;
	}
	else
	{	// Bits to be toggled is confined to a single uint8_t
		size_t i = offset / 8 + 2;
		offset %= 8;
		offset = 8 - offset - 1;
		uint8_t c = (uint8_t)0;
		for (uint8_t b = (uint8_t)(1 << offset); length > 0; --length)
		{
			c |= b;
			b >>= 1;
		}
		bitmap.at(i) ^= c;
	}
}
