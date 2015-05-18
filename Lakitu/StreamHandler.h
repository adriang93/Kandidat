/* 

Header f�r streamhandlern. Taget fr�n OculusRiftInActions exempel 13.2 med sm� modifikationer.

https://github.com/OculusRiftInAction/OculusRiftInAction

All kod skriven av Andr� Wallstr�m baserat p� exempel 13.2 ovan (som saknade headerfil).
Flera av de publika metodprototyperna �r baserade p� exempelkoden. Vilka som �r skrivna av 
projektet och vilka som �r fr�n exempekoden framg�r i k�llkodsfilen.

*/

#pragma once

#include "stdafx.h"
#include <mutex> // semafor
#include <thread> // std::thread
#include <opencv2/opencv.hpp> 
#include <string>
#include "Common.h" // Resurser fr�n OculusRiftInAction

class StreamHandler {
private:
	// Vilken enhet skall vi �ppna? Standard, om inget annat s�tts, �r device 0.
	int device = 0;
	// Filnamn, om s�dant angetts
	std::string file;
	// Anv�nder vi filnamn eller enhetsnummer?
	bool useFile = false;

	// Finns en bildruta att h�mta?
	bool hasFrame = false;
	// Har vi stoppats?
	bool stopped = false;

	// F�rdr�jningen mellan bildrutor. M�ste ber�knas.
	int frameDelay;

	// Tr�d f�r videoextrahering
	std::thread captureThread;

	// Objektet som hanterar videostr�mmen
	cv::VideoCapture videoCapture;
	// Semafor f�r att g�ra det om�jligt att l�sa halva bildrutor
	std::mutex mutex;
	// Bildrutan som h�mtats
	cv::Mat frame;

public:
	StreamHandler();
	void SetDevice(int);
	void SetFile(std::string);

	float StartCapture();
	void StopCapture();

	int GetDelay();

	void SetFrame(const cv::Mat& newFrame);
	bool GetFrame(cv::Mat& out);

	void CaptureLoop();
};