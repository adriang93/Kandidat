#include "stdafx.h"
#include "Coords.h"

using cv::Mat;

Coords::Coords(HSVfilter filter) : filter(filter), ready(false), validCoords(false) {}
Coords::Coords() {}

void Coords::SetHSV(HSVfilter newFilter)
{
	filter = newFilter;
}

bool Coords::Ready()
{
	return ready;
}

bool Coords::ValidCoords()
{
	return validCoords;
}

// returnera en kopia för att undvika att bilden ändras i denna klass medan den används någon annanstans
cv::Mat Coords::GetFilteredImage()
{
	std::lock_guard<std::mutex> guard(imageLock);
	if (ready) {
		return filteredImage;
	}
	else {
		return cv::Mat(640, 480, CV_8UC3, cv::Scalar(0, 0, 0));
	}
}

std::pair<int, int> Coords::GetCoords()
{
	std::lock_guard<std::mutex> guard(coordsLock);
	return std::pair<int, int>(posX, posY);
}

void Coords::SetCoords(int x, int y) {
	std::lock_guard<std::mutex> guard(coordsLock);
	posX = x;
	posY = y;
}

void Coords::CalculateCoords(const cv::Mat& imgOriginal) {
	using namespace cv;

	ready = false;

	cv::Mat imgHSV;
	cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
	inRange(imgHSV, Scalar(filter.lowH, filter.lowS, filter.lowV),
		Scalar(filter.highH, filter.highS, filter.highV), imgHSV); //Threshold the image

	//morphological opening (removes small objects from the foreground)
	erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

	//morphological closing (removes small holes from the foreground)
	dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

	//Denna kod är numera inte användbar utan bör istället skapas i den anropande klassen.

	//create windows
	//namedWindow("Control", CV_WINDOW_AUTOSIZE);

	//Create trackbars in "Control" window
	//createTrackbar("LowH", "Control", &iLowH, 179); //Hue (0 - 179)
	//createTrackbar("HighH", "Control", &iHighH, 179);

	//createTrackbar("LowS", "Control", &iLowS, 255); //Saturation (0 - 255)
	//createTrackbar("HighS", "Control", &iHighS, 255);

	//createTrackbar("LowV", "Control", &iLowV, 255);//Value (0 - 255)
	//createTrackbar("HighV", "Control", &iHighV, 255);

	//imshow("Control", imgHSV); //show the thresholded image

	//Calculate the moments of the thresholded image

	Moments oMoments = moments(imgHSV);
	double dM01 = oMoments.m01;
	double dM10 = oMoments.m10;
	double dArea = oMoments.m00;

	// if the area <= 10000, I consider that the there are no object in the image and it's because of the noise, the area is not zero 
	if (dArea > 10000)
	{
		//calculate the position of the ball
		int coordX = static_cast<int>(dM10 / dArea + 0.5);
		int coordY = static_cast<int>(dM01 / dArea + 0.5);

		SetCoords(coordX, coordY);
		validCoords = true;
	}
	else {
		validCoords = false;
	}

	//Lås bilden när vi sparar kopian för att förhindra att man läser en halvskriven bild
	std::lock_guard<std::mutex> guard(imageLock);
	filteredImage = imgHSV;
	ready = true;
}

void Coords::DrawCross(int x, int y, Mat& image) {

	using namespace cv;
	//use some of the openCV drawing functions to draw crosshairs
	//on your tracked image!

	//UPDATE:JUNE 18TH, 2013
	//added 'if' and 'else' statements to prevent
	//memory errors from writing off the screen (ie. (-25,-25) is not within the window!)
	Scalar scalar = Scalar(0, 255, 0);
	circle(image, Point(x, y), 10, scalar, 2);
	if (y - 10 > 0)
		line(image, Point(x, y), Point(x, y - 10), scalar, 2);
	else line(image, Point(x, y), Point(x, 0), scalar, 2);
	if (y + 10 < image.rows)
		line(image, Point(x, y), Point(x, y + 10), scalar, 2);
	else line(image, Point(x, y), Point(x, image.rows), scalar, 2);
	if (x - 10 > 0)
		line(image, Point(x, y), Point(x - 10, y), scalar, 2);
	else line(image, Point(x, y), Point(0, y), scalar, 2);
	if (x + 10 < image.cols)
		line(image, Point(x, y), Point(x + 10, y), scalar, 2);
	else line(image, Point(x, y), Point(image.cols, y), scalar, 2);
}