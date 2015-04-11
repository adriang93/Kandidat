#pragma once

/* 

Header för huvudmodulen som initerar alla värden. Baserad på exempel 13.2 från OculusRiftInAction.

TODO: Länk till licens här.

*/

#include "stdafx.h"

// Våra lokala klasser 
#include "Coords.h" 
#include "Compass.h"
#include "StreamHandler.h"

// Läsa filer, std::fstream samt std::cout och ::cin
#include <iostream>

// Från OculusRiftInAction
#include "Resources.h"
#include "ResourceEnums.h"

// Behövs för att alla shaders skall laddas in
#include "oglplus/texture_filter.hpp"
#include "Common.h"


class WebcamApp : public RiftApp {
private:

	// För att hålla reda på vilken rad varje funktion skall skriva text på i konsollen
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

	// Har vi börjat beräkna position i bilden? 
	bool started = false;

	// Vill vi rita ett kors för att markera aktuell beräknad position?
	bool cross = false;
	
	// Vilken sorts bild vill vi extrahera; filtrerad, circelberäknad eller originalbild?
	int displayMode = 0;
	
	// Standardläget är att bildbehandlingsmodulen enbart filtrerar, ej letar cirklar.
	int coordsMode = Coords::COORDS_FILTER;
	
	// Tråden som beräknar position i bilden
	std::thread calcThread;

	Coords coords;
	Compass compass;
	StreamHandler captureHandler;
		
	cv::Mat returnImage;

	// Behövs för att kunna flytta markören i konsollen för att skriva på valfri rad
	HANDLE consoleHandle;

	void calcCoordsCall(cv::Mat& image);
	void SetConsoleRow(int);

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