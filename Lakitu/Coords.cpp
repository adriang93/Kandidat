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
void Coords::CalculateCoords(const cv::Mat& imgOriginal,
	int minArea, int maxArea, int minCircularity, 
	int open, int close) {

	// Gör koden mycket renare då nästan varje rad använder cv-metoder
	using namespace cv;

	// Eftersom vi har börjat beräkna från en ny frame är inte längre koordinaterna 
	// giltiga.
	validCoords = false;

	// Eftersom vi håller på att beräkna är vi inte redo att börja beräkna.
	ready = false;

	// Skapa en semafor för koordinater, men lås den inte (std::defer_lock).
	std::unique_lock<std::mutex> coordsGuard(coordsLock, std::defer_lock);
	
	// Nedanstående kod delvis anpassad från OpenCV:s egna exempel från opencv.org.
	// Engelska kommentarer är från exempelkod från opencv.org.
	
	Mat imgHSV;
	cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV

	Mat imgHSV2 = imgHSV.clone();

	inRange(imgHSV, Scalar(filter.lowH, filter.lowS, filter.lowV),
		Scalar(filter.highH, filter.highS, filter.highV), imgHSV); //Threshold the image
																   
		//Hitta nästan-vita pixlar för att kunna detektera överexponerade objekt	
	inRange(imgHSV2, Scalar(0, 0, 245),	Scalar(255, 255, 255), imgHSV2); 

	imgHSV += imgHSV2; //addera för att möjliggöra detektion även vid överexponering

	if (open) {
		// Morfologisk öppning. Se opencv-dokumentation på 
		// http://docs.opencv.org/doc/tutorials/imgproc/erosion_dilatation/erosion_dilatation.html
		erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(open, open)));
		dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(open, open)));
	}
	if (close) {
		// Morfologisk stängning, se föregående länk. 
		dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(close, close)));
		erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(close, close)));
	}
	// Anpassat från: 
	// http://stackoverflow.com/questions/8076889/tutorial-on-opencv-simpleblobdetector

	cv::SimpleBlobDetector::Params params;
	params.minDistBetweenBlobs = 100;
	params.filterByInertia = false;
	params.filterByConvexity = false;
	params.filterByColor = true;
	params.filterByCircularity = true;
	params.filterByArea = true;
	params.blobColor = 255;
	params.minCircularity = ((float)minCircularity) / (100.0);
	params.minArea = max(minArea, 1);
	params.maxArea = max(maxArea, 1);

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
// inte kräver någon instantiering. Justerad för att inte använda fula
// if/else-satser
void Coords::DrawCross(int x, int y, Mat& image) {

	using namespace cv;
	//use some of the openCV drawing functions to draw crosshairs
	//on your tracked image!

	//UPDATE:JUNE 18TH, 2013
	//added 'if' and 'else' statements to prevent
	//memory errors from writing off the screen (ie. (-25,-25) is not within the window!)
	//Modifierad 2015-05-09 med max och min för att undvika if/else vilka gjorde koden
	//mer otydlig

	// Färg på korset
	Scalar color = Scalar(0, 255, 0);
	
	//circle(image, Point(x, y), 10, scalar, 2);
	line(image, Point(x, y), Point(x, max(y - 10,0)), color, 2);
	line(image, Point(x, y), Point(x, min(y + 10, image.rows)), color, 2);
	line(image, Point(x, y), Point(max(x - 10, 0), y), color, 2);
	line(image, Point(x, y), Point(min(x + 10, image.cols), y), color, 2);
}