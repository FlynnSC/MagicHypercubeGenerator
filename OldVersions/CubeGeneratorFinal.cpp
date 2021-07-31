#include <vector>
#include <time.h>
#include <iostream>
#include <fstream>
#include <cmath>

using namespace std;

void printTime(clock_t t1, clock_t t2)
{
	double secs = double(t2 - t1) / CLOCKS_PER_SEC;
	int mins = floor(secs / 60);
	secs = secs - mins * 60;
	cout << "Time: ";
	if (mins < 10) cout << "0";
	cout << mins << ":";
	if (secs < 10) cout << "0";
	cout << secs << endl;
}

int fact(int n)
{
	for (int i = n - 1; i > 1; --i)
	{
		n *= i;
	}
	return n;
}

void printVector(vector<int>& set)
{
	for (int x : set)
	{
		cout << x << " ";
	}
	cout << endl;
}

void printSet(int set[], int length)
{
	for (int i = 0; i < length; ++i)
	{
		cout << set[i] << " ";
	}
	cout << endl;
}

struct Info
{
	Info(int segmentIndex, int segmentStart, int segmentLength, bool firstAxis, int planeIndex, int planeLevel, int cubeLevel)
		: segmentIndex{ segmentIndex }, segmentStart{ segmentStart }, segmentLength{ segmentLength }, firstAxis{ firstAxis },
		planeIndex{ planeIndex }, planeLevel{ planeLevel }, cubeLevel{ cubeLevel }{}
	Info() {}

	int segmentIndex;
	int segmentStart;
	int segmentLength;
	bool firstAxis;
	int planeIndex;
	int planeLevel;
	int cubeLevel;
};

static clock_t startTime;
static unsigned long cubeCount = 0;
static int sideLength;
static int planeSize;
static int setSize;
static int originalSum;
static int* convSet; // Converts between shell index and square position
static int** permSet; // Set containing all possible permutations of size sideLength
static int permSetSize;
static Info* segmentInfo;

void initialiseConvSet()
{
	convSet = new int[setSize];

	// Iterates through the cube's diagonal level
	int curr = 0;
	for (int i = 0; i < sideLength; ++i)
	{
		// Iterates through the xy plane's diagonal level
		for (int j = i; j < sideLength; ++j)
		{
			// Fills in x segment
			for (int k = j; k < sideLength; ++k)
			{
				convSet[(int)pow(sideLength, 2) * i + sideLength * j + k] = curr;
				++curr;
			}
			// Fills in y segment
			for (int k = j + 1; k < sideLength; ++k)
			{
				convSet[(int)pow(sideLength, 2) * i + sideLength * k + j] = curr;
				++curr;
			}
		}
		// Iterates through the yz plane's diagonal level
		for (int j = i; j < sideLength; ++j)
		{
			// Skips first y segment
			if (j != i)
			{
				// Fills in y segment
				for (int k = j; k < sideLength; ++k)
				{
					convSet[(int)pow(sideLength, 2) * j + sideLength * k + i] = curr;
					++curr;
				}
			}
			// Fills in z segment
			for (int k = j + 1; k < sideLength; ++k)
			{
				convSet[(int)pow(sideLength, 2) * k + sideLength * j + i] = curr;
				++curr;
			}
		}
		// Iterates through the xz plane's diagonal level
		// Skips the first x segment and first z segment
		for (int j = i + 1; j < sideLength; ++j)
		{
			// Fills in x segment
			for (int k = j; k < sideLength; ++k)
			{
				convSet[(int)pow(sideLength, 2) * j + sideLength * i + k] = curr;
				++curr;
			}
			// Fills in z segment
			for (int k = j + 1; k < sideLength; ++k)
			{
				convSet[(int)pow(sideLength, 2) * k + sideLength * i + j] = curr;
				++curr;
			}
		}
	}
}

