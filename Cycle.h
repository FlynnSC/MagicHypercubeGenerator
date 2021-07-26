#pragma once
#include <vector>

// Represents a cycle of values with a single "current value". 
// Allows iterating the current value through the cycle, as well as creating a cycle reduction
struct Cycle {
	std::vector<int> values;
	int currIndex;

	Cycle(std::vector<int> values);

	// Returns the current value
	int getCurr();

	// Iterates the current value to the next value in the cycle
	void next();

	// Returns a new cycle with the current value removed, making the 
	// current value of that new cycle the next value in the cycle
	Cycle getReduction();
};
