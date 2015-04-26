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

// S�tt vilka ber�kningsmetoder som skalla anv�ndas. Meningen �r att de olika
// bitflaggorna i modes skall anv�ndas, men kontrollerar inte detta. Alla int-v�rden
// accepteras men bara de tv� minsta bittarna kontrolleras i koden.
void Coords::SetMode(int newmode) {
	mode = newmode;
}

bool Coords::Ready() {
	return ready;
}

// Returnerar en kombination av COORDS_CIRCLE och COORDS_FILTER beroende p� vilka
// metoder som lyckats ber�kna koordinater.
int Coords::ValidCoords() {
	return validCoords;
}

// Ber�kna avst�ndet (inte egentligen korrelationen) mellan de tv� ber�knade koordinaterna, 
// eller returnera 0 om inga eller bara en ber�knad koordinat finns.
float Coords::GetCorrellation() {
	
	// F�rhindra att koordinaterna �ndras under ber�kning
	// lock_guard l�ses upp automatiskt n�r den hamnar utanf�r scope
	std::lock_guard<std::mutex> guard(coordsLock); 

	// Har b�da metoderna lyckats ber�kna koordinater?
	if (validCoords & COORDS_CIRCLE & COORDS_FILTER) {
		
		//Ber�kna avst�ndet
		int distX = posFilter.first - posCircle.first;
		int distY = posFilter.second - posCircle.second;
		return sqrt(distX*distX + distY*distY);
	}
	else {
		return 0;
	}
}

// Returnerar de olika typerna av filterade bilder. Med circlar (CircledImage) eller filtrerad
// (FilteredImage): Returnerar en kopia, inte en referens, f�r att det skall vara s�kert f�r 
// mottagaren att inte bilden modifieras.
cv::Mat Coords::GetCircledImage() {

	// L�s bilden f�r att f�rhindra att den modifieras under kopiering.
	std::lock_guard<std::mutex> guard(circledLock);

	// TODO: B�r alltid returnera en bild om en s�dan existerat n�gon g�ng. Nu s� returneras en 
	// blank bild om ber�kningskoden b�rjat k�ras (och ready = false) ist�llet f�r att returnera den
	// just nu senaste bilden.
	if (ready) {
		return circledImage;
	}
	else {
		return cv::Mat(640, 480, CV_8UC3, cv::Scalar(0, 0, 0));
	}
}

// Samma som ovan.
cv::Mat Coords::GetFilteredImage() {
	std::lock_guard<std::mutex> guard(filteredLock);
	if (ready) {
		return filteredImage;
	}
	else {
		return cv::Mat(640, 480, CV_8UC3, cv::Scalar(0, 0, 0));
	}
}

// Returnera koordinater. Om b�da metoderna ber�knat koordinater returneras
// snittet mellan dem. 
// TODO: Om det skiljer "mycket" mellan koordinaterna, returnera ist�llet den koordinat
// som �r n�rmast f�rra koordinaten som ber�knades. 
// TODO: Retuerna huruvida koordinaterna �r valid samtidigt, s� att man alltid vet om returnerade
// koordinater �r sanna. Annars finns risk att metoden ValidCoords k�rs efter att koordinaterna 
// �ndrats.
std::pair<int, int> Coords::GetCoords() {

	// L�s koordinaterna under ber�kning
	std::lock_guard<std::mutex> guard(coordsLock);

	// Om b�da metoderna gett koordinater, returnera mittpunkten
	if (validCoords & COORDS_CIRCLE & COORDS_FILTER) {
		std::pair<int, int> pos(0, 0);
		pos.first = (posFilter.first + posCircle.first) / 2;
		pos.second = (posFilter.second + posCircle.second) / 2;
		return pos;
	}

	// Annars, retunera de koordinater som LYCKATS ber�knas
	else if (validCoords & COORDS_CIRCLE) {
		return posCircle;
	}
	else if (validCoords & COORDS_FILTER) {
		return posFilter;
	}

	// Om ingen metod lyckats, returnera 0.
	else {
		return std::pair<int, int>(0, 0);
	}
}

