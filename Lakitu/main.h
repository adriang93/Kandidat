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
	bool doneCalculating = false;
	bool started = false;
	bool cross = false;
	int mode = 0;
	int coordsMode = Coords::COORDS_FILTER;
	std::thread calcThread;
	Coords coords;
	Compass compass;
	cv::Mat returnImage;

	HANDLE consoleHandle;

	void calcCoordsCall(cv::Mat& image);

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