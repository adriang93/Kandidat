// test.cpp : Defines the entry point for the console application.
//

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

using namespace cv;
using namespace std;

Mat frame; Mat frame_gray;
int thresh = 20;
int max_thresh = 255;
RNG rng(12345);


/// Function header
void thresh_callback(int, void*);

/** @function main */
int main(int argc, char** argv)
{
	/// Load source image and convert it to gray
	VideoCapture cap("irvid.mp4");

	if (!cap.isOpened()){
		cout << "no file read" << endl;
		return -1;
	}

	/// Create Window
	char* source_window = "Source";
	namedWindow(source_window, CV_WINDOW_AUTOSIZE);

	while (true){

		bool bSuccess = cap.read(frame);

		if (!bSuccess){
			cout << "no more frames" << endl;
			break;
		}

		/// Convert image to gray and blur it
		cvtColor(frame, frame_gray, CV_BGR2GRAY);
		blur(frame_gray, frame_gray, Size(3, 3));

		imshow(source_window, frame_gray);

		createTrackbar(" Threshold:", "Source", &thresh, max_thresh, thresh_callback);
		thresh_callback(0, 0);

		waitKey(30);
	}

	
	return(0);
}

/** @function thresh_callback */
void thresh_callback(int, void*)
{
	Mat threshold_output;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	/// Detect edges using Threshold
	threshold(frame_gray, threshold_output, thresh, 255, THRESH_BINARY);
	/// Find contours
	findContours(threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	/// Approximate contours to polygons + get bounding rects and circles
	vector<vector<Point> > contours_poly(contours.size());
	vector<Rect> boundRect(contours.size());
	vector<Point2f>center(contours.size());
	vector<float>radius(contours.size());

	for (int i = 0; i < contours.size(); i++)
	{
		approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
		boundRect[i] = boundingRect(Mat(contours_poly[i]));
		minEnclosingCircle((Mat)contours_poly[i], center[i], radius[i]);
	}


	/// Draw polygonal contour + bonding rects + circles
	Mat drawing = Mat::zeros(threshold_output.size(), CV_8UC3);
	for (int i = 0; i< contours.size(); i++)
	{
		Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
		drawContours(drawing, contours_poly, i, color, 1, 8, vector<Vec4i>(), 0, Point());
		//rectangle(drawing, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0);
		circle(drawing, center[i], (int)radius[i], color, 2, 8, 0);

		Moments moment = moments((cv::Mat)contours[i]);
		double area = moment.m00;//m00 gives the area
		double x = moment.m10 / area;//gives the x coordinate
		double y = moment.m01 / area;  //gives y coordiante

		cout << "cirkel" << i << ":" << "(" << x << "," << y << ")" << endl;
	}

	/// Show in a window
	namedWindow("Contours", CV_WINDOW_AUTOSIZE);
	imshow("Contours", drawing);
}