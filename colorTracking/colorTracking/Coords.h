#pragma once

#include "stdafx.h"
#include <mutex>

class Coords
{
public:

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

	Coords(Coords::HSVfilter);
	Coords();

	void SetHSV(Coords::HSVfilter);
	bool Ready();
	bool ValidCoords();
	cv::Mat GetFilteredImage();
	std::pair<int, int> GetCoords();
	void CalculateCoords(const cv::Mat&);

private:
	void SetCoords(int, int);
	static void DrawCross(int x, int y, cv::Mat&);
	HSVfilter filter;
	cv::Mat filteredImage;
	int posX;
	int posY;
	bool ready;
	bool validCoords;
	std::mutex coordsLock;
	std::mutex imageLock;
};