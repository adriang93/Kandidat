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

// Sätt vilka beräkningsmetoder som skalla användas. Meningen är att de olika
// bitflaggorna i modes skall användas, men kontrollerar inte detta. Alla int-värden
// accepteras men bara de två minsta bittarna kontrolleras i koden.
void Coords::SetMode(int newmode) {
	mode = newmode;
}

bool Coords::Ready() {
	return ready;
}

// Returnerar en kombination av COORDS_CIRCLE och COORDS_FILTER beroende på vilka
// metoder som lyckats beräkna koordinater.
int Coords::ValidCoords() {
	return validCoords;
}

// Beräkna avståndet (inte egentligen korrelationen) mellan de två beräknade koordinaterna, 
// eller returnera 0 om inga eller bara en beräknad koordinat finns.
float Coords::GetCorrellation() {
	
	// Förhindra att koordinaterna ändras under beräkning
	// lock_guard låses upp automatiskt när den hamnar utanför scope
	std::lock_guard<std::mutex> guard(coordsLock); 

	// Har båda metoderna lyckats beräkna koordinater?
	if (validCoords & COORDS_CIRCLE & COORDS_FILTER) {
		
		//Beräkna avståndet
		int distX = posFilter.first - posCircle.first;
		int distY = posFilter.second - posCircle.second;
		return sqrt(distX*distX + distY*distY);
	}
	else {
		return 0;
	}
}

