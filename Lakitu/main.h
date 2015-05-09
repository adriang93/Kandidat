#pragma once

/* 

Header för huvudmodulen som initerar alla värden. Baserad på exempel 13.2 från OculusRiftInAction.

TODO: Länk till licens här.

*/

#include "stdafx.h"

// Våra lokala klasser 
#include "StreamHandler.h"
#include "Coords.h" 
#include "Compass.h"
#include "NavigatorComm.h"

// Läsa filer, std::fstream samt std::cout och ::cin
#include <iostream>
#include <cctype>
#include <string>

// Från OculusRiftInAction
#include "Resources.h"
#include "ResourceEnums.h"

// Behövs för att alla shaders skall laddas in
#include "oglplus/texture_filter.hpp"
#include "Common.h"


class WebcamApp : public RiftApp {
private:
	// Har vi börjat beräkna position i bilden? 
	bool started = false;

	// Vill vi rita ett kors för att markera aktuell beräknad position?
	bool cross = true;
	
	// Vilken sorts bild vill vi visa; originalbild eller filtrearad bild?
	int displayMode = 1;

	// parametrar för objektdetekteringen
	int minArea;
	int maxArea;
	int minCircularity;
	int open;
	int close;

	// parameter för kompassfiltreringen
	int smoothing; 

	// Portnummer för koimmunikation med Mission Planner-scriptet
	int port;
	std::string host;
	
	// Tråden som beräknar position i bilden
	std::thread calcThread;

	Coords coords;
	Compass compass;
	StreamHandler captureHandler;
	NavigatorComm* navigator;
		
	cv::Mat returnImage;

	void calcCoordsCall(cv::Mat& image);
	void SetConsoleRow(int);

	// Ärvda från exempel 13.2. Används för renderingen.
	TexturePtr texture;
	ProgramPtr program;
	ShapeWrapperPtr videoGeometry;
	Coords::HSVfilter filter;

public:
	GLboolean glewExperimental;
	// Dessa kallas av moderklassen och är ärvda metoder från 
	// RiftApp från OculusRiftInAction-resurserna
	WebcamApp();
	virtual ~WebcamApp();
	void initGl();
	virtual void update();
	virtual void onKey(int key, int scancode, int action, int mods);
	virtual void renderScene();
};