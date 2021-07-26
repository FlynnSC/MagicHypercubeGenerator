#include "Generator.h"
#include <functional>
#include <algorithm>
#include "Cycle.h"
#include <iostream>
#include <math.h>
#include <thread>
#include <chrono>

using namespace std;
using namespace chrono;

// Value cache for factorial
vector<int> factSet(1, 1);

int fact(int n) {
	while (n >= factSet.size()) {
		factSet.push_back(factSet.back() * factSet.size());
	}
	return factSet[n];
}

void printTimeTaken(chrono::high_resolution_clock::time_point startTime) {
	double secs = duration_cast<milliseconds>(high_resolution_clock::now() - startTime).count() * 0.001;
	int mins = std::floor(secs / 60);
	secs = secs - mins * 60;
	cout << "Time: ";
	if (mins < 10) cout << "0";
	cout << mins << ":";
	if (secs < 10) cout << "0";
	cout << secs << endl;
}

Generator::Generator(int _sideLength, int _dimensionality) {
	//------------------------------
	// Basic variable initialisation
	//------------------------------

	sideLength = _sideLength;
	dimensionality = _dimensionality;
	setSize = pow(sideLength, dimensionality);
	originValue = 1; //setSize / 2;
	for (int i = 0; i < dimensionality; ++i) {
		dimensionScales.push_back(pow(sideLength, i));
	}
	originalSum = (pow(sideLength, dimensionality + 1) + sideLength) / 2;
	intraAxisSwapPrintIndices = vector<int>(dimensionality, 0);

	//--------------------------------------------------------------------
	// convSet, segmentInfoSet and solidifiedSegmentInfoSet initialisation
	//--------------------------------------------------------------------

	convSet.resize(setSize);
	int value = 0;

	// Flags to keep track of which axes have already been solidified
	vector<bool> axesSolidified(dimensionality, false);

	// Indicates for each axis whether the next segment to be resolved needs to have that axis as a sumCheckSegment
	vector<bool> sumCheckAxisFlags(dimensionality, false);

	function<void(Cycle, int, vector<int>, int)> inner1 = [
		this, &value, &axesSolidified, &sumCheckAxisFlags, &inner1
	](Cycle axes, int currDimensionality, vector<int> axisScales, int offset) {
		int levelOffsetIncrement = 0;
		for (int axis : axes.values) {
			levelOffsetIncrement += dimensionScales[axis];
		}

		// Conv set segment resolution and info generation
		if (currDimensionality == 0) {
			int currAxis = axes.getCurr();
			SegmentInfo info;
			info.start = value;
			info.length = axisScales[currAxis];

			// Resolves current segment in convSet
			for (int i = 0; i < info.length; ++i) {
				convSet[offset + levelOffsetIncrement * i] = value;
				++value;
			}

			// Iterates backwards through the current axis from the offset position 
			// to resolve the sumComplementIndices
			for (int i = 1; i < sideLength - info.length + 1; ++i) {
				// Saves the indices in cube coordinates, to be converted to set coordinates later
				info.sumComplementIndices.push_back(offset - levelOffsetIncrement * i);
			}

			// Checks all axes for needed sumCheckSegment resolution
			for (int i = 1; i < dimensionality; ++i) {
				if (sumCheckAxisFlags[i]) {
					vector<int> segment;
					int scaleIncrement = dimensionScales[i];
					for (int j = 1; j < sideLength; ++j) {
						segment.push_back(offset - scaleIncrement * j);
					}
					sumCheckAxisFlags[i] = false;
					info.sumCheckSegments.push_back(segment);
				}
			}

			// The first segment to traverse the current axis is solidified, 
			// otherwise just enters the normal segmentInfo set
			if (!axesSolidified[currAxis]) {

				// If this is info for the very first segment, then the contents is altered
				// to take into account the fact that the first cell is universally solidified
				if (solidifiedSegmentInfoSet.size() == 0) {
					info.start = 1;
					info.length -= 1;
				}

				solidifiedSegmentInfoSet.push_back(info);
				axesSolidified[currAxis] = true;
			} else {
				segmentInfoSet.push_back(info);
			}
		} else {
			// Iterates diagonally (all axes simultaneously) through the current sub-structure 
			// (subset of the axes of the total structure, eg. plane within a cube)
			bool lastCellFilled = false;
			while (!lastCellFilled) {
				// Iterates through different substructures (eg. xy plane, then zx plane, then yz plane...)
				// by excluding one axis at a time (eg. z, then y, then x...)
				int innerOffset = 0;
				for (int i = 0; i < currDimensionality + 1 && !lastCellFilled; ++i) {
					// Flags that the currently excluded axis needs to be a sumCheckSegment
					int currAxis = axes.getCurr();
					if (axisScales[currAxis] == 1) sumCheckAxisFlags[currAxis] = true;

					inner1(axes.getReduction(), currDimensionality - 1, axisScales, offset + innerOffset);
					innerOffset += dimensionScales[currAxis];
					axisScales[currAxis] -= 1;
					lastCellFilled = axisScales[currAxis] == 0;
					axes.next();
				}
				offset += levelOffsetIncrement;
			}
		}
	};

	// Cycle initialised in reverse order to negate the reversing effect
	// of the recursion
	vector<int> axisValues;
	for (int i = 0; i < dimensionality; ++i) axisValues.push_back(dimensionality - 1 - i);
	Cycle axes(axisValues);

	vector<int> subStructureScales(dimensionality, sideLength);
	inner1(axes, dimensionality - 1, subStructureScales, 0);

	// Links each segmentInfo object to the next one in the set
	for (int i = 0; i < solidifiedSegmentInfoSet.size(); ++i) {
		solidifiedSegmentInfoSet[i].isAxisSegment = true;
		solidifiedSegmentInfoSet[i].nextSegment = (i == solidifiedSegmentInfoSet.size() - 1 ? nullptr
			: &solidifiedSegmentInfoSet[i + 1]);
	}

	// Last segment is removed because if all previous segments are resolved, then the final segment is resolved by 
	// definition
	segmentInfoSet.pop_back();
	for (int i = 0; i < segmentInfoSet.size(); ++i) {
		segmentInfoSet[i].nextSegment = (i == segmentInfoSet.size() - 1 ? nullptr : &segmentInfoSet[i + 1]);
	}

	// The segmentInfoSet and convSet now have to be rearranged such that that all solidified segments are placed 
	// consecutively at the front of the set, to ensure that once soldified, they are not affected by other segment 
	// resolution. This is easy for the segmentInfoSet, but convSet on the other hand, as a mapping from cube coords
	// to segment coords, is indexed in cube coords but needs to be manipulated within segment coords. This is 
	// accomplished by creating an inverseConvSet (a mapping from segment coords to cube coords derived from the 
	// convSet), manipulating that using segment coords, and then deriving the new convSet back from the inverseConvSet.
	vector<int> inverseConvSet(convSet.size());
	for (int i = 0; i < convSet.size(); ++i) {
		inverseConvSet[convSet[i]] = i;
	}

	// Starts at the third solidified segment as the first two are already consecutive
	int insertionIndex = sideLength + sideLength - 1;
	for (int i = 2; i < dimensionality; ++i) {
		SegmentInfo& solidifiedSegment = solidifiedSegmentInfoSet[i];

		// Takes the inverseConvSet indices for the solidified segment and places them at the insertionIndex, shifting all
		// subsequent indices
		auto segmentStartIter = inverseConvSet.cbegin() + solidifiedSegment.start;
		auto segmentEndIter = segmentStartIter + solidifiedSegment.length;
		vector<int> convSetIndices(segmentStartIter, segmentEndIter);
		inverseConvSet.erase(segmentStartIter, segmentEndIter);
		inverseConvSet.insert(inverseConvSet.cbegin() + insertionIndex, convSetIndices.cbegin(), convSetIndices.cend());

		// Updates info of all segments before the solidified segment
		for (SegmentInfo& segment : segmentInfoSet) {
			if (segment.start < solidifiedSegment.start) {
				segment.start += solidifiedSegment.length;
			} else break;
		}
		solidifiedSegment.start = insertionIndex;

		insertionIndex += solidifiedSegment.length;
	}

	// Maps transformations made on the inverseConvSet back onto the convSet
	for (int i = 0; i < convSet.size(); ++i) {
		convSet[inverseConvSet[i]] = i;
	}

	// Converts all cube coords into set coords after convSet has been reorganised
	for (SegmentInfo& segmentInfo : segmentInfoSet) {
		for (int& index : segmentInfo.sumComplementIndices) {
			index = convSet[index];
		}
		for (vector<int>& segment : segmentInfo.sumCheckSegments) {
			for (int& index : segment) {
				index = convSet[index];
			}
		}
	}


	//------------------------------------------------
	// permSegmentSets and permSwapSets initialisation
	//------------------------------------------------

	// permSegmentSets is required by both row swapping and axis swapping, 
	// and so needs to be large enough to satisfy both
	int permSegmentLength = max(sideLength, dimensionality);
	vector<int> set;
	for (int i = 0; i < permSegmentLength; ++i) {
		set.push_back(i);
	}

	// Recursively generates and fills permSegmentSet with every permutation of values -> [0, permSegmentLength - 1],
	// as well as saving the swap indices to permSwapSet
	function<void(int)> inner2 = [this, &set, &permSegmentLength, &inner2](int length) {
		if (length == 1) {
			permSegmentSets.push_back(set);
		} else {
			inner2(length - 1);
			for (int i = 0; i < length - 1; i++) {
				vector<int> swapSet = { length % 2 == 1 ? 0 : i, length - 1 };
				swap(set[swapSet[0]], set[swapSet[1]]);
				permSwapSets.push_back(swapSet);
				inner2(length - 1);
			}
		}
	};

	inner2(permSegmentLength);
}

