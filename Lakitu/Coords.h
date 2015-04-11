/* 

Header för bildbehandlingsmodulen. 

Baserad på exempelkod från openCV. 

TODO: Länk till licens för openCV-exemplen.

*/

#pragma once

#include "stdafx.h"
#include <mutex> // semaforer

class Coords
{
public:

	// Publik klass för att skicka och ta emot filtervärden för HSV-filtreringen.
	// Tomma konstruktorn ger defaultvärden. Alla interna variablar är publika för
	// att man skall kunna komma åt dem direkt utan set/get för enklare kod.
	class HSVfilter {
	public:
		HSVfilter(int lowH, int highH, int lowS, int highS, int lowV, int highV) :
			lowH(lowH), highH(highH), lowS(lowS), highS(highS), lowV(lowV), highV(highV) {
		}
		HSVfilter() {
			HSVfilter(0, 0, 0, 0, 0, 0);
		}
		int lowH;
		int highH;
		int lowS;
		int highS;
		int lowV;
		int highV;
	};

	Coords(Coords::HSVfilter&);
	Coords();

	// Bitflaggor för de två olika lägena som kan vara aktiverade oberoende av varandra.
	enum modes {
		COORDS_FILTER = 1,
		COORDS_CIRCLE = 2
	};

	void SetHSV(Coords::HSVfilter&);
	void SetMode(int);
	bool Ready();
	int ValidCoords();
	float GetCorrellation();
	cv::Mat GetFilteredImage();
	cv::Mat GetCircledImage();
	std::pair<int, int> GetCoords();
	void CalculateCoords(const cv::Mat&);
	static void DrawCross(int x, int y, cv::Mat&);

private:
	HSVfilter filter;
	cv::Mat filteredImage;
	cv::Mat circledImage;
	std::pair<int, int> posFilter;
	std::pair<int, int> posCircle;
	
	// Anger att bildbehandling genomförts. Sätts till true i slutet av beräkningskoden.
	bool ready = false;
	
	// Anger om vi har lyckats beräkna koordinater. Använder bitvärdena från modes för att ange
	// kombination av giltiga koordinater för de två olika metoderna.
	int validCoords = 0;

	// Standardläget är att bara filtrera, inte detektera cirklar.
	int mode = COORDS_FILTER;
	std::mutex coordsLock;
	std::mutex filteredLock;
	std::mutex circledLock;
};