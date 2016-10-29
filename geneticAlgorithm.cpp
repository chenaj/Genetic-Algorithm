// example.cpp
/*
Compile with:
g++ -std=c++11 `pkg-config --cflags opencv` -o example example.cpp draw.cpp `pkg-config --libs opencv`
*/
#include "draw.h" // renderPolyImage(), score()
#include <opencv2/core/core.hpp> // Mat, Point, Scalar
#include <opencv2/highgui/highgui.hpp> // imread(), imwrite(), imshow()
#include <random> //default_random_engine, uniform_int_distribution<>, uniform_real_distribution<>
#include <iostream> // cerr, cout
#include <sstream> // ostringstream
#include <fstream>
#include <string> // string
#include <vector>
using namespace cv;
using namespace std;

class CurrentState
{
public:
	double c_imScore;
	int c_numPolygons;
	int c_vertexCounts[100];
	Scalar c_colors[100];
	Point c_polyArr[100][6];
	const Point* c_polygons[100];

	CurrentState()
	{}
	void setState(double n_imScore, int n_numPolygons, Point n_polyArr[100][6], int n_vertexCounts[100], Scalar n_colors[100])
	{
		c_imScore = n_imScore;
		c_numPolygons = n_numPolygons;
		for (int i = 0; i < c_numPolygons; i++)
		{
			c_vertexCounts[i] = n_vertexCounts[i];
			for (int j = 0; j < c_vertexCounts[i]; j++)
				c_polyArr[i][j] = n_polyArr[i][j];
			c_colors[i] = n_colors[i];
			c_polygons[i] = &c_polyArr[i][0];
		}
	}
};

//ofstream file;
	Mat tempImage;
	Mat referenceImage;