void permSetPerm(int set[], int length, int& permIndex)
{
	if (length == 1)
	{
		permSet[permIndex] = new int[sideLength];
		copy(set, set + sideLength, permSet[permIndex]);
		++permIndex;
	}
	else
	{
		permSetPerm(set, length - 1, permIndex);
		for (int i = 0; i < length - 1; i++)
		{
			if (length % 2 == 1) swap(set[0], set[length - 1]);
			else swap(set[i], set[length - 1]);
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

void initialiseSegmentInfo()
{
	vector<Info> infoSet;
	infoSet.push_back(Info(0, 2 * sideLength - 1, sideLength - 1, true, 0, 1, 0));
	int segmentIndex = 1;
	bool done = false;

	while (!done)
	{
		infoSet.push_back(Info());
		Info& curr = infoSet[segmentIndex];
		Info& prev = infoSet[segmentIndex - 1];
		curr.segmentIndex = segmentIndex;
		curr.segmentStart = prev.segmentStart + prev.segmentLength;
		curr.segmentLength = prev.firstAxis ? prev.segmentLength - 1 : prev.segmentLength;
		curr.firstAxis = !prev.firstAxis;
		curr.planeIndex = prev.planeIndex;
		curr.planeLevel = prev.firstAxis ? prev.planeLevel : prev.planeLevel + 1;
		curr.cubeLevel = prev.cubeLevel;

		// Transition between planes
		if (prev.planeLevel == sideLength - 1)
		{
			switch (prev.planeIndex)
			{
			case 0:
				curr.planeIndex = 1;
				curr.planeLevel = curr.cubeLevel;
				curr.segmentLength = sideLength - curr.planeLevel - 1;
				curr.firstAxis = false;

				// Accounts for z axis solidification
				if (curr.cubeLevel == 0)
				{
					curr.segmentStart += sideLength - 1;
					curr.firstAxis = true;
					++curr.planeLevel;
				}
				break;
			case 1:
				curr.planeIndex = 2;
				curr.planeLevel = curr.cubeLevel + 1;
				curr.segmentLength = sideLength - curr.planeLevel;
				curr.firstAxis = true;
				break;
			case 2:
				++curr.cubeLevel;
				curr.planeIndex = 0;
				curr.planeLevel = curr.cubeLevel;
				curr.segmentLength = sideLength - curr.planeLevel;
				curr.firstAxis = true;
				break;
			}
		}

		++segmentIndex;
		if (curr.cubeLevel == sideLength - 1) done = true;
	}

	segmentInfo = new Info[infoSet.size()];
	copy(infoSet.data(), infoSet.data() + infoSet.size(), segmentInfo);
}

void printCube(int set[])
{
	static ofstream ofs("Magic Cubes.txt");
	// Explanation:
	// REDO

	int scales[3];
	scales[0] = planeSize;
	scales[1] = sideLength;
	scales[2] = 1;

	// z axis perms
	for (int i = 0; i < permSetSize; ++i)
	{
		// y axis perms
		for (int j = 0; j < permSetSize; ++j)
		{
			// x axis perms
			for (int k = 0; k < permSetSize; ++k)
			{
				// non-perm transformations using coordinate swapping
				int n = fact(3); // 3 is dimensionality not sideLength
				for (int l = 0; l < n; ++l)
				{
					// z axis
					for (int m = 0; m < sideLength; ++m)
					{
						int offset1 = permSet[i][m] * scales[permSet[l][0]];

						// y axis
						for (int n = 0; n < sideLength; ++n)
						{
							int offset2 = permSet[j][n] * scales[permSet[l][1]];

							// x axis
							for (int o = 0; o < sideLength; ++o)
							{
								int offset3 = permSet[k][o] * scales[permSet[l][2]];
								ofs << set[convSet[offset1 + offset2 + offset3]];
								if (o != sideLength - 1) ofs << "\t";
							}
							ofs << "\n";
						}
						ofs << "\n";
					}
					ofs << "\n";
				}
			}
		}
	}

	//printCube(set);
	cubeCount += pow(permSetSize, 3) * fact(3);
}

void comb(int set[], int depth, int exemptPos, int sum, const Info& info);

void perm(int set[], int length, const Info& info)
{
	if (length == 1)
	{
		Info& newInfo = segmentInfo[info.segmentIndex + 1];
		int newSum = originalSum;
		switch (newInfo.planeIndex)
		{
		case 0:
			if (newInfo.firstAxis)
			{
				for (int i = 0; i < newInfo.planeLevel; ++i)
				{
					newSum -= set[convSet[planeSize * newInfo.cubeLevel + sideLength * newInfo.planeLevel + i]];
				}
			}
			else
			{
				for (int i = 0; i < newInfo.planeLevel + 1; ++i)
				{
					newSum -= set[convSet[planeSize * newInfo.cubeLevel + sideLength * i + newInfo.planeLevel]];
				}
			}
			break;
		case 1:
			if (newInfo.firstAxis)
			{
				for (int i = 0; i < newInfo.planeLevel; ++i)
				{
					newSum -= set[convSet[planeSize * newInfo.planeLevel + sideLength * i + newInfo.cubeLevel]];
				}
			}
			else
			{
				for (int i = 0; i < newInfo.planeLevel + 1; ++i)
				{
					newSum -= set[convSet[planeSize * i + sideLength * newInfo.planeLevel + newInfo.cubeLevel]];
				}
			}
			break;
		case 2:
			if (newInfo.firstAxis)
			{
				for (int i = 0; i < newInfo.planeLevel; ++i)
				{
					newSum -= set[convSet[planeSize * newInfo.planeLevel + sideLength * newInfo.cubeLevel + i]];
				}
			}
			else
			{
				for (int i = 0; i < newInfo.planeLevel + 1; ++i)
				{
					newSum -= set[convSet[planeSize * i + sideLength * newInfo.cubeLevel + newInfo.planeLevel]];
				}
			}
			break;
		}
		if (newSum > 0) comb(set, newInfo.segmentStart, setSize, newSum, newInfo);
	}
	else
	{
		perm(set, length - 1, info);
		for (int i = 0; i < length - 1; i++)
		{
			if (length % 2 == 1) swap(set[info.segmentStart], set[info.segmentStart + length - 1]);
			else swap(set[info.segmentStart + i], set[info.segmentStart + length - 1]);
			perm(set, length - 1, info);
		}
	}
}

void comb(int set[], int depth, int exemptPos, int sum, const Info& info)
{
	if (depth == info.segmentStart + info.segmentLength - 1)//Last position in segment
	{
		if (info.cubeLevel == sideLength - 1) printCube(set);
		else
		{
			bool valid = true;

			// Ensures both axes sums match
			if (info.planeLevel == sideLength - 1)
			{
				int tempSum = originalSum;
				switch (info.planeIndex)
				{
				case 0:
					for (int i = 0; i < info.planeLevel; ++i)
					{
						tempSum -= set[convSet[planeSize * info.cubeLevel + sideLength * i + info.planeLevel]];
					}
					break;
				case 1:
					for (int i = 0; i < info.planeLevel; ++i)
					{
						tempSum -= set[convSet[planeSize * i + sideLength * info.planeLevel + info.cubeLevel]];
					}
					break;
				case 2:
					for (int i = 0; i < info.planeLevel; ++i)
					{
						tempSum -= set[convSet[planeSize * i + sideLength * info.cubeLevel + info.planeLevel]];
					}
					break;
				}
				valid = tempSum == sum;
			}

			if (valid)
			{
				//Finds the element to complete the sum, then perms
				for (int i = depth; i < exemptPos; ++i)
				{
					if (set[i] == sum)
					{
						if (i != depth) swap(set[i], set[depth]);
						int* tempSet = new int[setSize];
						copy(set, set + setSize, tempSet);
						perm(tempSet, info.segmentLength, info);
						delete[] tempSet;
						break;
					}
				}
			}
		}
	}
	else
	{
		if (set[depth] < sum) comb(set, depth + 1, exemptPos, sum - set[depth], info);
		//Iterates backwards from the exempt pos to find an element that works
		while (exemptPos > info.segmentStart + info.segmentLength)
		{
			--exemptPos;
			if (set[exemptPos] < sum)
			{
				swap(set[exemptPos], set[depth]);
				comb(set, depth + 1, exemptPos, sum - set[depth], info);
			}
		}
	}
}

void generateShellPartials(int set[], vector<int*>& shellPartials, int depth, int exemptPos, int sum)
{
	if (depth == sideLength - 1)
	{
		for (int i = depth; i < exemptPos; ++i)
		{
			if (set[i] == sum)
			{
				if (i != depth) swap(set[i], set[depth]);
				int* partial = new int[sideLength - 1];
				copy(set + 1, set + sideLength, partial);
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
				swap(set[exemptPos], set[depth]);
				generateShellPartials(set, shellPartials, depth + 1, exemptPos, sum - set[depth]);
			}
		}
	}
}

void axisSolidification(int set[])
{
	int* tempSet = new int[setSize];
	copy(set, set + setSize, tempSet);
	vector<int*> shellPartials;
	generateShellPartials(tempSet, shellPartials, 1, setSize, originalSum - 1);
	delete[] tempSet;
	vector<int*> shellSets;
	for (int i = 0; i < shellPartials.size(); ++i)
	{
		int* partial1 = shellPartials[i];
		for (int j = i + 1; j < shellPartials.size(); ++j)
		{
			int* partial2 = shellPartials[j];
			for (int k = j + 1; k < shellPartials.size(); ++k)
			{
				int* partial3 = shellPartials[k];
				bool collision = false;
				for (int l = 0; l < sideLength - 1; ++l)
				{
					for (int m = 0; m < sideLength - 1; ++m)
					{
						if (partial1[l] == partial2[m])
						{
							collision = true;
							break;
						}
						for (int n = 0; n < sideLength; ++n)
						{
							if (partial1[l] == partial3[n] || partial2[m] == partial3[n])
							{
								collision = true;
								break;
							}
						}
						if (collision) break;
					}
					if (collision) break;
				}
				if (!collision)
				{
					tempSet = new int[setSize];
					copy(set, set + setSize, tempSet);
					for (int l = 0; l < sideLength - 1; ++l)
					{
						// Swaps value i by accessing index i - 1
						// x axis
						swap(tempSet[l + 1], tempSet[partial1[l] - 1]);
						// y axis
						swap(tempSet[sideLength + l], tempSet[partial2[l] - 1]);
						// z axis
						swap(tempSet[planeSize + l], tempSet[partial3[l] - 1]);
					}
					shellSets.push_back(tempSet);
				}
			}
		}
	}
	cout << "Shell sets generated..." << endl;
	printTime(startTime, clock());

	int** sets = shellSets.data();
	int setCount = shellSets.size();
	double progressIncrement = setCount / 10.0;
	int progressCounter = 1;
	for (int i = 0; i < setCount; ++i)
	{
		int* currSet = sets[i];
		comb(currSet, segmentInfo[0].segmentStart, setSize, originalSum - currSet[sideLength], segmentInfo[0]);
		if ((i / (double)setCount) > progressCounter / 10.0)
		{
			printf("%d%%\tSquare count: %lu\t", progressCounter * 10, cubeCount);
			printTime(startTime, clock());
			++progressCounter;
		}
	}
	delete[] tempSet;
}

// x = width (left to right), y = height (top to bottom) and z = depth (front to back)
// Shell filled: xy plane -> yz plane -> xz plane
// xy: x -> y
// yz: y -> z (xy plane fills first y segment, so its effectively z -> y)
// xz: x -> z (xy plane fills first x segment, yz plane fills first z segment)

void makeCubes(int c)
{
	// Static variable initialisation
	sideLength = c;
	planeSize = pow(c, 2);
	originalSum = (pow(c, 4) + c) / 2;
	setSize = pow(c, 3);
	initialiseConvSet();
	initialisePermSet();
	initialiseSegmentInfo();
	int* set = new int[setSize];
	for (int i = 0; i < setSize; ++i)
	{
		set[i] = i + 1;
	}
	startTime = clock();
	axisSolidification(set);
	cout << "Total: " << cubeCount << endl;
	// There are permSetSize permutations for each axis and fact(3) axis orderings (coordinate swaps)
	cout << "Transformationally unique: " << cubeCount / (pow(permSetSize, 3) * fact(3)) << endl;
	printTime(startTime, clock());
	delete[] set;
	delete[] convSet;
	for (int i = 0; i < permSetSize; ++i) delete[] permSet[i];
	delete[] permSet;
}

int main()
{
	cout << "Magic cube generator" << endl;
	cout << "Enter sidelength: ";
	int c;
	cin >> c;
	makeCubes(c);
	return 0;
}