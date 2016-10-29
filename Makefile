all: draw.h
	g++ -std=c++11 `pkg-config --cflags opencv` -o example example.cpp draw.cpp `pkg-config --libs opencv`

clean:
	rm example
	rm PolyImage*.jpg