class Population
{
	int vertexCounts[100];
	Point polyArr[100][6];
	const Point* polygons[100]; //Array of constant pointers to arrays of Points
	Scalar colors[100];
	Mat polyImage;
	CurrentState currImg;
	vector<CurrentState> states;
	int numPolygons;

public:
	Population(char** argv)
	{
		// Interpret argv[1] as an image file name,
		// read in the file using the OpenCV imread funciton
		referenceImage = imread(argv[1]);

		// Create a random generator, seed it with the value 1
		// Found in the c++ standard library <random>
		default_random_engine randEngine(1);

		// Create two uniform distributions, one for randomly generating
		// 	x-values of vertices, one for randomly generating y-values of vertices,
		//	within the bounds of the image.
		uniform_int_distribution<int> xValGen(0, referenceImage.cols - 1);
		uniform_int_distribution<int> yValGen(0, referenceImage.rows - 1);

		// Create distributions for generating color and opacity values
		uniform_int_distribution<int> BGRgen(0, 255);
		uniform_real_distribution<double> alphaGen(0, 1);

		//Setting number of vertices to 3
		for (int i = 0; i < 100; i++) vertexCounts[i] = 3;

		for (int i = 0; i < 100; i++) {
			for (int j = 0; j < vertexCounts[i]; j++) {
				//Initialize each point randomly
				polyArr[i][j] = Point(xValGen(randEngine), yValGen(randEngine));
			}
		}

		for (int i = 0; i < 100; i++) {
			// Initialize each pointer to point to a polygon
			polygons[i] = &polyArr[i][0];
		}


		for (int i = 0; i < 100; i++) {
			// Randomly initialize each color value
			colors[i] = Scalar(BGRgen(randEngine), BGRgen(randEngine),
				BGRgen(randEngine), alphaGen(randEngine));
		}


		polyImage = renderPolyImage(
			referenceImage.cols,
			referenceImage.rows,
			100,
			polygons,
			vertexCounts,
			colors
			);

		// Display the reference image and the rendered polygon image during runtime
		imshow("Reference Image", referenceImage);
		imshow("PolyImage", polyImage);

		// Score the randomly generated image
		double polyImScore = score(polyImage, referenceImage);
		currImg.setState(polyImScore, 100, polyArr, vertexCounts, colors);
		cout << "Fitness score of an image of randomly generated polygons: ";
		cout << polyImScore << '\n';

		numPolygons = 100;

		// Create the ouput file name
		ostringstream fileOut;
		fileOut << "PolyImage_" << polyImScore << ".bmp";
		string filename = fileOut.str();

		// Use the OpenCV imwrite function to write the generated image to a file
		imwrite(filename, polyImage); //Extension determines write format
		cout << "Wrote image file " << filename << '\n';
	}
	double rand_normal(double mean, double stddev)
	{//Box muller method
		static double n2 = 0.0;
		static int n2_cached = 0;
		if (!n2_cached)
		{
			double x, y, r;
			do
			{
				x = 2.0*rand() / RAND_MAX - 1;
				y = 2.0*rand() / RAND_MAX - 1;

				r = x*x + y*y;
			} while (r == 0.0 || r > 1.0);
			{
				double d = sqrt(-2.0*log(r) / r);
				double n1 = x*d;
				n2 = y*d;
				double result = n1*stddev + mean;
				n2_cached = 1;
				return result;
			}
		}
		else
		{
			n2_cached = 0;
			return n2*stddev + mean;
		}
	}
	void mutation()
	{
		for (int i = 0; i < numPolygons; i++)
		{
			int prob = rand() % 5;
			//cout << "Probability: " << prob;
			if (prob<3)			// 3/5 chance mutatating existing poly
			{
				//cout << " " << prob;
				prob = rand() % 2;		//1/2 probablity of altering color slightly
				if (prob == 1)
				{
					colorMutation(i);
				}//cout << " " << prob;
				prob = rand() % 2;		//1/2 chance of changing shape
				if (prob == 1)
				{
					prob = rand() % 3;		//1/3 chance of adding vertex
					if (prob == 1)
						addVertexMutation(i);
					prob = rand() % 3;
					if (prob == 1)			//1/3 chance of subtracting a vertex
						subVertexMutation(i);
					prob = rand() % 3;
					if (prob == 1)			//1/3 chance of moving a vertex
						moveMutation(i);
				}
			}
			//cout << " " << prob;
			prob = rand() % 5;	//1/5 chance of being replaced by a random triangle
			if (prob == 1)
			{
				newPolyMutation(i);
			}
			prob = rand() % 5;	//1/5 chance of being replaced by a random triangle
								//cout << " " << prob<<endl;
			if (prob == 1)
			{
				listMutation(i);
			}

		}
		drawIteration();
		cout << "Current Image Score: " << currImg.c_imScore << " Next Score: " << score(tempImage, referenceImage) << endl;
		if (currImg.c_imScore < score(tempImage, referenceImage)) {
			currImg.setState(score(tempImage, referenceImage), numPolygons, polyArr, vertexCounts, colors);
			states.push_back(currImg);
			cout << "New State added" << endl;
			drawPolyImage();
			getResults();
		}
		else
		{
			for (int j = 0; j < numPolygons; j++)	//revert to fitter mutation image
			{
				vertexCounts[j] = currImg.c_vertexCounts[j];
				for (int k = 0; k < vertexCounts[j]; k++)
					polyArr[j][k] = currImg.c_polyArr[j][k];
				polygons[j] = &polyArr[j][0];
				colors[j] = currImg.c_colors[j];
			}
		}
		/*if (currImg.c_imScore > score(polyImage, referenceImage))
		{
		for (int j = 0; j < numPolygons; j++)
		{
		vertexCounts[j] = currImg.c_vertexCounts[j];
		for (int k = 0; k < vertexCounts[j]; k++)
		polyArr[j][k] = currImg.c_polyArr[j][k];
		polygons[j] = &polyArr[j][0];
		colors[j] = currImg.c_colors[j];
		}
		} */
		/*else
		{
		currImg.setState(score(polyImage, referenceImage), numPolygons, polyArr, vertexCounts, colors);
		}*/

	}
	void listMutation(int polygon)	//change ordering in polygons list
	{
		const Point* tempPoly = polygons[polygon];
		int prob = rand_normal(0, 0.5);
		int newPosition = polygon + prob;
		//cout << "New position: " << newPosition;
		if (newPosition < numPolygons && newPosition >= 0) {
			polygons[polygon] = polygons[newPosition];
			polygons[newPosition] = tempPoly;
		}
	}
	void newPolyMutation(int polygon)	//replace existing polygon with random triangle
	{
		vertexCounts[polygon] = 3;
		for (int j = 0; j < vertexCounts[polygon]; j++) {
			//Initialize each point randomly
			int xValGen = rand() % referenceImage.cols;
			int yValGen = rand() % referenceImage.rows;
			polyArr[polygon][j] = Point(xValGen, yValGen);
		}
		colorMutation(polygon);
	}
	void colorMutation(int polygon)	//changing the scalar values of colors array
	{

		double prob = rand_normal(-0.025, 5.0);	//change hues
												//file << prob<<",";
		double newColor = colors[polygon].val[0] + prob;
		if (newColor <= 255 && newColor >= 0)
			colors[polygon].val[0] = newColor;
		prob = rand_normal(-0.025, 5.0);
		//file << prob<<",";
		newColor = colors[polygon].val[1] + prob;
		if (newColor <= 255 && newColor >= 0)
			colors[polygon].val[1] = newColor;
		//file << prob << ",";
		prob = rand_normal(-0.025, 5.0);
		newColor = colors[polygon].val[2] + prob;
		if (newColor <= 255 && newColor >= 0)
			colors[polygon].val[2] = newColor;
		//file <<prob<< ",";
		prob = rand_normal(-0.0025, 5.0);		//change brightness
		newColor = colors[polygon].val[0] + prob;
		if (newColor <= 255 && newColor >= 0) {
			colors[polygon].val[0] += prob;
			newColor = colors[polygon].val[1] + prob;
			if (newColor <= 255 && newColor >= 0) {
				colors[polygon].val[1] += prob;
				newColor = colors[polygon].val[2] + prob;
				if (newColor <= 255 && newColor >= 0)
					colors[polygon].val[2] += prob;
			}
		}

		double prob2 = rand_normal(0, 0.01);
		//file << prob2 << ",";
		double newAlpha = prob2 + colors[polygon].val[3];
		if (newAlpha <= 1.0 && newAlpha >= 0.0)
			colors[polygon].val[3] = newAlpha;
	}
	void moveMutation(int polygon)
	{
		int selectVertex = rand() % vertexCounts[polygon];
		double prob = rand_normal(-0.025, 5.0);
		polyArr[polygon][selectVertex].x = polyArr[polygon][selectVertex].x + prob;
		prob = rand_normal(-0.025, 5.0);
		polyArr[polygon][selectVertex].y = polyArr[polygon][selectVertex].y + prob;
	}
	void addVertexMutation(int polygon) {
		if (vertexCounts[polygon] < 6)
		{
			vertexCounts[polygon]++;
		}
		int xValGen = rand() % referenceImage.cols;
		int yValGen = rand() % referenceImage.rows;
		polyArr[polygon][vertexCounts[polygon] - 1] = Point(xValGen, yValGen);
		//TODO***********************************************************************************
	}
	void subVertexMutation(int polygon) {
		if (vertexCounts[polygon]>3)
			vertexCounts[polygon]--;
	}//TODO***********************************************************************************
	CurrentState inheritance() {
		return currImg;
	}
	void inherited(CurrentState parent1, CurrentState parent2)
	{
		int randomCrossOver = rand() % 100;	//select C polygons between 0-99 as midpoint for crossover
		for (int i = 0; i < 100; i++)
		{
			if (i < randomCrossOver) {
				currImg.c_vertexCounts[i] = parent1.c_vertexCounts[i];
				currImg.c_colors[i] = parent1.c_colors[i];
				for (int j = 0; j < currImg.c_vertexCounts[i]; j++)
				{
					currImg.c_polyArr[i][j] = parent1.c_polyArr[i][j];
				}
				currImg.c_polygons[i] = &currImg.c_polyArr[i][0];
			}
			else {
				currImg.c_vertexCounts[i] = parent2.c_vertexCounts[i];
				currImg.c_colors[i] = parent2.c_colors[i];
				for (int j = 0; j < currImg.c_vertexCounts[i]; j++)
				{
					currImg.c_polyArr[i][j] = parent2.c_polyArr[i][j];
				}
				currImg.c_polygons[i] = &currImg.c_polyArr[i][0];
			}
		}
	}
	double getFitnessScore()
	{
		return score(polyImage, referenceImage);
	}
	void drawIteration()
	{
		tempImage = renderPolyImage(
			referenceImage.cols,
			referenceImage.rows,
			numPolygons,
			polygons,
			vertexCounts,
			colors
			);

		imshow("TempImage", tempImage);
		// Score the randomly generated image
		double polyImScore = score(polyImage, referenceImage);
		cout << "Fitness score of mutations: ";
		cout << polyImScore << '\n';
		//Another openCV function that pauses execution until the user presses the given key
		//waitKey(0); 
	}
	void drawPolyImage()
	{
		int p_vertexCounts[100];
		Point p_polyArr[100][6];
		const Point* p_polygons[100]; //Array of constant pointers to arrays of Points
		Scalar p_colors[100];

		for (int j = 0; j < numPolygons; j++)
		{
			p_vertexCounts[j] = currImg.c_vertexCounts[j];
			for (int k = 0; k < vertexCounts[j]; k++)
				p_polyArr[j][k] = currImg.c_polyArr[j][k];
			p_polygons[j] = &polyArr[j][0];
			p_colors[j] = currImg.c_colors[j];
		}

		polyImage = renderPolyImage(
			referenceImage.cols,
			referenceImage.rows,
			numPolygons,
			p_polygons,
			p_vertexCounts,
			p_colors
			);
		/*int best = states.size();
		polyImage = renderPolyImage(
		referenceImage.cols,
		referenceImage.rows,
		numPolygons,
		states.end()->c_polygons,
		states.end()->c_vertexCounts,
		states.end()->c_colors
		);*/

		imshow("PolyImage", polyImage);
		getResults();
		// Score the randomly generated image
	}
	void getResults()
	{
		double polyImScore = score(polyImage, referenceImage);
		/*cout << "Fitness score of an image of randomly generated polygons: ";
		cout << polyImScore << '\n';*/

		// Create the ouput file name
		ostringstream fileOut;
		fileOut << "PolyImage_" << currImg.c_imScore << ".bmp";
		string filename = fileOut.str();

		// Use the OpenCV imwrite function to write the generated image to a file
		imwrite(filename, polyImage); //Extension determines write format
		cout << "Wrote image file " << filename << '\n';
	}

};

