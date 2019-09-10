#ifndef P2_MEMORYMANAGER_H
#define P2_MEMORYMANAGER_H
//
// Created by pspam on 2/22/2019.
//

#include <cstddef>
#include <iostream>
#include <functional>
#include <vector>
#include <fcntl.h>

	int bestFit(int sizeInWords, void *pList);
	int worstFit(int sizeInWords, void *pList);


class MemoryManager
{

public:

	MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator);
	~MemoryManager();

	void initialize(size_t sizeInWords);
	void shutdown();
	void * allocate(size_t sizeInBytes);
	void free(void *address);
	void setAllocator(std::function<int(int, void *)> allocator);
	int dumpMemoryMap(char *filename);
	void * getList();
	void * getBitmap();
	unsigned getWordSize();
	void * getMemoryStart();
	unsigned getMemoryLimit();




private:

	static const size_t MAX_MEMORY_ARRAY_SIZE_IN_BYTES = 65535;
	uint8_t * memory_array;	// The memory segment for the entire manager (contains all the blocks).
	unsigned word_size;		// The size in bytes of the "word" unit of measurement.
	unsigned memLimit;		// stores memory limit of block that was allocated
	std::vector<uint16_t> hole_list;
	std::vector<uint16_t> filled_list;
	std::function<int(int, void *)> hole_finder;	// Allocator function for finding an available memory hole.
	std::vector<uint8_t> bitmap;

	void toggleBits(unsigned short offset, unsigned short length);	// Toggle the specified bits in the bitmap (1s to 0s, 0s to 1s)

	MemoryManager(const MemoryManager & M) {}	// Copy constructor made private to prevent anyone from using it.
};

#endif //P2_MEMORYMANAGER_H