// Returnerar de olika typerna av filterade bilder. Med circlar (CircledImage) eller filtrerad
// (FilteredImage): Returnerar en kopia, inte en referens, för att det skall vara säkert för 
// mottagaren att inte bilden modifieras.
cv::Mat Coords::GetCircledImage() {

	// Lås bilden för att förhindra att den modifieras under kopiering.
	std::lock_guard<std::mutex> guard(circledLock);

	// TODO: Bör alltid returnera en bild om en sådan existerat någon gång. Nu så returneras en 
	// blank bild om beräkningskoden börjat köras (och ready = false) istället för att returnera den
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

// Returnera koordinater. Om båda metoderna beräknat koordinater returneras
// snittet mellan dem. 
// TODO: Om det skiljer "mycket" mellan koordinaterna, returnera istället den koordinat
// som är närmast förra koordinaten som beräknades. 
// TODO: Retuerna huruvida koordinaterna är valid samtidigt, så att man alltid vet om returnerade
// koordinater är sanna. Annars finns risk att metoden ValidCoords körs efter att koordinaterna 
// ändrats.
std::pair<int, int> Coords::GetCoords() {

	// Lås koordinaterna under beräkning
	std::lock_guard<std::mutex> guard(coordsLock);

	// Om båda metoderna gett koordinater, returnera mittpunkten
	if (validCoords & COORDS_CIRCLE & COORDS_FILTER) {
		std::pair<int, int> pos(0, 0);
		pos.first = (posFilter.first + posCircle.first) / 2;
		pos.second = (posFilter.second + posCircle.second) / 2;
		return pos;
	}

	// Annars, retunera de koordinater som LYCKATS beräknas
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

// Beräkna koordinater, med 0, 1 eller 2 metoder.
void Coords::CalculateCoords(const cv::Mat& imgOriginal) {
	
	// Gör koden mycket renare då varje rad använder cv-metoder
	using namespace cv;

	// Eftersom vi har börjat beräkna från en ny frame är inte längre koordinaterna 
	// giltiga.
	validCoords = false;
	
	// Eftersom vi håller på att beräkna är vi inte redo att börja beräkna.
	ready = false;

	// Skapa en semafor för koordinater, men lås den inte (std::defer_lock).
	std::unique_lock<std::mutex> coordsGuard(coordsLock, std::defer_lock);

	// Skall vi beräkna koordinater med filtermetoden?
	if (mode & COORDS_FILTER)
	{
		cv::Mat imgHSV;
		cv::Mat imgHSV2;

		// Nedanstående kod fårn OpenCV-exempel. Alla engelska kommentarer är tagna från 
		// exempelkoden.

		cvtColor(imgOriginal, imgHSV, COLOR_BGR2HSV); //Convert the captured frame from BGR to HSV
		imgHSV2 = imgHSV.clone();
		inRange(imgHSV, Scalar(filter.lowH, filter.lowS, filter.lowV),
			Scalar(filter.highH, filter.highS, filter.highV), imgHSV); //Threshold the image

		inRange(imgHSV2, Scalar(0, 0, 245),
			Scalar(255, 255, 255), imgHSV2); //Threshold the image

		imgHSV = imgHSV + imgHSV2;

		//morphological opening (removes small objects from the foreground)
		erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

		//morphological closing (removes small holes from the foreground)
		dilate(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));
		erode(imgHSV, imgHSV, getStructuringElement(MORPH_ELLIPSE, Size(5, 5)));

		//http://stackoverflow.com/questions/8076889/tutorial-on-opencv-simpleblobdetector

		cv::SimpleBlobDetector::Params params;
		params.minDistBetweenBlobs = 50.0f;
		params.filterByInertia = false;
		params.filterByConvexity = false;
		params.filterByColor = false;
		params.filterByCircularity = true;
		params.filterByArea = true;
		params.minCircularity = 0.65;
		//params.maxCircularity = ;
		params.minArea = 60;
		params.maxArea = 1200;


		SimpleBlobDetector detector(params);
		std::vector<KeyPoint> keypoints;
		
		detector.detect(imgHSV, keypoints);
		
		Mat im_with_keypoints;
		
		drawKeypoints(imgHSV, keypoints, im_with_keypoints, Scalar(0, 0, 255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);


		//Calculate the moments of the thresholded image
		//Moments oMoments = moments(imgHSV);
		//double dM01 = oMoments.m01;
		//double dM10 = oMoments.m10;
		//double dArea = oMoments.m00;

		// Lås koordinaterna med tidigare skapad semafor. 
		coordsGuard.lock();
		
		// if the area <= 10000, I consider that the there are no object in the image and it's because of the noise, the area is not zero 
		int mindist = 1000000;
		int dist = 0;
		int disty = 0; 
		int distx = 0;
		int newposx, newposy;
		int size = keypoints.size();
		if (size > 0)
		{
			
			for (int i = 0; i < size; i++) {
				int kx = keypoints[i].pt.x;
				int ky = keypoints[i].pt.y;
				disty = posFilter.first - kx;
				distx = posFilter.second - ky;
				dist = sqrt((disty*disty) + (distx*distx));
				if (dist < mindist) {
					mindist = dist;
					newposx = keypoints[i].pt.x;// static_cast<int>(dM10 / dArea + 0.5);
					newposy = keypoints[i].pt.y; // static_cast<int>(dM01 / dArea + 0.5);
				}
			}
			// Eftersom vi lyckades beräkna koordinaterna så sätter vi flaggan.
			validCoords |= COORDS_FILTER;
			posFilter.first = newposx;
			posFilter.second = newposy;
		}
		coordsGuard.unlock();

		//Lås bilden när vi sparar kopian för att förhindra att man läser en halvskriven bild
		std::unique_lock<std::mutex> filterGuard(filteredLock);
		filteredImage = im_with_keypoints;
		filterGuard.unlock();
	}
	/*
	// Skall vi beräkna position med cirkelmetod? Används ej!
	if (mode & COORDS_CIRCLE) {
		
		// Nedanstående kod från OpenCV-exempel. Kommentarer på engelska är från exempelkoden.
		
		Mat src_gray;

		// Convert it to gray
		cvtColor(imgOriginal, src_gray, CV_BGR2GRAY);

		// Reduce the noise so we avoid false circle detection
		GaussianBlur(src_gray, src_gray, Size(1, 1), 0, 0);

		vector<Vec3f> circles;

		// Apply the Hough Transform to find the circles
		HoughCircles(src_gray, circles, CV_HOUGH_GRADIENT, 1, src_gray.rows / 8, 200, 100, 0, 23);

		cv::Mat imgCircles = src_gray;

		// Håll reda på vilken cirkel som är störst och spara koordinaterna för den.
		// TODO: Störst cirkel behöver inte vara rätt cirkel. Hitta metod för att istället
		// spara koordinaterna för bästa cirkel.
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

		// Lås koordinaterna
		coordsGuard.lock();

		// Om största cirkeln är 0 har vi inte hittat någon cirkel alls.
		if (largestRadius != 0) {
			posCircle.first = circles[largest][0];
			posCircle.second = circles[largest][1];
			validCoords |= COORDS_CIRCLE;
		}
		coordsGuard.unlock();

		// Lås cirkelbilden så att inte halvfärdig bild kan läsas med GetCircledImage.
		std::lock_guard<std::mutex> guard(circledLock);
		circledImage = imgCircles;
	}*/
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