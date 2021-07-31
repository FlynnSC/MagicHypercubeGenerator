#include <time.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <math.h>

void printTime(clock_t t1, clock_t t2)
{
	double secs = double(t2 - t1) / CLOCKS_PER_SEC;
	int mins = floor(secs / 60);
	secs = secs - mins * 60;
	std::cout << "Time: ";
	if (mins < 10) std::cout << "0";
	std::cout << mins << ":";
	if (secs < 10) std::cout << "0";
	std::cout << secs << std::endl;
}

int fact(int n)
{
	for (int i = n - 1; i > 1; --i)
	{
		n *= i;
	}
	return n;
}

void printVector(std::vector<int>& set)
{
	for (int x : set)
	{
		std::cout << x << " ";
	}
	std::cout << std::endl;
}

void printSet(int set[], int length)
{
	for (int i = 0; i < length; ++i)
	{
		std::cout << set[i] << " ";
	}
	std::cout << std::endl;
}

static clock_t startTime;
static unsigned long squareCount = 0;
static int sideLength;
static int setSize;
static int originalSum;
static int* convSet; // Converts between shell index and square position
static int** permSet; // Set containing all possible permutations of size sideLength
static int* rotSet;
static int permSetSize;

void initialiseConvSet()
{
	convSet = new int[(int)pow(sideLength, 2)];
	int curr = 0;
	for (int i = 0; i < sideLength; ++i)
	{
		for (int j = 0; j < sideLength - i; ++j)
		{
			convSet[(sideLength + 1) * i + j] = curr;
			++curr;
		}
		for (int j = 1; j < sideLength - i; ++j)
		{
			convSet[(sideLength + 1) * i + sideLength * j] = curr;
			++curr;
		}
	}
}

void permSetPerm(int set[], int length, int& permIndex)
{
	if (length == 1)
	{
		permSet[permIndex] = new int[sideLength];
		std::copy(set, set + sideLength, permSet[permIndex]);
		++permIndex;
	}
	else
	{
		permSetPerm(set, length - 1, permIndex);
		for (int i = 0; i < length - 1; i++)
		{
			if (length % 2 == 1) std::swap(set[0], set[length - 1]);
			else std::swap(set[i], set[length - 1]);			
			permSetPerm(set, length - 1, permIndex);
		}
	}
}

void initialisePermSet()
{
	permSetSize = fact(sideLength);
	permSet = new int*[permSetSize];

	int* set = new int[sideLength];
	for (int i = 0; i < sideLength; ++i)
	{
		set[i] = i;
	}
	int permIndex = 0;
	permSetPerm(set, sideLength, permIndex);
}

void initialiseRotSet()
{
	rotSet = new int[setSize];
	for (int i = 0; i < sideLength; ++i)
	{
		for (int j = 0; j < sideLength; ++j)
		{
			rotSet[sideLength * i + j] = sideLength * j + i;
		}
	}
}

void printSquare(int set[])
{
	static std::ofstream ofs("Magic Squares.txt");
	for (int i = 0; i < permSetSize; ++i)
	{
		for (int j = 0; j < permSetSize; ++j)
		{
			for (int k = 0; k < sideLength; ++k)
			{
				int offset = permSet[i][k] * sideLength;
				for (int l = 0; l < sideLength; ++l)
				{
					ofs << set[convSet[offset + permSet[j][l]]];
					if (l != sideLength - 1) ofs << "\t";
				}
				ofs << "\n";
			}
			ofs << "\n";
			//Rotated compositions
			for (int k = 0; k < sideLength; ++k)
			{
				int offset = permSet[i][k] * sideLength;
				for (int l = 0; l < sideLength; ++l)
				{
					ofs << set[convSet[rotSet[offset + permSet[j][l]]]];
					if (l != sideLength - 1) ofs << "\t";
				}
				ofs << "\n";
			}
			ofs << "\n";
			squareCount += 2;
		}
	}
}

void comb(int set[], const int& segmentStart, const int& segmentLength, int depth, int exemptPos, int sum, const int& level, const bool& horizontal);

void perm(int set[], const int& segmentStart, const int& segmentLength, int length, const int& level, const bool& horizontal)
{
	if (length == 1)
	{
		if (level == sideLength - 2 && !horizontal) printSquare(set);
		else
		{
			int newSegmentStart = segmentStart + segmentLength;
			int newSegmentLength = (horizontal) ? segmentLength - 1 : segmentLength;
			int newDepth = newSegmentStart;
			int newExemptPos = setSize;
			int newLevel = (horizontal) ? level : level + 1;
			bool newHorizontal = !horizontal;
			int newSum = originalSum;
			if (newHorizontal)
			{
				for (int i = 0; i < newLevel; ++i)
				{
					newSum -= set[convSet[sideLength * newLevel + i]];
				}
			}
			else
			{
				for (int i = 0; i <= newLevel; ++i)
				{
					newSum -= set[convSet[sideLength * i + newLevel]];
				}
			}
			if (newSum > 0) comb(set, newSegmentStart, newSegmentLength, newDepth, newExemptPos, newSum, newLevel, newHorizontal);
		}
	}
	else
	{
		perm(set, segmentStart, segmentLength, length - 1, level, horizontal);
		for (int i = 0; i < length - 1; i++)
		{
			if (length % 2 == 1) std::swap(set[segmentStart], set[segmentStart + length - 1]);
			else std::swap(set[segmentStart + i], set[segmentStart + length - 1]);
			perm(set, segmentStart, segmentLength, length - 1, level, horizontal);
		}
	}
}

