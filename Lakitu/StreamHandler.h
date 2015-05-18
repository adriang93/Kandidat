/* 

Header för streamhandlern. Taget från OculusRiftInActions exempel 13.2 med små modifikationer.

https://github.com/OculusRiftInAction/OculusRiftInAction

All kod skriven av André Wallström baserat på exempel 13.2 ovan (som saknade headerfil).
Flera av de publika metodprototyperna är baserade på exempelkoden. Vilka som är skrivna av 
projektet och vilka som är från exempekoden framgår i källkodsfilen.

*/

#pragma once

#include "stdafx.h"
#include <mutex> // semafor
#include <thread> // std::thread
#include <opencv2/opencv.hpp> 
#include <string>
#include "Common.h" // Resurser från OculusRiftInAction

class StreamHandler {
private:
	// Vilken enhet skall vi öppna? Standard, om inget annat sätts, är device 0.
	int device = 0;
	// Filnamn, om sådant angetts
	std::string file;
	// Använder vi filnamn eller enhetsnummer?
	bool useFile = false;

	// Finns en bildruta att hämta?
	bool hasFrame = false;
	// Har vi stoppats?
	bool stopped = false;

	// Fördröjningen mellan bildrutor. Måste beräknas.
	int frameDelay;

	// Tråd för videoextrahering
	std::thread captureThread;

	// Objektet som hanterar videoströmmen
	cv::VideoCapture videoCapture;
	// Semafor för att göra det omöjligt att läsa halva bildrutor
	std::mutex mutex;
	// Bildrutan som hämtats
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