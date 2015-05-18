/*

Modulen ber�knar koordinaten till ett objekt i bilden. Koordinaterna sparas internt och kan h�mtas 
av kallande klass. Anv�nder OpenCV f�r att ber�kna koordinatv�rdena.

All kod utom det som markerats skriven av Andr� Wallstr�m.

*/

#include "stdafx.h"
#include "Coords.h"

using cv::Mat;

// Konstruktor som anv�nder medskickade filterv�rden f�r att filtrera bilden.
Coords::Coords() {}

// Returnerar true om ber�kningar slutf�rts
bool Coords::Ready() {
	return ready;
}

// Returnerar den filterade bilden. 
// Returnerar en kopia, inte en referens, f�r att det skall vara s�kert f�r 
// mottagaren att inte bilden modifieras.
cv::Mat Coords::GetFilteredImage() {

	// L�s semaforen medan bilden returneras s� att inte bilden modifieras
	std::lock_guard<std::mutex> guard(filteredLock);
	if (ready) {
		return filteredImage;
	}
	else {
		NULL;
	}
}

// Returnera koordinater. 
// TODO: Retuerna huruvida koordinaterna �r valid samtidigt, s� att man alltid vet om returnerade
// koordinater �r sanna. Annars finns risk att metoden ValidCoords k�rs efter att koordinaterna 
// �ndrats.
Coords::Coord Coords::GetCoords() {
	return coord;
}

