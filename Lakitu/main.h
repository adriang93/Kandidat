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

// Läsa filer, std::fstream
#include <iostream>
#include <string>

// Från OculusRiftInAction
#include "Resources.h"
#include "ResourceEnums.h"

// Behövs för att alla shaders skall laddas in
#include "oglplus/texture_filter.hpp"
#include "Common.h"


class WebcamApp : public RiftApp {
private:
	// Tråden som beräknar position i bilden
	std::thread calcThread;
	// Har vi börjat beräkna position i bilden? 
	bool started = false;
	// Vilken sorts bild vill vi visa; originalbild eller filtrearad bild?
	int displayMode = 1;
	// Vill vi rita ett kors för att markera aktuell beräknad position?
	bool cross = true;

	// Storleken, i millimeter, på objektet som skall detekteras samt horisontell fov. 
	// Används för avståndsbestämning
	int distance = 0;
	int objSize;
	float horisontalFov;
	int distanceCutoff;

	// Portnummer för koimmunikation med Mission Planner-scriptet
	int port;
	
	// Handtag till konsollen
	HANDLE consoleHandle;
	bool console = false;

	// Parametrar för objektdetekteringen
	Coords::CoordsFilter filter;
	bool interlaced;
	Coords coords;

	// parameter för kompassfiltreringen
	int compassCutoff;
	Compass compass;

	StreamHandler captureHandler;
	int frameDelay = 0;

	NavigatorComm* navigator;
		
	cv::Mat returnImage;

	void calcCoordsCall(cv::Mat& image);

	// Ärvda från exempel 13.2. Används för renderingen.
	TexturePtr texture;
	ProgramPtr program;
	ShapeWrapperPtr videoGeometry;

public:
	// Dessa kallas av moderklassen och är ärvda metoder från 
	// RiftApp från OculusRiftInAction-resurserna
	WebcamApp();
	virtual ~WebcamApp();
	void initGl();
	virtual void update();
	virtual void onKey(int key, int scancode, int action, int mods);
	virtual void renderScene();
};