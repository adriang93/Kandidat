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
	bool filtered = false;
	bool cross = false;
	std::thread calcThread;
	Coords coords;
	cv::Mat returnImage;

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