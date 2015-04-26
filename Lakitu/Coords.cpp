/*

Kod som beräknar positionen av ett objekt, genom cirkeligenkänning, färgfiltrering eller båda,
och spara koordinaterna. Beräknar även hur bra de två metodernas resultat överensstämmer.

TODO: Licens för openCV-exempelkoden.

TODO, viktigare: Returnera både koordinater och huruvida de är giltiga för att undvika att
koordinaterna hinner ändras mellan anrop till ValidCoords och GetCoords.

*/

#include "stdafx.h"
#include "Coords.h"

using cv::Mat;

// Konstruktor som använder medskickade filtervärden för att filtrera bilden.
Coords::Coords(HSVfilter& filter) : filter(filter) {}

Coords::Coords() {}

// Det går att modifiera filtervärdena under exekvering.
// TODO: Gör threadsafe med en semafor, som även låses där värdena används i koden
void Coords::SetHSV(HSVfilter& newFilter) {
	filter = newFilter;
}

bool Coords::Ready() {
	return ready;
}

// Returnerar en kombination av COORDS_CIRCLE och COORDS_FILTER beroende på vilka
// metoder som lyckats beräkna koordinater.
bool Coords::ValidCoords() {
	return validCoords;
}

// Returnerar den filterade bilden. 
// Returnerar en kopia, inte en referens, för att det skall vara säkert för 
// mottagaren att inte bilden modifieras.

cv::Mat Coords::GetFilteredImage() {
	std::lock_guard<std::mutex> guard(filteredLock);
	if (ready) {
		return filteredImage;
	}
	else {
		return cv::Mat(640, 480, CV_8UC3, cv::Scalar(0, 0, 0));
	}
}

// Returnera koordinater. 
// TODO: Retuerna huruvida koordinaterna är valid samtidigt, så att man alltid vet om returnerade
// koordinater är sanna. Annars finns risk att metoden ValidCoords körs efter att koordinaterna 
// ändrats.
std::pair<int, int> Coords::GetCoords() {

	// Lås koordinaterna under beräkning
	std::lock_guard<std::mutex> guard(coordsLock);
	
	if (validCoords) {
		return posFilter;
	}

	// Om ingen metod lyckats, returnera 0.
	else {
		return std::pair<int, int>(0, 0);
	}
}

// Beräkna koordinater genom att filtrera bilden
void Coords::CalculateCoords(const cv::Mat& imgOriginal) {

	// Gör koden mycket renare då nästan varje rad använder cv-metoder
	using namespace cv;

	// Eftersom vi har börjat beräkna från en ny frame är inte längre koordinaterna 
	// giltiga.
	validCoords = false;

	// Eftersom vi håller på att beräkna är vi inte redo att börja beräkna.
	ready = false;

	// Skapa en semafor för koordinater, men lås den inte (std::defer_lock).
	std::unique_lock<std::mutex> coordsGuard(coordsLock, std::defer_lock);
	// Nedanstående kod fårn OpenCV-exempel. Alla engelska kommentarer är tagna från 
	// exempelkoden.

	Mat imgHSV;
	cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV

	Mat imgHSV2 = imgHSV.clone();

	inRange(imgHSV, Scalar(filter.lowH, filter.lowS, filter.lowV),
		Scalar(filter.highH, filter.highS, filter.highV), imgHSV); //Threshold the image

	inRange(imgHSV2, Scalar(0, 0, 245),
		Scalar(255, 255, 255), imgHSV2); //Hitta nästan-vita pixlar

	imgHSV += imgHSV2; //addera för att möjliggöra detektion även vid överexponering

	//morphological opening (removes small objects from the foreground)
	erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

	//morphological closing (removes small holes from the foreground)
	dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
	erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

	//Från: http://stackoverflow.com/questions/8076889/tutorial-on-opencv-simpleblobdetector

	cv::SimpleBlobDetector::Params params;
	params.minDistBetweenBlobs = 100;
	params.filterByInertia = false;
	params.filterByConvexity = false;
	params.filterByColor = false;
	params.filterByCircularity = true;
	params.filterByArea = true;
	params.minCircularity = 0.70;
	params.minArea = 60;
	params.maxArea = 1200;

	SimpleBlobDetector blobDetector(params);
	std::vector<KeyPoint> keypoints;
	blobDetector.detect(imgHSV, keypoints);

	Mat imgKeypoints;
	drawKeypoints(imgHSV, keypoints, imgKeypoints, Scalar(200, 0, 150),
		DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

	// Lås koordinaterna med tidigare skapad semafor. 
	coordsGuard.lock();

	int mindist = std::numeric_limits<int>::max();
	int dist, disty, distx;
	int newposx, newposy;
	int size = keypoints.size();
	if (size > 0)
	{
		for (int i = 0; i < size; i++) {
			disty = posFilter.first - keypoints[i].pt.x;
			distx = posFilter.second - keypoints[i].pt.y;
			dist = sqrt((disty*disty) + (distx*distx));
			if (dist < mindist) {
				mindist = dist;
				newposx = keypoints[i].pt.x;// static_cast<int>(dM10 / dArea + 0.5);
				newposy = keypoints[i].pt.y; // static_cast<int>(dM01 / dArea + 0.5);
			}
		}
		// Eftersom vi lyckades beräkna koordinaterna så sätter vi flaggan.
		validCoords = true;
		posFilter.first = newposx;
		posFilter.second = newposy;
	}
	coordsGuard.unlock();

	//Lås bilden när vi sparar kopian för att förhindra att man läser en halvskriven bild
	std::lock_guard<std::mutex> filterGuard(filteredLock);
	filteredImage = imgKeypoints;
	ready = true;
}

// Rita ett kors. Anpassad från OpenCV-exempel och gjord statisk då den
// inte kräver någon instantiering.
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