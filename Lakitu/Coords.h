/* 

Header f�r bildbehandlingsmodulen. 

Baserad p� exempelkod fr�n openCV. 

TODO: L�nk till licens f�r openCV-exemplen.

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

	Coords();

	bool Ready();
	cv::Mat GetFilteredImage();
	Coord GetCoords();
	
	void CalculateCoords(const cv::Mat& image, CoordsFilter params, bool interlaced);
	static void DrawCross(int x, int y, cv::Mat& image);

private:
	cv::Mat filteredImage;
	Coord coord;

	// Anger att bildbehandling genomf�rts. S�tts till true i slutet av ber�kningskoden.
	bool ready = false;
	
	// Standardl�get �r att bara filtrera, inte detektera cirklar.
	std::mutex coordsLock;
	std::mutex filteredLock;
};