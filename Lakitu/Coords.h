/* 

Klass för beräkning av koordinater med hjälp av objektdetektering i en bildruta.

Använder OpenCV för mycket av bildbehandlingen.

All kod skriven av André Wallström. Vissa funktionsprototyper dock baserade på exempelkod.

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

	// Endast tom konstruktor
	Coords();

	bool Ready();
	cv::Mat GetFilteredImage();
	Coord GetCoords();
	
	void CalculateCoords(const cv::Mat& image, CoordsFilter params, 
		bool interlaced, bool returnFiltered);
	static void DrawCross(int x, int y, cv::Mat& image);

private:
	// Senast filtrerade bilden
	cv::Mat filteredImage;
	// Senast beräknade koordinaten. Behöver inte komma från senaste beräkningsrundan.
	// Om senaste beräkningen misslyckades att hitta en koordinat behålls den förra koordinaten
	// men markeras som ogiltig.
	Coord coord;

	// Anger att bildbehandling genomförts. Sätts till true i slutet av beräkningskoden.
	bool ready = false;
	
	// Semaforer för att undvika att halvfärdiga koordinater läses
	std::mutex coordsLock;
	std::mutex filteredLock;
};