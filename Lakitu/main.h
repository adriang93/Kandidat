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

// L�sa filer, std::fstream samt std::cout och ::cin
#include <iostream>
#include <cctype>
#include <string>

// Fr�n OculusRiftInAction
#include "Resources.h"
#include "ResourceEnums.h"

// Beh�vs f�r att alla shaders skall laddas in
#include "oglplus/texture_filter.hpp"
#include "Common.h"


class WebcamApp : public RiftApp {
private:
	// Har vi b�rjat ber�kna position i bilden? 
	bool started = false;

	// Vill vi rita ett kors f�r att markera aktuell ber�knad position?
	bool cross = true;
	
	// Vilken sorts bild vill vi visa; originalbild eller filtrearad bild?
	int displayMode = 1;

	// parametrar f�r objektdetekteringen
	int minArea;
	int maxArea;
	int minCircularity;
	int open;
	int close;

	// parameter f�r kompassfiltreringen
	int smoothing; 

	// Portnummer f�r koimmunikation med Mission Planner-scriptet
	int port;
	std::string host;
	
	// Tr�den som ber�knar position i bilden
	std::thread calcThread;

	Coords coords;
	Compass compass;
	StreamHandler captureHandler;
	NavigatorComm* navigator;
		
	cv::Mat returnImage;

	void calcCoordsCall(cv::Mat& image);
	void SetConsoleRow(int);

	// �rvda fr�n exempel 13.2. Anv�nds f�r renderingen.
	TexturePtr texture;
	ProgramPtr program;
	ShapeWrapperPtr videoGeometry;
	Coords::HSVfilter filter;

public:
	GLboolean glewExperimental;
	// Dessa kallas av moderklassen och �r �rvda metoder fr�n 
	// RiftApp fr�n OculusRiftInAction-resurserna
	WebcamApp();
	virtual ~WebcamApp();
	void initGl();
	virtual void update();
	virtual void onKey(int key, int scancode, int action, int mods);
	virtual void renderScene();
};