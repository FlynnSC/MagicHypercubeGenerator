.default: all

all: Cycle.o Generator.o Source.o
	g++ -std=c++2a -g -O -o magicHyperCubeGenerator $^ -pthread

%.o: %.cpp
	g++ -Wall -std=c++2a -g -O -c $^

clean:
	rm -rf magicHyperCubeGenerator *.o *.dSYM