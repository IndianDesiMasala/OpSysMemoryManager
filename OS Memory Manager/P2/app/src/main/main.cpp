#include "MemoryManager.h"
#include "AllocateAlgorithm.h"

int main()
{
	MemoryManager M1(4, AllocateAlgorithm::bestFit);
	// hole_list == [0]
	// filled_list == <0>

	M1.initialize(24);
	// bitmap == [00000000][00011000][00000000][00000000][00000000]

	void * p1 = M1.allocate(24);
	// bitmap == [00000000][00011000][11111100][00000000][00000000]

	void * p2 = M1.allocate(36);
	// bitmap == [00000000][00011000][11111111][11111110][00000000]

	void * p3 = M1.allocate(12);
	// bitmap == [00000000][00011000][11111111][11111111][11000000]

	M1.free(p2);
	// bitmap == [00000000][00011000][11111100][00000001][11000000]

	M1.free(p1);
	// bitmap == [00000000][00011000][00000000][00000001][11000000]

	p1 = M1.allocate(20);
	// bitmap == [00000000][00011000][00000000][00000001][11111110]

	M1.free(p3);
	// bitmap == [00000000][00011000][00000000][00000000][00111110]

	return 0;
}