class GeneticAlgorithm
{
	int numOfParents;
	int numOfChildren;
	int population;
	int searchEffort;
	int fittestElement;
	int unfittestElement;
	int unfittestScore;
	int rand_partner;
	double fittestScore;
	vector<Population> popList;
public:
	GeneticAlgorithm(char** argv, int pop, int children, int effort)
	{
		numOfParents = pop;
		numOfChildren = children;
		searchEffort = effort;
		population = numOfParents + numOfChildren;
		for (int i = 0; i < population; i++)
		{
			popList.push_back(Population(argv));
		}
	}
	void chooseParentPair() {
		fittestElement = 0;
		fittestScore = popList[fittestElement].getFitnessScore();
		unfittestElement = 0;
		unfittestScore = popList[unfittestElement].getFitnessScore();
		for (int i = 1; i < population; i++) //find the best and worst fit in the list
		{
			if (popList[i].getFitnessScore()>fittestScore) {
				fittestScore = popList[i].getFitnessScore();
				fittestElement = i;
			}
			if (popList[i].getFitnessScore()<unfittestScore) {
				unfittestScore = popList[i].getFitnessScore();
				unfittestElement = i;
			}
		}
		do {
			rand_partner = rand() % population;	//randomly select the parent in the list
		} while (rand_partner == unfittestElement && population>2);	//excluding unfittest element
	}
	void crossover() {
		
		CurrentState childPart1=popList[fittestElement].inheritance();
		CurrentState childPart2=popList[rand_partner].inheritance();

		popList[unfittestElement].inherited(childPart1, childPart2);
		
	}
	int numGenerations() {
		return (searchEffort - numOfParents) / numOfChildren;
	}
	void mutate() {
		for (int i = 0; i < population; i++)
		{
			popList[i].mutation();
		}
	}
};