void Generator::generate(PrintOption printOption) {
	this->printOption = printOption;
	ofs = ofstream("Magic Cubes.txt");

	vector<int> set;
	for (int i = 0; i < setSize; ++i) {
		set.push_back(i + 1);
	}

	// Makes the very first cell the origin value passed in
	swap(set[0], set[originValue - 1]);

	// First calculates the total number of axis solidification sets
	cout << "Counting axis solidification sets..." << endl;
	generating = true;
	calculatingAxisSolidificationSetCount = true;
	startTime = high_resolution_clock::now();
	thread progressDisplayThread1([this]() {
		while (generating) {
			this_thread::sleep_for((high_resolution_clock::now() - startTime) * 0.1);
			cout << "Axis solidification set count: " << totalAxisSolidificationSetCount << " | ";
			printTimeTaken(startTime);
		}
	});

	SegmentInfo firstSegment = solidifiedSegmentInfoSet[0];
	resolveSegment(set, firstSegment, firstSegment.start, setSize, setSize, originalSum - originValue);
	generating = false;
	progressDisplayThread1.join();
	cout << "Total axis solidification sets: " << totalAxisSolidificationSetCount << endl;
	printTimeTaken(startTime);

	// Then actually generate all cubes
	cout << endl << "Generating magic hypercubes..." << endl;
	calculatingAxisSolidificationSetCount = false;
	generating = true;
	startTime = high_resolution_clock::now();
	thread progressDisplayThread2([this]() { // TODO make a different thread so that joining between resolves works
		while (generating) {
			this_thread::sleep_for((high_resolution_clock::now() - startTime) * 0.1);
			cout << "Cube identity count: " << cubeIdentityCount << " | Axis solidification set progress: "
				<< traversedAxisSolidificationSetCount << "/" << totalAxisSolidificationSetCount << " | ";
			printTimeTaken(startTime);
		}
	});
	resolveSegment(set, firstSegment, firstSegment.start, setSize, setSize, originalSum - originValue);

	generating = false;
	progressDisplayThread2.join();
	cout << "Cube identities: " << cubeIdentityCount << endl;

	// All permutations of intra-axis swaps within each axis, and inter-axis swaps between axes
	cout << "Cubes: " << cubeIdentityCount * pow(fact(sideLength), dimensionality) * fact(dimensionality) << endl;
	printTimeTaken(startTime);
}