// Ber�kna koordinater genom att filtrera bilden
void Coords::CalculateCoords(const cv::Mat& imgOriginal, 
		CoordsFilter filter, bool interlaced, bool returnFiltered) {

	// G�r koden mycket renare d� n�stan varje rad anv�nder cv-metoder
	using namespace cv;

	// Eftersom vi h�ller p� att ber�kna �r vi inte redo att b�rja ber�kna.
	ready = false;
	
	// Om bilden �r sammanfl�tad anv�nder vi bara varannan rad dubblerad f�r att
	// slippa randig bild till detekteringen
	Mat interlacedImg(0, imgOriginal.rows, CV_8UC3);
	if (interlaced) {
		for (int i = 0; i < imgOriginal.rows; i += 2) {
			interlacedImg.push_back(imgOriginal.row(i));
			interlacedImg.push_back(imgOriginal.row(i));
		}
	}

	// Nedanst�ende kod delvis anpassad fr�n OpenCV:s egna exempel fr�n opencv.org.
	// Engelska kommentarer �r fr�n exempelkod fr�n opencv.org.
	Mat imgHSV;
	if (interlaced) {
		cvtColor(interlacedImg, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
	}
	else {
		cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
	}
	Mat imgHSV2 = imgHSV.clone();

	// Filtrera ut alla bildpunkter som omfattas av filtrets parametrar
	inRange(imgHSV, Scalar(filter.lowH, filter.lowS, filter.lowV),
		Scalar(filter.highH, filter.highS, filter.highV), imgHSV); //Threshold the image
																   
	//Hitta n�stan-vita bildpunkter f�r att kunna detektera �verexponerade objekt	
	inRange(imgHSV2, Scalar(0, 0, 245),	Scalar(255, 255, 255), imgHSV2); 

	imgHSV += imgHSV2; //addera f�r att m�jligg�ra detektion �ven vid �verexponering

	// Utf�r bara de morfologiska operationerna om v�rdena p� dem �r > 0
	if (filter.open) {
		// Morfologisk �ppning. Se opencv-dokumentation p� 
		// http://docs.opencv.org/doc/tutorials/imgproc/erosion_dilatation/erosion_dilatation.html
		erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(filter.open, filter.open)));
		dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(filter.open, filter.open)));
	}
	if (filter.close) {
		// Morfologisk st�ngning, se f�reg�ende l�nk. 
		dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(filter.close, filter.close)));
		erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(filter.close, filter.close)));
	}

	// Anpassat fr�n: 
	// http://stackoverflow.com/questions/8076889/tutorial-on-opencv-simpleblobdetector
	
	// Parametrar f�r SimpleBlobDetector skapas
	cv::SimpleBlobDetector::Params params;
	params.minDistBetweenBlobs = 100;
	
	// Filtrera inte baserat p� avsmalnad eller konvexitet
	params.filterByInertia = false;
	params.filterByConvexity = false;
	
	// Filtrera baserat p� f�rg och enbart vita bildpunkter
	params.filterByColor = true;
	params.blobColor = 255;

	// Filtrera baserat p� cirkul�ritet
	params.filterByCircularity = true;
	params.minCircularity = ((float)filter.minCircularity) / (100.0);

	// Filtrera �ven baserat p� area
	params.filterByArea = true;
	params.minArea = max(filter.minArea, 1);
	params.maxArea = max(filter.maxArea, 1);

	// Skapa detektorobjektet med parametrarna ovan.
	SimpleBlobDetector blobDetector(params);
	
	// Skapa en vektor att spara alla detekterade objekt i och utf�r detekteringen i 
	// bildobjektet
	std::vector<KeyPoint> keypoints;
	blobDetector.detect(imgHSV, keypoints);

	// G� igenom samtliga detekterade objekt och ber�kna skillnaden i avst�nd
	// och storlek mellan respektive objekt och f�rra detektionen.

	float sizediff; // Skillnaden i storlek
	float dist, disty, distx; // Totala avst�ndet, samt avst�ndet i x- och y-led
	float mindist = std::numeric_limits<float>::max(); // Nuvarande minsta avst�nd �r max.

	// Tillf�llig ny position. Efter loopen kommer detta vara den b�sta matchningen
	float newposx, newposy, newsize;
	
	// Skapa en semafor f�r koordinater, men l�s den inte  �nnu (std::defer_lock).
	std::unique_lock<std::mutex> coordsGuard(coordsLock, std::defer_lock);
	
	int num = keypoints.size(); // Antalet detekterade objekt
	if (num > 0)
	{
		// Kontrollera alla detekterade objekt
		for (int i = 0; i < num; i++) {
			disty = coord.x - keypoints[i].pt.x; // Avst�ndet i x-led
			distx = coord.y - keypoints[i].pt.y; // avst�ndet i y-led
			sizediff = coord.size - keypoints[i].size; // skillnaden i storlek, motsvarar y-led
			// Tredimensionella avst�ndet d�r storleken �r en av dimensionerna (representerande
			// avst�ndet till objektet i verkligheten)
			dist = sqrt((disty*disty) + (distx*distx) + (sizediff*sizediff));

			// Om det nyss ber�knade avst�ndet �r mindre �n det hittills minsta
			// sparas det
			if (dist < mindist) {
				mindist = dist;
				newposx = keypoints[i].pt.x; 
				newposy = keypoints[i].pt.y; 
				newsize = keypoints[i].size;
			}
		}
		// L�s koordinaterna med tidigare skapad semafor. 
		coordsGuard.lock();
		
		// Eftersom vi lyckades ber�kna koordinaterna markeras koordinaterna som korrekta.
		coord.valid = true;
		coord.x = newposx;
		coord.y = newposy;
		coord.size = newsize;
	}
	else {
		// L�s koordinaterna med tidigare skapad semafor. L�t dock tidigare ber�knade
		// koordinater vara kvar f�r att underl�tta avst�ndsber�kningen ovan.
		coordsGuard.lock();
		coord.valid = false;
	}
	// L�s upp semaforerna
	coordsGuard.unlock();

	//L�s bilden n�r vi sparar kopian f�r att f�rhindra att man l�ser en halvskriven bild
	std::lock_guard<std::mutex> filterGuard(filteredLock);

	// Om kallande klassen vill det kan vi spara den filtrerade bilden med alla objekt inritade
	if (returnFiltered) {
		Mat imgKeypoints;
		// Koden f�r ritningen fr�n exemplet n�mnt ovan:
		// http://stackoverflow.com/questions/8076889/tutorial-on-opencv-simpleblobdetector
		drawKeypoints(imgHSV, keypoints, imgKeypoints, Scalar(255, 0, 255),
			DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
		filteredImage = imgKeypoints;
	}
	else {
		filteredImage = imgHSV;
	}
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