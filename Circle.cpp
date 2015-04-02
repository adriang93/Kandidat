#include "stdafx.h"
#include "Circle.h"



pair<int,int> Circle::getCoords() {

	return pair<int, int>(xPos, yPos);

}

void Circle::FindCircles(Mat& src) {

	Mat src_gray;

	/// Convert it to gray
	cvtColor(src, src_gray, CV_BGR2GRAY);

	/// Reduce the noise so we avoid false circle detection
	GaussianBlur(src_gray, src_gray, Size(9, 9), 2, 2);

	vector<Vec3f> circles;

	/// Apply the Hough Transform to find the circles
	HoughCircles(src_gray, circles, CV_HOUGH_GRADIENT, 1, src_gray.rows / 8, 120, 60, 0, 0);

	/// Draw the circles detected
	for (size_t i = 0; i < circles.size(); i++)
	{
		Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
		int radius = cvRound(circles[i][2]);
		// circle center
		circle(src, center, 3, Scalar(0, 255, 0), -1, 8, 0);
		// circle outline
		circle(src, center, radius, Scalar(0, 0, 255), 3, 8, 0);

		xPos = cvRound(circles[i][0]);
		yPos = cvRound(circles[i][1]);

		cout << "(" << xPos << "," << yPos << ")" << endl;

	}

	/// Show your results
	namedWindow("Hough Circle Transform Demo", CV_WINDOW_AUTOSIZE);
	imshow("Hough Circle Transform Demo", src);
	imshow("MR.grey", src_gray);


}
