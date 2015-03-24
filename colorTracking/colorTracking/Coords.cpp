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

int iLowH = 100;
int iHighH = 160;

int iLowS = 100;
int iHighS = 255;

int iLowV = 60;
int iHighV = 255;

int iLastY = -1;
int iLastX = -1;

Mat img;

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

	Mat imgLines;
	Mat imgHSV;
	
	cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV

	inRange(imgHSV, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), imgHSV); //Threshold the image

	//morphological opening (removes small objects from the foreground)
	erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

	//morphological closing (removes small holes from the foreground)
	dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

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

		readFrame(imgOriginal);

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

		Mat imgLines = Mat::zeros(imgOriginal.size(), CV_8UC3);

		calcCoords(imgOriginal);

		coord = getCoords();
		cout << "(" << coord.at(0) << "," << coord.at(1) << ")" << endl;

		if (waitKey(30) == 27) //wait for 'esc' key press for 30ms. If 'esc' key is pressed, break loop
		{
			cout << "esc key is pressed by user" << endl;
			break;
		}
	}

	return;
}

void Coords::readFrame(Mat& imgOriginal){

	Mat imgLines = Mat::zeros(imgOriginal.size(), CV_8UC3);;
	
	if (iLastX >= 0 && iLastY >= 0 && posX >= 0 && posY >= 0)
	{
		//Draw a red line from the previous point to the current point
		line(imgLines, Point(posX, posY), Point(iLastX, iLastY), CV_RGB(239, 224, 21), 2, 8, 0);
	}

	iLastX = posX;
	iLastY = posY;

	img = imgOriginal + imgLines;
	imshow("Original", img);


}



