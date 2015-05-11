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

	// Publik klass för att skicka och ta emot parametervärden för filtreringen.
	// Alla interna variablar är publika för att man skall kunna komma åt dem 
	// direkt utan set/get för enklare kod.
	class CoordsFilter {
	public:
		CoordsFilter() {}
		int lowH;
		int highH;
		int lowS;
		int highS;
		int lowV;
		int highV;
		int minArea;
		int maxArea;
		int minCircularity;
		int open;
		int close;
	};

	// Hjälpklass för att representera en koordinat, och huruvida den är giltig.
	class Coord {
	public:
		Coord() {}
		float x = 0;
		float y = 0;
		float size = 0;
		bool valid = false;
	};

	Coords();

	bool Ready();
	cv::Mat GetFilteredImage();
	Coord GetCoords();
	
	void CalculateCoords(const cv::Mat& image, CoordsFilter params, bool interlaced);
	static void DrawCross(int x, int y, cv::Mat& image);

private:
	cv::Mat filteredImage;
	Coord coord;

	// Anger att bildbehandling genomförts. Sätts till true i slutet av beräkningskoden.
	bool ready = false;
	
	// Standardläget är att bara filtrera, inte detektera cirklar.
	std::mutex coordsLock;
	std::mutex filteredLock;
};