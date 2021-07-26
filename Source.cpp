#include <iostream>
#include "Generator.h"

using std::cout;
using std::endl;
using std::cin;

int main() {
	cout << "Magic cube generator" << endl;
	cout << "Enter sidelength: ";
	int sideLength;
	cin >> sideLength;
	cout << "Enter dimensionality: ";
	int dimensionality;
	cin >> dimensionality;
	cout << "Output will be saved to 'Magic Cubes.txt'. Output all cubes (a), identities only (i), or none (n): ";
	char choice;
	cin >> choice;
	PrintOption printOption;
	switch (choice) {
	case 'a':
		printOption = PrintOption::ALL;
		break;
	case 'i':
		printOption = PrintOption::IDENTITIES;
		break;
	default:
		printOption = PrintOption::NONE;
		break;
	}
	
	Generator generator(sideLength, dimensionality);
	generator.generate(printOption);
	return 0;
}