int main(int argc, char** argv) {
	if (argc < 2) {
		cerr << "Not enough arguments\n";
		return 0;
	}
	int populationNum, numOfChildren, effort;
	cout << "Please enter in population size (N) {1,2,4,8}: ";
	cin >> populationNum;
	cout << "Please enter in number of new children generated per generation (K) {1,2,4,8}: ";
	cin >> numOfChildren;
	cout << "Please enter search effort (E) {25000,50000} : ";
	cin >> effort;

	GeneticAlgorithm gAlgorithm(argv, populationNum, numOfChildren, effort);
	cout << "Generation Effort: " << gAlgorithm.numGenerations();
	for (int i = 0; i < gAlgorithm.numGenerations(); i++)
	{
		gAlgorithm.chooseParentPair();
		gAlgorithm.crossover();
		gAlgorithm.mutate();
	}

	/*Population parent(argv);
	if (populationNum == 1)
	{
		for (int i = 0; i < gAlgorithm.numGenerations(); i++)
		{
			parent.mutation();
		}
	}
	else if (populationNum == 2)
	{
		Population parent2(argv);
		for (int i = 0; i < gAlgorithm.numGenerations(); i++)
		{
			parent.mutation();
			parent2.mutation();
		}
	}
	else if (populationNum == 4) {
		Population parent2(argv);
		Population parent3(argv);
		Population parent4(argv);
		for (int i = 0; i < gAlgorithm.numGenerations(); i++) {
			parent.mutation();
			parent2.mutation();
			parent3.mutation();
			parent4.mutation();
		}
	}
	else if (populationNum == 8) {
		Population parent2(argv);
		Population parent3(argv);
		Population parent4(argv);
		Population parent5(argv);
		Population parent6(argv);
		Population parent7(argv);
		Population parent8(argv);
		for (int i = 0; i < gAlgorithm.numGenerations(); i++) {
			parent.mutation();
			parent2.mutation();
			parent3.mutation();
			parent4.mutation();
			parent5.mutation();
			parent6.mutation();
			parent7.mutation();
			parent8.mutation();
		}
	}*/

	return 0;
}