// Testkod för att kunna testa Coords separat från resten av applikationen

#include "stdafx.h"
#include "colorTracking.h"
#include <iostream>

using std::cin;
using std::cout;
using std::endl;

int main(int argc, char** argv)
{
	
	cv::VideoCapture cap(0); //capture the video from webcam

	if (!cap.isOpened())  // if not success, exit program
	{
		std::cout << "Cannot access videostream" << std::endl;
		return 0;
	}

	while (true) {

		cv::Mat imgOriginal;

		bool bSuccess = cap.read(imgOriginal); // read a new frame from video

		if (!bSuccess) //if not success, break loop
		{
			cout << "Cannot read a frame from video stream" << endl;
			break;
		}

		Coords::HSVfilter filter(0, 255, 0, 255, 0, 255);

		Coords coordsModule(filter);
		coordsModule.CalculateCoords(imgOriginal);
		if (coordsModule.ValidCoords()) {
			std::pair<int, int> coords = coordsModule.GetCoords();
			cout << "(" << coords.first << "," << coords.second << ")" << endl;
		}
		else
		{
			cout << "(" << -1 << "," << -1 << ")" << endl;
		}
		cv::imshow("Bild", coordsModule.GetFilteredImage());
	}

	return 0;
}