// Ber�kna koordinater, med 0, 1 eller 2 metoder.
void Coords::CalculateCoords(const cv::Mat& imgOriginal) {
	
	// G�r koden mycket renare d� varje rad anv�nder cv-metoder
	using namespace cv;

	// Eftersom vi har b�rjat ber�kna fr�n en ny frame �r inte l�ngre koordinaterna 
	// giltiga.
	validCoords = false;
	
	// Eftersom vi h�ller p� att ber�kna �r vi inte redo att b�rja ber�kna.
	ready = false;

	// Skapa en semafor f�r koordinater, men l�s den inte (std::defer_lock).
	std::unique_lock<std::mutex> coordsGuard(coordsLock, std::defer_lock);

	// Skall vi ber�kna koordinater med filtermetoden?
	if (mode & COORDS_FILTER)
	{
		cv::Mat imgHSV;

		// Nedanst�ende kod f�rn OpenCV-exempel. Alla engelska kommentarer �r tagna fr�n 
		// exempelkoden.

		cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
		inRange(imgHSV, Scalar(filter.lowH, filter.lowS, filter.lowV),
			Scalar(filter.highH, filter.highS, filter.highV), imgHSV); //Threshold the image

		//morphological opening (removes small objects from the foreground)
		erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(2, 2)));
		dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(2, 2)));

		//morphological closing (removes small holes from the foreground)
		dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(2, 2)));
		erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(2, 2)));

		//Denna kod �r numera inte anv�ndbar utan b�r ist�llet skapas i den anropande klassen.

		//Calculate the moments of the thresholded image
		Moments oMoments = moments(imgHSV);
		double dM01 = oMoments.m01;
		double dM10 = oMoments.m10;
		double dArea = oMoments.m00;

		// L�s koordinaterna med tidigare skapad semafor. 
		coordsGuard.lock();
		
		// if the area <= 10000, I consider that the there are no object in the image and it's because of the noise, the area is not zero 
		if (dArea > 10000)
		{
			// calculate the position of the object
			posFilter.first = static_cast<int>(dM10 / dArea + 0.5);
			posFilter.second = static_cast<int>(dM01 / dArea + 0.5);

			// Eftersom vi lyckades ber�kna koordinaterna s� s�tter vi flaggan.
			validCoords |= COORDS_FILTER;
		}
		coordsGuard.unlock();

		//L�s bilden n�r vi sparar kopian f�r att f�rhindra att man l�ser en halvskriven bild
		std::unique_lock<std::mutex> filterGuard(filteredLock);
		filteredImage = imgHSV;
		filterGuard.unlock();
	}

	// Skall vi ber�kna position med cirkelmetod?
	if (mode & COORDS_CIRCLE) {
		
		// Nedanst�ende kod fr�n OpenCV-exempel. Kommentarer p� engelska �r fr�n exempelkoden.
		
		Mat src_gray;

		// Convert it to gray
		cvtColor(imgOriginal, src_gray, CV_BGR2GRAY);

		// Reduce the noise so we avoid false circle detection
		GaussianBlur(src_gray, src_gray, Size(1, 1), 0, 0);

		vector<Vec3f> circles;

		// Apply the Hough Transform to find the circles
		HoughCircles(src_gray, circles, CV_HOUGH_GRADIENT, 1, src_gray.rows / 8, 100, 50, 0, 23);

		cv::Mat imgCircles = src_gray;

		// H�ll reda p� vilken cirkel som �r st�rst och spara koordinaterna f�r den.
		// TODO: St�rst cirkel beh�ver inte vara r�tt cirkel. Hitta metod f�r att ist�llet
		// spara koordinaterna f�r b�sta cirkel.
		int largest = 0;
		float largestRadius = 0;
		
		// Draw the circles detected
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

		// L�s koordinaterna
		coordsGuard.lock();

		// Om st�rsta cirkeln �r 0 har vi inte hittat n�gon cirkel alls.
		if (largestRadius != 0) {
			posCircle.first = circles[largest][0];
			posCircle.second = circles[largest][1];
			validCoords |= COORDS_CIRCLE;
		}
		coordsGuard.unlock();

		// L�s cirkelbilden s� att inte halvf�rdig bild kan l�sas med GetCircledImage.
		std::lock_guard<std::mutex> guard(circledLock);
		circledImage = imgCircles;
	}
	ready = true;
}

// Rita ett kors. Anpassad fr�n OpenCV-exempel och gjord statisk d� den
// inte kr�ver n�gon instantiering.
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