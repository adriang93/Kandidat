#include "stdafx.h"
#include "Coords.h"

using cv::Mat;

Coords::Coords(HSVfilter& filter) : filter(filter), ready(false), validCoords(0) {}

Coords::Coords() : ready(false), validCoords(0) {}

void Coords::SetHSV(HSVfilter& newFilter) {
	filter = newFilter;
}

// Möjligt att skicka skräpvärden
void Coords::SetMode(int newmode) {
	mode = newmode;
}

bool Coords::Ready() {
	return ready;
}

int Coords::ValidCoords() {
	return validCoords;
}

float Coords::GetCorrellation() {
	std::lock_guard<std::mutex> guard(coordsLock);
	if (validCoords & COORDS_CIRCLE & COORDS_FILTER) {
		int distX = posFilter.first - posCircle.first;
		int distY = posFilter.second - posCircle.second;
		return sqrt(distX*distX + distY*distY);
	}
	else {
		return 0;
	}
}

// returnera en kopia för att undvika att bilden ändras i denna klass medan den används någon annanstans
cv::Mat Coords::GetCircledImage() {
	std::lock_guard<std::mutex> guard(circledLock);
	if (ready) {
		return circledImage;
	}
	else {
		return cv::Mat(640, 480, CV_8UC3, cv::Scalar(0, 0, 0));
	}
}
cv::Mat Coords::GetFilteredImage() {
	std::lock_guard<std::mutex> guard(filteredLock);
	if (ready) {
		return filteredImage;
	}
	else {
		return cv::Mat(640, 480, CV_8UC3, cv::Scalar(0, 0, 0));
	}
}

std::pair<int, int> Coords::GetCoords() {
	std::lock_guard<std::mutex> guard(coordsLock);
	if (validCoords & COORDS_CIRCLE & COORDS_FILTER) {
		std::pair<int, int> pos(0, 0);
		pos.first = (posFilter.first + posCircle.first) / 2;
		pos.second = (posFilter.second + posCircle.second) / 2;
		return pos;
	}
	else if (validCoords & COORDS_CIRCLE) {
		return posCircle;
	}
	else if (validCoords & COORDS_FILTER) {
		return posFilter;
	}
	else {
		return std::pair<int, int>(0, 0);
	}
}

void Coords::CalculateCoords(const cv::Mat& imgOriginal) {
	using namespace cv;

	ready = false;

	std::unique_lock<std::mutex> coordsGuard(coordsLock, std::defer_lock);
	if (mode & COORDS_FILTER)
	{
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
		coordsGuard.lock();
		if (dArea > 10000)
		{
			//calculate the position of the object
			posFilter.first = static_cast<int>(dM10 / dArea + 0.5);
			posFilter.second = static_cast<int>(dM01 / dArea + 0.5);
			validCoords |= COORDS_FILTER;
		}
		else {
			validCoords |= ~COORDS_FILTER;
		}
		coordsGuard.unlock();

		//Lås bilden när vi sparar kopian för att förhindra att man läser en halvskriven bild
		std::unique_lock<std::mutex> filterGuard(filteredLock);
		filteredImage = imgHSV;
		filterGuard.unlock();
	}

	if (mode & COORDS_CIRCLE) {
		Mat src_gray;

		// Convert it to gray
		cvtColor(imgOriginal, src_gray, CV_BGR2GRAY);

		// Reduce the noise so we avoid false circle detection
		GaussianBlur(src_gray, src_gray, Size(9, 9), 2, 2);

		vector<Vec3f> circles;

		// Apply the Hough Transform to find the circles
		HoughCircles(src_gray, circles, CV_HOUGH_GRADIENT, 1, src_gray.rows / 8, 120, 60, 5, 0);

		cv::Mat imgCircles = imgOriginal;

		// Draw the circles detected
		int largest = 0;
		float largestRadius = 0;
		for (size_t i = 0; i < circles.size(); i++)
		{
			Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
			int radius = cvRound(circles[i][2]);
			if (radius > largestRadius) {
				largestRadius = radius;
				largest = i;
			}
			// circle center
			circle(imgCircles, center, 3, Scalar(0, 255, 0), -1, 8, 0);
			// circle outline
			circle(imgCircles, center, radius, Scalar(0, 0, 255), 3, 8, 0);
		}
		coordsGuard.lock();
		if (largestRadius == 0) {
			validCoords |= ~COORDS_CIRCLE;
		}
		else {
			posCircle.first = circles[largest][0];
			posCircle.second = circles[largest][1];
			validCoords |= COORDS_CIRCLE;
		}
		coordsGuard.unlock();
		std::lock_guard<std::mutex> guard(circledLock);
		circledImage = imgCircles;
	}
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