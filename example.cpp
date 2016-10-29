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
#include <iomanip>
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

ofstream file;

class GeneticAlgorithm
{
	Mat referenceImage;			
	int vertexCounts[100];
	Point polyArr[100][6];
	const Point* polygons[100]; //Array of constant pointers to arrays of Points
	Scalar colors[100];
	Mat polyImage;
	Mat tempImage;
	CurrentState currImg;
	vector<CurrentState> states; //
	int numPolygons;
	int iteration;
	int generations;
public:
	GeneticAlgorithm(char** argv)
	{
		// Interpret argv[1] as an image file name,
		// read in the file using the OpenCV imread funciton
		referenceImage = imread(argv[1]);
		iteration = 0;
		generations = 0;
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
			referenceImage.rows,
			referenceImage.cols, 
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
		currImg.setState(polyImScore, 100,polyArr, vertexCounts, colors);
		cout << "Fitness score of an image of randomly generated polygons: ";
		cout << polyImScore << '\n';

		numPolygons = 100;

		// Create the ouput file name
		ostringstream fileOut;
		iteration += 1000;
		fileOut << setfill('0') << setw(8) << iteration<<".bmp";
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
	void inheritance(){
	}
	void crossover(){
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
				if (prob==1)
				{
					colorMutation(i);
				}//cout << " " << prob;
				prob = rand() % 2;		//1/2 chance of changing shape
				if (prob==1)	
				{
					prob = rand() % 3;		//1/3 chance of adding vertex
					if (prob==1)
						addVertexMutation(i);
					prob = rand() % 3;
					if (prob==1)			//1/3 chance of subtracting a vertex
						subVertexMutation(i);
					prob = rand() % 3;
					if (prob==1)			//1/3 chance of moving a vertex
						moveMutation(i);
				}
			}
			//cout << " " << prob;
			prob = rand() % 5;	//1/5 chance of being replaced by a random triangle
			if (prob==1)
			{
				newPolyMutation(i);
			}
						prob = rand() % 5;	//1/5 chance of being replaced by a random triangle
						//cout << " " << prob<<endl;
			if (prob==1)
			{
				listMutation(i);
			}
			
		}
		drawIteration();
		cout << "Current Image Score: " << currImg.c_imScore << " Next Score: "<< score(tempImage, referenceImage) << endl;
		if (currImg.c_imScore < score(tempImage, referenceImage)) {
			currImg.setState(score(tempImage, referenceImage), numPolygons, polyArr, vertexCounts, colors);
			states.push_back(currImg);
			cout << "New State added" << endl;
			drawPolyImage();
			getResults();
		}
		else
		{
			for (int j = 0; j < numPolygons; j++)
			{
				vertexCounts[j] = currImg.c_vertexCounts[j];
				for (int k = 0; k < vertexCounts[j]; k++)
					polyArr[j][k] = currImg.c_polyArr[j][k];
				polygons[j] = &polyArr[j][0];
				colors[j] = currImg.c_colors[j];
			}
		}
		file << (int)(generations*0.6666667) <<" "<<currImg.c_imScore<<endl;
		generations++;
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
		const Point* tempPoly=polygons[polygon];
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
		vertexCounts[polygon]=3;
		for (int j = 0; j < vertexCounts[polygon]; j++) {
				//Initialize each point randomly
			int xValGen=rand()%referenceImage.cols;
			int yValGen=rand()% referenceImage.rows;
				polyArr[polygon][j] = Point(xValGen, yValGen);
			}
		int r = rand() % 256;
		int b = rand() % 256;
		int g = rand() % 256;
		double a = (rand() % 100)/100.0;
		polygons[polygon] = &polyArr[polygon][0];
		colors[polygon] = Scalar(r, b, g, a);
	}
	void colorMutation(int polygon)	//changing the scalar values of colors array
	{
		
		double prob = rand_normal(-0.025, 5.0);	//change hues
		//file << prob<<",";
		double newColor= colors[polygon].val[0] + prob;
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
		int selectVertex =rand()%vertexCounts[polygon];
		double prob = rand_normal(-0.025, 5.0);
		polyArr[polygon][selectVertex].x = polyArr[polygon][selectVertex].x + prob;
		prob = rand_normal(-0.025, 5.0);
		polyArr[polygon][selectVertex].y = polyArr[polygon][selectVertex].y + prob;
	}
	void addVertexMutation(int polygon){
		if (vertexCounts[polygon]<6)
		{
			vertexCounts[polygon]++;
			int xValGen=rand()%referenceImage.cols;
			int yValGen=rand()% referenceImage.rows;
			polyArr[polygon][vertexCounts[polygon]-1] = Point(xValGen, yValGen);
		} //TODO***********************************************************************************
	}
	void subVertexMutation(int polygon){
		if (vertexCounts[polygon]>3)
			vertexCounts[polygon]--;
	}//TODO***********************************************************************************
	void drawIteration()
	{
		tempImage = renderPolyImage(
			referenceImage.rows,
			referenceImage.cols, 
			numPolygons, 
			polygons, 
			vertexCounts, 
			colors
			);

		imshow("TempImage", tempImage);
		// Score the randomly generated image
		double polyImScore = score(polyImage, referenceImage);
		cout << "Fitness score of an image of randomly generated polygons: ";
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
			referenceImage.rows,
			referenceImage.cols,
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
		//getResults();
		// Score the randomly generated image
	}
	void getResults()
	{
		double polyImScore = score(polyImage, referenceImage);
		/*cout << "Fitness score of an image of randomly generated polygons: ";
		cout << polyImScore << '\n';*/

		 //Create the ouput file name
		ostringstream fileOut;
		iteration += 1000;
		fileOut << setfill('0') << setw(8) << iteration << ".bmp";
		string filename = fileOut.str();

		// Use the OpenCV imwrite function to write the generated image to a file
		imwrite(filename, polyImage); //Extension determines write format
		cout << "Wrote image file " << filename << '\n';
	}

};



int main(int argc, char** argv) {
	if (argc < 2) {
		cerr << "Not enough arguments\n";
		return 0;
	}
	file.open("C:/Users/foobar/Documents/Visual Studio 2015/Projects/eecs492hw1/eecs492hw1/data.txt");
	GeneticAlgorithm gAlgorithm(argv);
	for (int i=0; i<75000; i++)
	{
             		gAlgorithm.mutation();
		//gAlgorithm.drawIteration();
		//gAlgorithm.getResults();
		//gAlgorithm.getResults();
	}
	file.close();
	

	return 0;
}