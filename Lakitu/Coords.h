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

	Coords(Coords::HSVfilter& filter);
	Coords();

	void SetHSV(Coords::HSVfilter& filter);
	void SetMode(int);
	bool Ready();
	bool ValidCoords();
	cv::Mat GetFilteredImage();
	std::pair<int, int> GetCoords();
	void CalculateCoords(const cv::Mat& image, int minArea, int maxArea, int minCircularity);
	static void DrawCross(int x, int y, cv::Mat& image);

private:
	HSVfilter filter;
	cv::Mat filteredImage;
	std::pair<int, int> posFilter;

	// Anger att bildbehandling genomf�rts. S�tts till true i slutet av ber�kningskoden.
	bool ready = false;
	
	// Anger om vi har lyckats ber�kna koordinater.
	bool validCoords = false;

	// Standardl�get �r att bara filtrera, inte detektera cirklar.
	std::mutex coordsLock;
	std::mutex filteredLock;
};