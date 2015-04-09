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

	Coords(Coords::HSVfilter&);
	Coords();

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
	bool ready;
	int validCoords;
	int mode = COORDS_FILTER;
	std::mutex coordsLock;
	std::mutex filteredLock;
	std::mutex circledLock;
};