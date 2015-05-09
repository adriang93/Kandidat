/* 

Header för bildbehandlingsmodulen. 

Baserad på exempelkod från openCV. 

TODO: Länk till licens för openCV-exemplen.

*/

#pragma once

#include "stdafx.h"
#include <limits>
#include <math.h>
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

	Coords(Coords::HSVfilter& filter);
	Coords();

	void SetHSV(Coords::HSVfilter& filter);
	bool Ready();
	bool ValidCoords();
	cv::Mat GetFilteredImage();
	std::pair<int, int> GetCoords();
	void CalculateCoords(const cv::Mat& image, int minArea, int maxArea, int minCircularity, int open, int close);
	static void DrawCross(int x, int y, cv::Mat& image);

private:
	HSVfilter filter;
	cv::Mat filteredImage;
	std::pair<int, int> posFilter;

	// Anger att bildbehandling genomförts. Sätts till true i slutet av beräkningskoden.
	bool ready = false;
	
	// Anger om vi har lyckats beräkna koordinater.
	bool validCoords = false;

	// Standardläget är att bara filtrera, inte detektera cirklar.
	std::mutex coordsLock;
	std::mutex filteredLock;
};