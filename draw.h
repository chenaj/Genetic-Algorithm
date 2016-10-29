#pragma once
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <cmath>
using namespace cv;

#ifndef DRAW_H
#define DRAW_H

/***************
renderPolyImage
****************
	Takes a list of polygons and renders them into an image
	Returns:
		An OpenCV "Mat" object, a matrix specifying the image

	Arguments:
		int imWidth: Width of the image
		int imHeight: height of the image
		int numPolygons: The number of polygons being passed in
		const Point** polygons:
			A pointer to an array of pointers to arrays of OpenCV Points,
			Each array of points specifies the vertices of a polygon
		const int* vertexCounts:
			A pointer to an array of integers
			containing the number of vertices of each polygon
		const Scalar* colors:
			A pointer to an array of OpenCV Scalar objects
			Each object specifies the color and opacity of a polygon
			Each scalar object stores color information in BGRA format
			The BGR fields must be between 0 and 255
			The opacity field must be between 0 and 1
*/
Mat renderPolyImage(
	int imHeight,
	int imWidth,
	int numPolygons,
	const Point** polygons,
	const int* vertexCounts,
	const Scalar* colors
	);

/*****
score
******
	Returns:
		The fitness score of the polygon image as compared to the
		reference image.

	Arguments:
	polyImage: The image made of polygons
	referenceImage: The image we are trying to approximate

*/
double score(const Mat& polyImage, const Mat& referenceImage);

/*
Full example: Read in a reference image,
generate a random polygon image "polyImage",
score polyImage against the reference image,
display the score and polyImage,
and write polyImage to a file.
**********************
// example.cpp
#include "draw.h" // renderPolyImage(), score()
#include <opencv2/core/core.hpp> // Mat, Point, Scalar
#include <opencv2/highgui/highgui.hpp> // imread(), imwrite(), imshow()
#include <random> //default_random_engine, uniform_int_distribution<>, uniform_real_distribution<>
#include <iostream> // cerr, cout
#include <sstream> // ostringstream
#include <string> // string
using namespace cv;
using namespace std;

int main(int argc, char** argv) {
if (argc < 2) {
cerr << "Not enough arguments\n";
return 0;
}

// Interpret argv[1] as an image file name,
// read in the file using the OpenCV imread funciton
Mat referenceImage = imread(argv[1]);

// Create a random generator, seed it with the value 1
// Found in the c++ standard library <random>
default_random_engine randEngine(1);

// Create two uniform distributions, one for randomly generating
// 	x-values of vertices, one for randomly generating y-values of vertices,
//	within the bounds of the image.
uniform_int_distribution<int> xValGen(0, referenceImage.cols-1);
uniform_int_distribution<int> yValGen(0, referenceImage.rows-1);

// Create distributions for generating color and opacity values
uniform_int_distribution<int> BGRgen(0,255);
uniform_real_distribution<double> alphaGen(0,1);

int vertexCounts[100];
for (int i = 0; i < 100; i++) vertexCounts[i] = 3;

Point polyArr[100][6];
for (int i = 0; i < 100; i++) {
for (int j = 0; j < vertexCounts[i]; j++) {
//Initialize each point randomly
polyArr[i][j] = Point(xValGen(randEngine), yValGen(randEngine));
}
}
const Point* polygons[100];
//Array of constant pointers to arrays of Points
for (int i = 0; i < 100; i++) {
// Initialize each pointer to point to a polygon
polygons[i] = &polyArr[i][0];
}

Scalar colors[100];
for (int i = 0; i < 100; i++) {
// Randomly initialize each color value
colors[i] = Scalar(BGRgen(randEngine), BGRgen(randEngine),
BGRgen(randEngine), alphaGen(randEngine));
}

Mat polyImage = renderPolyImage(
referenceImage.cols,
referenceImage.rows,
100,
polygons,
vertexCounts,
colors
);

// Display the rendered polygon image during runtime
imshow("Reference Image", referenceImage);
imshow("PolyImage", polyImage);
//Another openCV function that pauses execution until the user presses the given key
waitKey(0);

// Score the randomly generated image
double polyImScore = score(polyImage, referenceImage);
cout << "Fitness score of an image of randomly generated polygons: ";
cout << polyImScore << '\n';

// Create the ouput file name
ostringstream fileOut;
fileOut << "PolyImage_" << polyImScore << ".jpg";
string filename = fileOut.str();

// Use the OpenCV imwrite function to write the generated image to a file
imwrite(filename, polyImage); //Extension determines write format
cout << "Wrote image file " << filename << '\n';

return 0;
}

************
Compile with:
g++ -std=c++11 `pkg-config --cflags opencv` -o example example.cpp draw.cpp `pkg-config --libs opencv`

Run on some image file "ref_im.jpg":
./example ref_im.jpg

Output to screen:
Fitness score of an image of randomly generated polygons: <score>
Wrote image file PolyImage_<score>.jpg

******************
You'll want to modify this example code to implement genetic search.
For example, you may want to wrap vertexCounts, polyArr, polygons, and colors
into a class that handles polygon images.
*/

#endif