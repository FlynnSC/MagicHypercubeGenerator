#pragma once
#include <vector>
#include <chrono>
#include <fstream>

using std::vector;
using std::chrono::high_resolution_clock;
using std::ofstream;

struct SegmentInfo {
	int start; // Starting index of the segment within the set 
	int length;
	bool isAxisSegment = false;
	vector<int> sumComplementIndices; // List of indices within set that make up the segment's sum complement
	vector<vector<int>> sumCheckSegments;
	SegmentInfo* nextSegment;
};

enum class PrintOption {
	ALL,
	IDENTITIES,
	NONE,
};

class Generator {
	int dimensionality; // The number of dimensions the cube has (2 = square, 3 = cube, 4 = hypercube, etc)
	int sideLength;
	int setSize;
	vector<int> dimensionScales; // Set of values of sidelength^d where d -> [0, dimensionality - 1]
	int originalSum; // Required sum for a single full segment
	int originValue; // The value for the first element of the set (origin point of cube)

	unsigned long cubeIdentityCount = 0;
	bool calculatingAxisSolidificationSetCount;
	unsigned long totalAxisSolidificationSetCount = 0;
	unsigned long traversedAxisSolidificationSetCount = 0;
	high_resolution_clock::time_point startTime;
	bool generating = false;

	// Printing stuff
	PrintOption printOption;
	ofstream ofs;
	int interAxisSwapPrintIndex = 0;
	vector<int> intraAxisSwapPrintIndices;

	vector<int> convSet; // Converts an index from cube coordinates to set coordinates

	// Set containing all possible permutations of the values -> [0, max(sideLength, dimensionality) - 1]
	vector<vector<int>> permSegmentSets; 

	// Set containing the all pairs of indices that when swapped can acheive the segment permutations contained within 
	// permSegmentSet
	vector<vector<int>> permSwapSets; 
	vector<SegmentInfo> segmentInfoSet;
	vector<SegmentInfo> solidifiedSegmentInfoSet;

	/*
	* Recursively resolves each element in the current segment (recursion transition A), and then calls into the next 
	* task function. Axis segments are first, and so having completed an axis segment this function will call itself 
	* again (recursion transition B) to move onto the next axis segment. Having resolved the last axis segment, it will 
	* call itself again (recursion transition C) to move onto the first non-axis segment. Having resolved a non-axis
	* segment, if it was the last segment in the set then it will call print(), otherwise it will call into 
	* permuteSegment()
	* 
	* exemptPos acts as an index marker to identify which vales in the set have already been tried in the resolution of 
	* the current segment. segmentExemptPos has a similar purpose, but effectively only applies to the first element of 
	* each axis segment, to ensure the same combinations of segments aren't generated multiple times in different orders
	* during axis solidification
	*/
	void resolveSegment(vector<int>& set, SegmentInfo& segmentInfo, int depth, int exemptPos, int segmentExemptPos, 
		int currSum);

	// Ensures that the currSum matches the value that would be created by the segment's sum check segments 
	// (where necessary)
	bool validateSumCheckSegments(vector<int>& set, SegmentInfo& segmentInfo, int& currSum);

	// Iterates through every permutation of the current segment, calling into resolveSegment() for every permutation 
	// generated
	void permuteSegment(vector<int> set, SegmentInfo& segmentInfo);

	// Simple interface point for performing the correct printing logic based on the value of printOption
	void print(vector<int>& set);

	// Recursively propegates through the cube and prints its elements to a file in the correct format
	void printCube(vector<int>& set, int axisIndex, int offset);

	// Recursively performs intra/inter-axis swap logic and then delegates to printCube
	void printTransformations(vector<int>& set, int axisIndex);

public:
	Generator(int sideLength, int dimensionality);
	void generate(PrintOption printOption);
};
