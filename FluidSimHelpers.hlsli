#ifndef FLUID_SIM_HELPER
#define FLUID_SIM_HELPER

#define GROUP_SIZE 8

int3 GetLeftIndex(int3 index) {
	index.x = index.x == 0 ? 0 : index.x - 1;
	return index;
}

int3 GetRightIndex(int3 index, int gridSize) {
	//index.x = index.x >= gridSize - 1 ? gridSize -1 : index.x + 1;
	index.x = min(index.x + 1, gridSize - 1); //clamp grid size 
	return index;
}

int3 GetBottomIndex(int3 index) {
	index.y = index.y == 0 ? 0 : index.y - 1;
	return index;
}

int3 GetTopIndex(int3 index, int gridSize) {
	//index.y = index.y >= gridSize - 1 ? gridSize - 1 : index.y + 1;
	index.y = min(index.y + 1, gridSize - 1);
	return index;
}

int3 GetBackIndex(int3 index) {
	index.z = index.z == 0 ? 0 : index.z - 1;
	return index;
}

int3 GetFrontIndex(int3 index, int gridSize) {
	//index.z = index.z == gridSize - 1 ? gridSize - 1 : index.z + 1;
	index.z = min(index.z + 1, gridSize - 1);
	return index;
}
#endif