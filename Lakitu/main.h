#pragma once

/* 

Header f�r huvudmodulen som initerar alla v�rden. Baserad p� exempel 13.2 fr�n OculusRiftInAction.

TODO: L�nk till licens h�r.

*/

#include "stdafx.h"

// V�ra lokala klasser 
#include "Coords.h" 
#include "Compass.h"
#include "StreamHandler.h"

// L�sa filer, std::fstream samt std::cout och ::cin
#include <iostream>

// Fr�n OculusRiftInAction
#include "Resources.h"
#include "ResourceEnums.h"

// Beh�vs f�r att alla shaders skall laddas in
#include "oglplus/texture_filter.hpp"
#include "Common.h"


class WebcamApp : public RiftApp {
private:

	// F�r att h�lla reda p� vilken rad varje funktion skall skriva text p� i konsollen
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

	// Har vi b�rjat ber�kna position i bilden? 
	bool started = false;

	// Vill vi rita ett kors f�r att markera aktuell ber�knad position?
	bool cross = true;
	
	// Vilken sorts bild vill vi extrahera; filtrerad, circelber�knad eller originalbild?
	int displayMode = 1;
	
	// Standardl�get �r att bildbehandlingsmodulen enbart filtrerar, ej letar cirklar.
	int coordsMode = Coords::COORDS_FILTER && Coords::COORDS_CIRCLE;
	
	// Tr�den som ber�knar position i bilden
	std::thread calcThread;

	Coords coords;
	Compass compass;
	StreamHandler captureHandler;
		
	cv::Mat returnImage;

	// Beh�vs f�r att kunna flytta mark�ren i konsollen f�r att skriva p� valfri rad
	HANDLE consoleHandle;

	void calcCoordsCall(cv::Mat& image);
	void SetConsoleRow(int);

	// �rvda fr�n exempel 13.2. Anv�nds f�r renderingen.
	TexturePtr texture;
	ProgramPtr program;
	ShapeWrapperPtr videoGeometry;

public:

	// Dessa kallas av moderklassen och �r �rvda metoder fr�n 
	// RiftApp fr�n OculusRiftInAction-resurserna
	WebcamApp();
	virtual ~WebcamApp();
	void initGl();
	virtual void update();
	virtual void onKey(int key, int scancode, int action, int mods);
	virtual void renderScene();
};