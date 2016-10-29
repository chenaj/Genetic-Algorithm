#include "draw.h"
#include <opencv2/imgproc/imgproc.hpp>
using namespace cv;
using namespace std;

Mat renderPolyImage(int imHeight, int imWidth, int numPolygons,
	const Point** polys, const int* vertexCounts, const Scalar* cols) {

	Mat polyIm = Mat::zeros(imHeight, imWidth, CV_8UC3); // Create a black background image
	for (int i = 0; i < numPolygons; ++i) {
		Mat tempClone = polyIm.clone();
		const Point** thisPoly = polys + i;
		const int* thisVertexCount = vertexCounts + i;
         fillPoly(
			tempClone,
			thisPoly,
			thisVertexCount,
			1, // One contour
			cols[i] //The color for that polygon
			);
		double alpha = cols[i].val[3]; //3 is the alpha index

									   // Add tempClone to polyIm with transparency alpha, store result in polyIm
		addWeighted(tempClone, alpha, polyIm, 1 - alpha, 0, polyIm);
	}

	return polyIm;
}


double score(const Mat& polyImage, const Mat& referenceImage) {
	return -log(norm(polyImage, referenceImage) / (norm(polyImage) + norm(referenceImage)));
}