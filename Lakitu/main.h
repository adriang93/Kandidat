#pragma once

/* 

Header f�r huvudmodulen som initerar alla v�rden. Baserad p� exempel 13.2 fr�n OculusRiftInAction.

https://github.com/OculusRiftInAction/OculusRiftInAction

All kod skriven av Andr� Wallstr�m baserat p� exempel 13.2 ovan (som saknade headerfil).
Flera av de publika metodprototyperna �r baserade p� exempelkoden. Vilka som �r skrivna av
projektet och vilka som �r fr�n exempekoden framg�r i k�llkodsfilen.

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
#include "oglplus/texture_filter.hpp"
#include "Common.h"

// RiftApp �r en klass fr�n OculusRiftExamples som hanterar rendering och
// kommunikation med Oculus Rift
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
	
	// Handtag till konsollen. Anv�nds ej
	HANDLE consoleHandle;
	// S�tts till true via filinl�sning om konsoll skall anv�ndas
	bool console = false;

	// Parametrar f�r objektdetekteringen
	Coords::CoordsFilter filter;
	bool interlaced;
	// Koordinatber�kningsklassen
	Coords coords;

	// parameter f�r kompassfiltreringen
	int compassCutoff;
	// Kompassber�kningsklassen
	Compass compass;

	// Klassen som extraherar data fr�n videostr�m
	StreamHandler captureHandler;
	// Anv�nds f�r att l�gpassfiltrera avst�ndsber�kningen
	int frameDelay = 0;

	// Kommunikationsklass f�r att skcika data till navigationsmodulen
	NavigatorComm* navigator;

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