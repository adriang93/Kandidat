#include "stdafx.h"
#include "opencv2/core/core.hpp"
#include "opencv2/flann/miniflann.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/photo/photo.hpp"
#include "opencv2/video/video.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/ml/ml.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core_c.h"
#include "opencv2/highgui/highgui_c.h"
#include "opencv2/imgproc/imgproc_c.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "Coords.h"

using namespace cv;
using namespace std;

int posX = 0;
int posY = 0;

int iLowH = 80;
int iHighH = 100;

int iLowS = 100;
int iHighS = 255;

int iLowV = 60;
int iHighV = 255;

int iLastY = -1;
int iLastX = -1;

const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;

Mat img;

string intToString(int number){


	std::stringstream ss;
	ss << number;
	return ss.str();
}

vector<int> Coords::getCoords()
{
	vector<int> coords;

	coords.push_back(posX);
	coords.push_back(posY);

	return coords;
}

void Coords::setCoords(int x, int y){
	int *xcoord = &posX;
	int *ycoord = &posY;

	*xcoord = x;
	*ycoord = y;
}

void Coords::calcCoords(Mat& imgOriginal){

	//Mat imgLines;
	Mat imgHSV;
	
	cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV

	inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgHSV); //Threshold the image

	//morphological opening (removes small objects from the foreground)
	erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

	//morphological closing (removes small holes from the foreground)
	dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

	//create windows
	namedWindow("Control", CV_WINDOW_AUTOSIZE);

	//Create trackbars in "Control" window
	createTrackbar("LowH", "Control", &iLowH, 179); //Hue (0 - 179)
	createTrackbar("HighH", "Control", &iHighH, 179);

	createTrackbar("LowS", "Control", &iLowS, 255); //Saturation (0 - 255)
	createTrackbar("HighS", "Control", &iHighS, 255);

	createTrackbar("LowV", "Control", &iLowV, 255);//Value (0 - 255)
	createTrackbar("HighV", "Control", &iHighV, 255);

	imshow("Control", imgHSV); //show the thresholded image

	//Calculate the moments of the thresholded image
	Moments oMoments = moments(imgHSV);

	double dM01 = oMoments.m01;
	double dM10 = oMoments.m10;
	double dArea = oMoments.m00;

	// if the area <= 10000, I consider that the there are no object in the image and it's because of the noise, the area is not zero 
	if (dArea > 10000)
	{
		//calculate the position of the ball
		int coordX = dM10 / dArea;
		int coordY = dM01 / dArea;

		setCoords(coordX, coordY);

		drawShit(coordX, coordY, imgOriginal);

	}

}

void Coords::init(VideoCapture& cap)
{
	vector<int> coord;

	while (true){

		Mat imgOriginal;

		bool bSuccess = cap.read(imgOriginal); // read a new frame from video

		if (!bSuccess) //if not success, break loop
		{
			cout << "Cannot read a frame from video stream" << endl;
			break;
		}

		//Mat imgLines = Mat::zeros(imgOriginal.size(), CV_8UC3);

		calcCoords(imgOriginal);

		//coord = getCoords();
		//cout << "(" << coord.at(0) << "," << coord.at(1) << ")" << endl;

		if (waitKey(30) == 27) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
		{
			cout << "esc key is pressed by user" << endl;
			break;
		}
	}

	return;
}

void Coords::drawShit(int x, int y, Mat& imgOriginal){

		//use some of the openCV drawing functions to draw crosshairs
		//on your tracked image!

		//UPDATE:JUNE 18TH, 2013
		//added 'if' and 'else' statements to prevent
		//memory errors from writing off the screen (ie. (-25,-25) is not within the window!)

		circle(imgOriginal, Point(x, y), 10, Scalar(0, 255, 0), 2);
		if (y - 10>0)
			line(imgOriginal, Point(x, y), Point(x, y - 10), Scalar(0, 255, 0), 2);
		else line(imgOriginal, Point(x, y), Point(x, 0), Scalar(0, 255, 0), 2);
		if (y + 10<FRAME_HEIGHT)
			line(imgOriginal, Point(x, y), Point(x, y + 10), Scalar(0, 255, 0), 2);
		else line(imgOriginal, Point(x, y), Point(x, FRAME_HEIGHT), Scalar(0, 255, 0), 2);
		if (x - 10>0)
			line(imgOriginal, Point(x, y), Point(x - 10, y), Scalar(0, 255, 0), 2);
		else line(imgOriginal, Point(x, y), Point(0, y), Scalar(0, 255, 0), 2);
		if (x + 10<FRAME_WIDTH)
			line(imgOriginal, Point(x, y), Point(x + 10, y), Scalar(0, 255, 0), 2);
		else line(imgOriginal, Point(x, y), Point(FRAME_WIDTH, y), Scalar(0, 255, 0), 2);

		putText(imgOriginal, intToString(x) + "," + intToString(y), Point(x, y + 30), 1, 1, Scalar(0, 255, 0), 2);

	
		imshow("Original", imgOriginal);

	


}