void comb(int set[], const int& segmentStart, const int& segmentLength, int depth, int exemptPos, int sum, const int& level, const bool& horizontal)
{
	if (depth == segmentStart + segmentLength - 1)//Last position in segment
	{
		//Finds the element to complete the sum, then perms
		for (int i = depth; i < exemptPos; ++i)
		{
			if (set[i] == sum)
			{
				if (i != depth) std::swap(set[i], set[depth]);
				int* tempSet = new int[setSize];
				std::copy(set, set + setSize, tempSet);
				perm(tempSet, segmentStart, segmentLength, segmentLength, level, horizontal);
				delete[] tempSet;
				break;
			}
		}
	}
	else
	{
		if (set[depth] < sum) comb(set, segmentStart, segmentLength, depth + 1, exemptPos, sum - set[depth], level, horizontal);
		//Iterates backwards from the exempt pos to find an element that works
		while (exemptPos > segmentStart + segmentLength)
		{
			--exemptPos;
			if (set[exemptPos] < sum)
			{
				std::swap(set[exemptPos], set[depth]);
				comb(set, segmentStart, segmentLength, depth + 1, exemptPos, sum - set[depth], level, horizontal);
			}
		}
	}
}

void generateShellPartials(int set[], std::vector<int*>& shellPartials, int depth, int exemptPos, int sum)
{
	if (depth == sideLength - 1)
	{
		for (int i = depth; i < exemptPos; ++i)
		{
			if (set[i] == sum)
			{
				if (i != depth) std::swap(set[i], set[depth]);
				int* partial = new int[sideLength - 1];
				std::copy(set + 1, set + sideLength, partial);
				shellPartials.push_back(partial);
				break;
			}
		}
	}
	else
	{
		if (set[depth] < sum) generateShellPartials(set, shellPartials, depth + 1, exemptPos, sum - set[depth]);
		while (exemptPos > sideLength)
		{
			--exemptPos;
			if (set[exemptPos] < sum)
			{
				std::swap(set[exemptPos], set[depth]);
				generateShellPartials(set, shellPartials, depth + 1, exemptPos, sum - set[depth]);
			}
		}
	}
}

void outerShell(int set[])
{
	int* tempSet = new int[setSize];
	std::copy(set, set + setSize, tempSet);
	std::vector<int*> shellPartials;
	generateShellPartials(tempSet, shellPartials, 1, setSize, originalSum - 1);
	delete[] tempSet;
	std::vector<int*> shellSets;
	for (int i = 0; i < shellPartials.size(); ++i)
	{
		int* partial1 = shellPartials[i];
		for (int j = i + 1; j < shellPartials.size(); ++j)
		{
			int* partial2 = shellPartials[j];
			bool collision = false;
			for (int k = 0; k < sideLength - 1; ++k)
			{
				for (int l = 0; l < sideLength - 1; ++l)
				{
					if (partial1[k] == partial2[l])
					{
						collision = true;
						break;
					}
				}
				if (collision) break;
			}
			if (!collision)
			{
				tempSet = new int[setSize];
				std::copy(set, set + setSize, tempSet);
				for (int k = 0; k < sideLength - 1; ++k)
				{
					std::swap(tempSet[k + 1], tempSet[partial1[k] - 1]); // std::swaps value i by accessing index i - 1
					std::swap(tempSet[sideLength + k], tempSet[partial2[k] - 1]);
				}
				shellSets.push_back(tempSet);
			}
		}
	}

	int** sets = shellSets.data();
	int setCount = shellSets.size();
	double progressIncrement = setCount / 10.0;
	int progressCounter = 1;
	for (int i = 0; i < setCount; ++i)
	{
		int* currSet = sets[i];
		int segmentStart = 2 * sideLength - 1;
		comb(currSet, segmentStart, sideLength - 1, segmentStart, setSize, originalSum - currSet[sideLength], 1, true);
		if (sideLength != 3 && (i / (double)setCount) > progressCounter / 10.0)
		{
			printf("%d%%\tSquare count: %d\t", progressCounter * 10, squareCount);
			printTime(startTime, clock());
			++progressCounter;
		}
	}
	delete[] tempSet;
}

void makeSquares(int c)
{
	// Static variable intialisation
	sideLength = c;
	originalSum = (pow(c, 3) + c) / 2;
	setSize = pow(c, 2);
	initialiseConvSet();
	initialisePermSet();
	initialiseRotSet();
	int* set = new int[setSize];
	//std::vector<int> set(setSize);
	for (int i = 0; i < setSize; ++i)
	{
		set[i] = i + 1;
	}
	startTime = clock();
	outerShell(set);
	std::cout << "Total: " << squareCount << std::endl;
	// Four possible rotations
	std::cout << "Rotationally unique: " << squareCount / 4 << std::endl;
	// std::swapping the order of the rows and/or columns preserves the magic sqaure, and so
	// there are (c!)^2 possible row and column permutations
	std::cout << "Compositionally unique: " << squareCount / pow(fact(c), 2) << std::endl;
	// Permuting the rows and columns elminates half the possible rotations, hence the
	// count is only decreased by half
	std::cout << "Rotationally and Compositionally unique: " << squareCount / (pow(fact(c), 2) * 2) << std::endl;
	printTime(startTime, clock());
	delete[] set;
	delete[] convSet;
	for (int i = 0; i < permSetSize; ++i) delete[] permSet[i];
	delete[] permSet;
}

int main()
{
	int c;
	std::cout << "Sidelength: ";
	std::cin >> c;
	makeSquares(c);
	return 0;
}