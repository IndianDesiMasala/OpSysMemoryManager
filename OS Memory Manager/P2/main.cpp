#include "MemoryManager.h"
#include "AllocateAlgorithm.h"

int main()
{
	MemoryManager M1(4, AllocateAlgorithm::bestFit);
	// hole_list == [0]
	// filled_list == <0>

	M1.initialize(71);
	// hole_list == [4][0,71]
	// filled_list == <0>

	void * p1 = M1.allocate(29);
	// hole_list == [4][8,63]
	// filled_list == <4> <0,8>

	void * p2 = M1.allocate(32);
	// hole_list == [4][16,55]
	// filled_list == <8> <0,8> <8,8>

	void * p3 = M1.allocate(32);
	// hole_list == [4][24,47]
	// filled_list == <12> <0,8> <8,8> <16,8>

	void * p4 = M1.allocate(32);
	// hole_list == [4][32,39]
	// filled_list == <16> <0,8> <8,8> <16,8> <24,8>


	M1.free(0);
	M1.free((char*)p1 + 4);
	M1.free((char*)p4 + 100);

	M1.free(p1);
	// hole_list == [8] [0,8] [32,39]
	// filled_list == <12> <8,8> <16,8> <24,8>

	M1.free(p3);
	// hole_list == [12] [0,8] [16,8] [32,39]
	// filled_list == <8> <8,8>  <24,8>

	M1.free(p2);
	// hole_list == [8] [0,24] [32,39]
	// filled_list == <4> <24,8>

	M1.free(p4);
	// hole_list == [4] [0,71]
	// filled_list == <0> 

	return 0;
}