void Generator::resolveSegment(vector<int>& set, SegmentInfo& segmentInfo, int depth, int exemptPos, int segmentExemptPos, 
	int currSum) {
	if (depth == segmentInfo.start + segmentInfo.length - 1) {
		if (validateSumCheckSegments(set, segmentInfo, currSum)) {
			for (int i = depth; i < exemptPos; ++i) {
				if (set[i] == currSum) {
					swap(set[i], set[depth]);
					if (segmentInfo.isAxisSegment) {
						// If this solidifies the last axis segment then move onto subsequent non-axis segments, 
						// otherwise continue with the next axis segment
						bool isLastSegment = segmentInfo.nextSegment == nullptr;
						if (isLastSegment && calculatingAxisSolidificationSetCount) {
							++totalAxisSolidificationSetCount;
						} else {
							if (isLastSegment) {
								++traversedAxisSolidificationSetCount;
							}
							vector<int> newSet(set);
							SegmentInfo& nextSegment = isLastSegment ? segmentInfoSet[0] : *segmentInfo.nextSegment;
							int newExemptPos = isLastSegment ? setSize : segmentExemptPos;
							int newSum = originalSum - (isLastSegment ? set[nextSegment.sumComplementIndices[0]] : originValue);
							resolveSegment(newSet, nextSegment, nextSegment.start, newExemptPos, segmentExemptPos, newSum);
						}
					} else {
						if (segmentInfo.nextSegment == nullptr) {
							print(set);
						} else {
							permuteSegment(set, segmentInfo);
						}
					}
					break;
				}
			}
		}
	} else {
		if (set[depth] < currSum) {
			resolveSegment(set, segmentInfo, depth + 1, exemptPos, segmentExemptPos, currSum - set[depth]);
		}

		// Iterates backwards from the exempt pos to find an element that works
		while (exemptPos > segmentInfo.start + segmentInfo.length) {
			--exemptPos;
			if (segmentInfo.isAxisSegment && depth == segmentInfo.start) {
				--segmentExemptPos;
			}
			if (set[exemptPos] < currSum) {
				swap(set[exemptPos], set[depth]);
				resolveSegment(set, segmentInfo, depth + 1, exemptPos, segmentExemptPos, currSum - set[depth]);
			}
		}
	}
}

