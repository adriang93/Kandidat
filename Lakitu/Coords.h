/* 

Header f�r bildbehandlingsmodulen. 

Baserad p� exempelkod fr�n openCV. 

TODO: L�nk till licens f�r openCV-exemplen.

*/

#pragma once

#include "stdafx.h"
#include <mutex> // semaforer

class Coords
{
public:

	// Publik klass f�r att skicka och ta emot filterv�rden f�r HSV-filtreringen.
	// Tomma konstruktorn ger defaultv�rden. Alla interna variablar �r publika f�r
	// att man skall kunna komma �t dem direkt utan set/get f�r enklare kod.
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

	// Bitflaggor f�r de tv� olika l�gena som kan vara aktiverade oberoende av varandra.
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
	
	// Anger att bildbehandling genomf�rts. S�tts till true i slutet av ber�kningskoden.
	bool ready = false;
	
	// Anger om vi har lyckats ber�kna koordinater. Anv�nder bitv�rdena fr�n modes f�r att ange
	// kombination av giltiga koordinater f�r de tv� olika metoderna.
	int validCoords = 0;

	// Standardl�get �r att bara filtrera, inte detektera cirklar.
	int mode = COORDS_FILTER;
	std::mutex coordsLock;
	std::mutex filteredLock;
	std::mutex circledLock;
};