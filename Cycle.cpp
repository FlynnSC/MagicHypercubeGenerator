#include "Cycle.h"

Cycle::Cycle(std::vector<int> values) {
	this->values = values;
	currIndex = 0;
}

int Cycle::getCurr() {
	return values[currIndex];
}

void Cycle::next() {
	currIndex = (currIndex + 1) % values.size();
}

Cycle Cycle::getReduction() {
	Cycle newCycle(values);
	newCycle.values.erase(newCycle.values.begin() + currIndex);
	newCycle.currIndex = currIndex % newCycle.values.size();
	return newCycle;
}