bool Generator::validateSumCheckSegments(vector<int>& set, SegmentInfo& segmentInfo, int& currSum) {
	for (vector<int>& segment : segmentInfo.sumCheckSegments) {
		int tempSum = originalSum;
		for (int& index : segment) {
			tempSum -= set[index];
		}
		if (tempSum != currSum) {
			return false;
		}
	}
	return true;
}

void Generator::permuteSegment(vector<int> set, SegmentInfo& segmentInfo) {
	// Feeds the set through as-is, then does every perm of the segment
	SegmentInfo* nextSegment = segmentInfo.nextSegment;
	int newSum = originalSum;
	for (int& index : nextSegment->sumComplementIndices) {
		newSum -= set[index];
	}
	resolveSegment(set, *nextSegment, nextSegment->start, setSize, setSize, newSum);

	int swapCount = fact(segmentInfo.length) - 1;
	for (int i = 0; i < swapCount; ++i) {
		vector<int>& swapSet = permSwapSets[i];
		swap(set[segmentInfo.start + swapSet[0]], set[segmentInfo.start + swapSet[1]]);
		nextSegment = segmentInfo.nextSegment;
		newSum = originalSum;
		for (int& index : nextSegment->sumComplementIndices) {
			newSum -= set[index];
		}
		resolveSegment(set, *nextSegment, nextSegment->start, setSize, setSize, newSum);
	}
}

void Generator::print(vector<int>& set) {
	++cubeIdentityCount;
	if (printOption == PrintOption::ALL) {
		printTransformations(set, dimensionality - 1);
	} else if (printOption == PrintOption::IDENTITIES) {
		printCube(set, dimensionality - 1, 0);
	}
}

void Generator::printCube(vector<int>& set, int axisIndex, int offset) {
	// Prints through the current axis
	for (int i = 0; i < sideLength; ++i) {
		// If the recursion has reached the foremost axis (the x axis) then it will print that segment, otherwise it 
		// will add to the offset and recurse
		int newOffset = offset + dimensionScales[permSegmentSets[interAxisSwapPrintIndex][axisIndex]]
			* permSegmentSets[intraAxisSwapPrintIndices[axisIndex]][i];
		if (axisIndex == 0) {
			ofs << set[convSet[newOffset]] << "\t";
		} else {
			printCube(set, axisIndex - 1, newOffset);
		}
	}
	ofs << "\n";
}

void Generator::printTransformations(vector<int>& set, int axisIndex) {
	// Iterates through intra-axis swaps
	int& intraAxisSwapIndex = intraAxisSwapPrintIndices[axisIndex];
	for (intraAxisSwapIndex = 0; intraAxisSwapIndex < fact(sideLength); ++intraAxisSwapIndex) {
		if (axisIndex == 0) {
			// Iterates through inter-axis swaps
			for (interAxisSwapPrintIndex = 0; interAxisSwapPrintIndex < fact(dimensionality); ++interAxisSwapPrintIndex) {
				printCube(set, dimensionality - 1, 0);
			}
		} else {
			printTransformations(set, axisIndex - 1);
		}
	}
}
