/*

Kod som ber�knar positionen av ett objekt, genom cirkeligenk�nning, f�rgfiltrering eller b�da,
och spara koordinaterna. Ber�knar �ven hur bra de tv� metodernas resultat �verensst�mmer.

TODO: Licens f�r openCV-exempelkoden.

TODO, viktigare: Returnera b�de koordinater och huruvida de �r giltiga f�r att undvika att
koordinaterna hinner �ndras mellan anrop till ValidCoords och GetCoords.

*/

#include "stdafx.h"
#include "Coords.h"

using cv::Mat;

// Konstruktor som anv�nder medskickade filterv�rden f�r att filtrera bilden.
Coords::Coords(HSVfilter& filter) : filter(filter) {}

Coords::Coords() {}

// Det g�r att modifiera filterv�rdena under exekvering.
// TODO: G�r threadsafe med en semafor, som �ven l�ses d�r v�rdena anv�nds i koden
void Coords::SetHSV(HSVfilter& newFilter) {
	filter = newFilter;
}

bool Coords::Ready() {
	return ready;
}

// Returnerar en kombination av COORDS_CIRCLE och COORDS_FILTER beroende p� vilka
// metoder som lyckats ber�kna koordinater.
bool Coords::ValidCoords() {
	return validCoords;
}

// Returnerar den filterade bilden. 
// Returnerar en kopia, inte en referens, f�r att det skall vara s�kert f�r 
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
// TODO: Retuerna huruvida koordinaterna �r valid samtidigt, s� att man alltid vet om returnerade
// koordinater �r sanna. Annars finns risk att metoden ValidCoords k�rs efter att koordinaterna 
// �ndrats.
std::pair<int, int> Coords::GetCoords() {

	// L�s koordinaterna under ber�kning
	std::lock_guard<std::mutex> guard(coordsLock);

	if (validCoords) {
		return posFilter;
	}

	// Om ingen metod lyckats, returnera 0.
	else {
		return std::pair<int, int>(0, 0);
	}
}

// Ber�kna koordinater genom att filtrera bilden
void Coords::CalculateCoords(const cv::Mat& imgOriginal,
	int minArea, int maxArea, int minCircularity, 
	int open, int close) {

	// G�r koden mycket renare d� n�stan varje rad anv�nder cv-metoder
	using namespace cv;

	// Eftersom vi har b�rjat ber�kna fr�n en ny frame �r inte l�ngre koordinaterna 
	// giltiga.
	validCoords = false;

	// Eftersom vi h�ller p� att ber�kna �r vi inte redo att b�rja ber�kna.
	ready = false;

	// Skapa en semafor f�r koordinater, men l�s den inte (std::defer_lock).
	std::unique_lock<std::mutex> coordsGuard(coordsLock, std::defer_lock);
	
	// Nedanst�ende kod delvis anpassad fr�n OpenCV:s egna exempel fr�n opencv.org.
	// Engelska kommentarer �r fr�n exempelkod fr�n opencv.org.
	
	Mat imgHSV;
	cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV

	Mat imgHSV2 = imgHSV.clone();

	inRange(imgHSV, Scalar(filter.lowH, filter.lowS, filter.lowV),
		Scalar(filter.highH, filter.highS, filter.highV), imgHSV); //Threshold the image
																   
		//Hitta n�stan-vita pixlar f�r att kunna detektera �verexponerade objekt	
	inRange(imgHSV2, Scalar(0, 0, 245),	Scalar(255, 255, 255), imgHSV2); 

	imgHSV += imgHSV2; //addera f�r att m�jligg�ra detektion �ven vid �verexponering

	if (open) {
		// Morfologisk �ppning. Se opencv-dokumentation p� 
		// http://docs.opencv.org/doc/tutorials/imgproc/erosion_dilatation/erosion_dilatation.html
		erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(open, open)));
		dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(open, open)));
	}
	if (close) {
		// Morfologisk st�ngning, se f�reg�ende l�nk. 
		dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(close, close)));
		erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(close, close)));
	}
	// Anpassat fr�n: 
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

	// L�s koordinaterna med tidigare skapad semafor. 
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
		// Eftersom vi lyckades ber�kna koordinaterna s� s�tter vi flaggan.
		validCoords = true;
		posFilter.first = newposx;
		posFilter.second = newposy;
	}
	coordsGuard.unlock();

	//L�s bilden n�r vi sparar kopian f�r att f�rhindra att man l�ser en halvskriven bild
	std::lock_guard<std::mutex> filterGuard(filteredLock);
	filteredImage = imgKeypoints;
	ready = true;
}

// Rita ett kors. Anpassad fr�n OpenCV-exempel och gjord statisk d� den
// inte kr�ver n�gon instantiering. Justerad f�r att inte anv�nda fula
// if/else-satser
void Coords::DrawCross(int x, int y, Mat& image) {

	using namespace cv;
	//use some of the openCV drawing functions to draw crosshairs
	//on your tracked image!

	//UPDATE:JUNE 18TH, 2013
	//added 'if' and 'else' statements to prevent
	//memory errors from writing off the screen (ie. (-25,-25) is not within the window!)
	//Modifierad 2015-05-09 med max och min f�r att undvika if/else vilka gjorde koden
	//mer otydlig

	// F�rg p� korset
	Scalar color = Scalar(0, 255, 0);
	
	//circle(image, Point(x, y), 10, scalar, 2);
	line(image, Point(x, y), Point(x, max(y - 10,0)), color, 2);
	line(image, Point(x, y), Point(x, min(y + 10, image.rows)), color, 2);
	line(image, Point(x, y), Point(max(x - 10, 0), y), color, 2);
	line(image, Point(x, y), Point(min(x + 10, image.cols), y), color, 2);
}