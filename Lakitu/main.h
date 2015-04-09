#pragma once

#include "stdafx.h"
#include "Coords.h"
#include "Compass.h"
#include "WebcamHandler.h"
#include <iostream>
#include "Resources.h"
#include "ResourceEnums.h"
#include "oglplus/texture_filter.hpp"
#include "Common.h"


class WebcamApp : public RiftApp {
private:
	enum rows {
		headingOVRRow = 0,
		headingDroneRow = 1,
		distanceRow = 2,
		modeRow = 3,
		coordsModeRow = 4,
		posRow = 5,
		validRow = 6,
		corrRow = 7
	};
	bool doneCalculating = false;
	bool started = false;
	bool cross = false;
	int displayMode = 0;
	int coordsMode = Coords::COORDS_FILTER;
	std::thread calcThread;
	Coords coords;
	Compass compass;
	cv::Mat returnImage;

	HANDLE consoleHandle;

	void calcCoordsCall(cv::Mat& image);
	void SetConsoleRow(int);

	TexturePtr texture;
	ProgramPtr program;
	ShapeWrapperPtr videoGeometry;
	WebcamHandler captureHandler;

public:

	WebcamApp();
	virtual ~WebcamApp();
	void initGl();
	virtual void update();
	virtual void onKey(int key, int scancode, int action, int mods);
	virtual void renderScene();
};