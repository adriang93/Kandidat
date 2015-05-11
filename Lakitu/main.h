#pragma once

/* 

Header f�r huvudmodulen som initerar alla v�rden. Baserad p� exempel 13.2 fr�n OculusRiftInAction.

TODO: L�nk till licens h�r.

*/

#include "stdafx.h"

// V�ra lokala klasser 
#include "StreamHandler.h"
#include "Coords.h" 
#include "Compass.h"
#include "NavigatorComm.h"

// L�sa filer, std::fstream
#include <iostream>
#include <string>

// Fr�n OculusRiftInAction
#include "Resources.h"
#include "ResourceEnums.h"

// Beh�vs f�r att alla shaders skall laddas in
#include "oglplus/texture_filter.hpp"
#include "Common.h"


class WebcamApp : public RiftApp {
private:
	// Tr�den som ber�knar position i bilden
	std::thread calcThread;
	// Har vi b�rjat ber�kna position i bilden? 
	bool started = false;
	// Vilken sorts bild vill vi visa; originalbild eller filtrearad bild?
	int displayMode = 1;
	// Vill vi rita ett kors f�r att markera aktuell ber�knad position?
	bool cross = true;

	// Storleken, i millimeter, p� objektet som skall detekteras samt horisontell fov. 
	// Anv�nds f�r avst�ndsbest�mning
	int distance = 0;
	int objSize;
	float horisontalFov;
	int distanceCutoff;

	// Portnummer f�r koimmunikation med Mission Planner-scriptet
	int port;
	
	// Handtag till konsollen
	HANDLE consoleHandle;
	bool console = false;

	// Parametrar f�r objektdetekteringen
	Coords::CoordsFilter filter;
	bool interlaced;
	Coords coords;

	// parameter f�r kompassfiltreringen
	int compassCutoff;
	Compass compass;

	StreamHandler captureHandler;
	int frameDelay = 0;

	NavigatorComm* navigator;
		
	cv::Mat returnImage;

	void calcCoordsCall(cv::Mat& image);

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