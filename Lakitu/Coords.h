/* 

Klass f�r ber�kning av koordinater med hj�lp av objektdetektering i en bildruta.

Anv�nder OpenCV f�r mycket av bildbehandlingen.

All kod skriven av Andr� Wallstr�m. Vissa funktionsprototyper dock baserade p� exempelkod.

*/

#pragma once

#include "stdafx.h"
#include <limits>
#include <math.h>
#include <mutex> // semaforer

class Coords
{
public:

	// Publik klass f�r att skicka och ta emot parameterv�rden f�r filtreringen.
	// Alla interna variablar �r publika f�r att man skall kunna komma �t dem 
	// direkt utan set/get f�r enklare kod.
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

	// Hj�lpklass f�r att representera en koordinat, och huruvida den �r giltig.
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
	// Senast ber�knade koordinaten. Beh�ver inte komma fr�n senaste ber�kningsrundan.
	// Om senaste ber�kningen misslyckades att hitta en koordinat beh�lls den f�rra koordinaten
	// men markeras som ogiltig.
	Coord coord;

	// Anger att bildbehandling genomf�rts. S�tts till true i slutet av ber�kningskoden.
	bool ready = false;
	
	// Semaforer f�r att undvika att halvf�rdiga koordinater l�ses
	std::mutex coordsLock;
	std::mutex filteredLock;
};