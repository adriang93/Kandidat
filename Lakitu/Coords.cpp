/*

Modulen beräknar koordinaten till ett objekt i bilden. Koordinaterna sparas internt och kan hämtas 
av kallande klass. Använder OpenCV för att beräkna koordinatvärdena.

All kod utom det som markerats skriven av André Wallström.

*/

#include "stdafx.h"
#include "Coords.h"

using cv::Mat;

// Konstruktor som använder medskickade filtervärden för att filtrera bilden.
Coords::Coords() {}

// Returnerar true om beräkningar slutförts
bool Coords::Ready() {
	return ready;
}

// Returnerar den filterade bilden. 
// Returnerar en kopia, inte en referens, för att det skall vara säkert för 
// mottagaren att inte bilden modifieras.
cv::Mat Coords::GetFilteredImage() {

	// Lås semaforen medan bilden returneras så att inte bilden modifieras
	std::lock_guard<std::mutex> guard(filteredLock);
	if (ready) {
		return filteredImage;
	}
	else {
		NULL;
	}
}

// Returnera koordinater. 
// TODO: Retuerna huruvida koordinaterna är valid samtidigt, så att man alltid vet om returnerade
// koordinater är sanna. Annars finns risk att metoden ValidCoords körs efter att koordinaterna 
// ändrats.
Coords::Coord Coords::GetCoords() {
	return coord;
}

// Beräkna koordinater genom att filtrera bilden
void Coords::CalculateCoords(const cv::Mat& imgOriginal, 
		CoordsFilter filter, bool interlaced, bool returnFiltered) {

	// Gör koden mycket renare då nästan varje rad använder cv-metoder
	using namespace cv;

	// Eftersom vi håller på att beräkna är vi inte redo att börja beräkna.
	ready = false;
	
	// Om bilden är sammanflätad använder vi bara varannan rad dubblerad för att
	// slippa randig bild till detekteringen
	Mat interlacedImg(0, imgOriginal.rows, CV_8UC3);
	if (interlaced) {
		for (int i = 0; i < imgOriginal.rows; i += 2) {
			interlacedImg.push_back(imgOriginal.row(i));
			interlacedImg.push_back(imgOriginal.row(i));
		}
	}

	// Nedanstående kod delvis anpassad från OpenCV:s egna exempel från opencv.org.
	// Engelska kommentarer är från exempelkod från opencv.org.
	Mat imgHSV;
	if (interlaced) {
		cvtColor(interlacedImg, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
	}
	else {
		cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
	}
	Mat imgHSV2(imgHSV.clone());

	// Filtrera ut alla bildpunkter som omfattas av filtrets parametrar
	inRange(imgHSV, Scalar(filter.lowH, filter.lowS, filter.lowV),
		Scalar(filter.highH, filter.highS, filter.highV), imgHSV); //Threshold the image
																   
	//Hitta nästan-vita bildpunkter för att kunna detektera överexponerade objekt	
	inRange(imgHSV2, Scalar(0, 0, 245),	Scalar(255, 255, 255), imgHSV2); 

	imgHSV = imgHSV + imgHSV2; //addera för att möjliggöra detektion även vid överexponering

	// Utför bara de morfologiska operationerna om värdena på dem är > 0
	if (filter.open) {
		// Morfologisk öppning. Se opencv-dokumentation på 
		// http://docs.opencv.org/doc/tutorials/imgproc/erosion_dilatation/erosion_dilatation.html
		erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(filter.open, filter.open)));
		dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(filter.open, filter.open)));
	}
	if (filter.close) {
		// Morfologisk stängning, se föregående länk. 
		dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(filter.close, filter.close)));
		erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(filter.close, filter.close)));
	}

	// Anpassat från: 
	// http://stackoverflow.com/questions/8076889/tutorial-on-opencv-simpleblobdetector
	
	// Parametrar för SimpleBlobDetector skapas
	cv::SimpleBlobDetector::Params params;
	params.minDistBetweenBlobs = 100;
	
	// Filtrera inte baserat på avsmalnad eller konvexitet
	params.filterByInertia = false;
	params.filterByConvexity = false;
	
	// Filtrera baserat på färg och enbart vita bildpunkter
	params.filterByColor = true;
	params.blobColor = 255;

	// Filtrera baserat på cirkuläritet
	params.filterByCircularity = true;
	params.minCircularity = ((float)filter.minCircularity) / (100.0);

	// Filtrera även baserat på area
	params.filterByArea = true;
	params.minArea = max(filter.minArea, 1);
	params.maxArea = max(filter.maxArea, 1);

	// Skapa detektorobjektet med parametrarna ovan.
	SimpleBlobDetector blobDetector(params);
	
	// Skapa en vektor att spara alla detekterade objekt i och utför detekteringen i 
	// bildobjektet
	std::vector<KeyPoint> keypoints;
	blobDetector.detect(imgHSV, keypoints);

	// Gå igenom samtliga detekterade objekt och beräkna skillnaden i avstånd
	// och storlek mellan respektive objekt och förra detektionen.

	float sizediff; // Skillnaden i storlek
	float dist, disty, distx; // Totala avståndet, samt avståndet i x- och y-led
	float mindist = std::numeric_limits<float>::max(); // Nuvarande minsta avstånd är max.

	// Tillfällig ny position. Efter loopen kommer detta vara den bästa matchningen
	float newposx, newposy, newsize;
	
	// Skapa en semafor för koordinater, men lås den inte  ännu (std::defer_lock).
	std::unique_lock<std::mutex> coordsGuard(coordsLock, std::defer_lock);
	
	int num = keypoints.size(); // Antalet detekterade objekt
	if (num > 0)
	{
		// Kontrollera alla detekterade objekt
		for (int i = 0; i < num; i++) {
			disty = coord.x - keypoints[i].pt.x; // Avståndet i x-led
			distx = coord.y - keypoints[i].pt.y; // avståndet i y-led
			sizediff = coord.size - keypoints[i].size; // skillnaden i storlek, motsvarar y-led
			// Tredimensionella avståndet där storleken är en av dimensionerna (representerande
			// avståndet till objektet i verkligheten)
			dist = sqrt((disty*disty) + (distx*distx) + (sizediff*sizediff));

			// Om det nyss beräknade avståndet är mindre än det hittills minsta
			// sparas det
			if (dist < mindist) {
				mindist = dist;
				newposx = keypoints[i].pt.x; 
				newposy = keypoints[i].pt.y; 
				newsize = keypoints[i].size;
			}
		}
		// Lås koordinaterna med tidigare skapad semafor. 
		coordsGuard.lock();
		
		// Eftersom vi lyckades beräkna koordinaterna markeras koordinaterna som korrekta.
		coord.valid = true;
		coord.x = newposx;
		coord.y = newposy;
		coord.size = newsize;
	}
	else {
		// Lås koordinaterna med tidigare skapad semafor. Låt dock tidigare beräknade
		// koordinater vara kvar för att underlätta avståndsberäkningen ovan.
		coordsGuard.lock();
		coord.valid = false;
	}
	// Lås upp semaforerna
	coordsGuard.unlock();

	//Lås bilden när vi sparar kopian för att förhindra att man läser en halvskriven bild
	std::lock_guard<std::mutex> filterGuard(filteredLock);

	// Om kallande klassen vill det kan vi spara den filtrerade bilden med alla objekt inritade
	if (returnFiltered) {
		Mat imgKeypoints;
		// Koden för ritningen från exemplet nämnt ovan:
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