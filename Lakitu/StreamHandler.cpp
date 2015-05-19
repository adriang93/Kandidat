/* 

Hanterar str�mmar fr�n videoenheter. Taget fr�n exempel 13.2 fr�n OculusRiftInAction
https://github.com/OculusRiftInAction/OculusRiftInAction/
Licensierad under Apache 2.0

All kod utom det som markerats skriven av Andr� Wallstr�m.

*/

#include "stdafx.h"
#include "StreamHandler.h"

// Standarddevice �r noll.
StreamHandler::StreamHandler() {}

// Om klassen skapas med ett nummer antas det vara ett enhetsnummer
void StreamHandler::SetDevice(int dev)
{
	device = dev;
}

// Om klassen skapas med en str�ng som argument antas det vara ett filnamn
void StreamHandler::SetFile(std::string filename)
{
	file = filename;
	useFile = true;
}

// Spawn capture thread and return webcam aspect ratio (width over height)
// Fr�n exempelkoden med modifikationer
float StreamHandler::StartCapture() {
	// Skapat av projektet. �ppna enhet eller fil baserat p�
	// konstruktorns argument
	if (useFile) {
		videoCapture.open(file);
	}
	else {
		videoCapture.open(device);
	}
	// Nedanst�ende fr�n exempekod
	if (!videoCapture.isOpened()
		|| !videoCapture.read(frame)) {
		FAIL("Could not open video source to capture first frame");
	}
	float aspectRatio = (float)frame.cols / (float)frame.rows;
	captureThread = std::thread(&StreamHandler::CaptureLoop, this);
	return aspectRatio; // Inte s� elegant att bildf�rh�llandet returneras h�r
}

// Fr�n exempelkoden
void StreamHandler::StopCapture() {
	stopped = true;
	captureThread.join();
	videoCapture.release();
}

// Fr�n exempelkoden
void StreamHandler::SetFrame(const cv::Mat& newFrame) {
	std::lock_guard<std::mutex> guard(mutex);
	frame = newFrame;
	hasFrame = true;
}

// Fr�n exempelkoden
bool StreamHandler::GetFrame(cv::Mat& out) {
	if (!hasFrame) {
		return false;
	}
	std::lock_guard<std::mutex> guard(mutex);
	out = frame;
	hasFrame = false;
	return true;
}

// Returnerar delay fr�n senaste bildrutan
int StreamHandler::GetDelay() {
	return frameDelay;
}

// Huvudloopen. Modifierad fr�n exempelkod.
void StreamHandler::CaptureLoop() {
	// H�mtade bildrutan
	cv::Mat captured;

	// Om vi l�ser fr�n en fil �r latencyn konstant. Metoden returnerar dock
	// noll om vi l�ser fr�n fil s� vi beh�ver kontrollera detta f�r att inte
	// riskera division med noll
	int fileLatency = videoCapture.get(CV_CAP_PROP_FPS);
	if (fileLatency) {
		fileLatency = 1000/fileLatency;
	}

	// Anv�nder systemtimer f�r att r�kna framerate. L�g noggrannhet
	// men tillr�ckligt f�r syftet
	double timer = GetTickCount();
	int diff;
	
	// Fr�n exempelkod
	while (!stopped) {
		videoCapture.read(captured);

		// Projektkod. Hur l�ng tid tog det att f� en ny bildruta?
		frameDelay = GetTickCount() - timer;
		timer = GetTickCount();
		
		// Fr�n exempelkod
		cv::flip(captured.clone(), captured, 0);
		SetFrame(captured);
		
		// Projektkod. Om vi l�ser fr�n en fil s� m�ste vi v�nta f�r att inte
		// fill�sningen skall g� s� snabbt som m�jligt. Eftersom fill�sningen
		// tar en viss tid men FPS-v�rdet som ber�knas �r helt korrekt f�r filen
		// kommer vi v�nta aningen f�r l�nge. Detta �r dock inte kritiskt eftersom
		// fill�sningen enbart anv�nds f�r testning och inte f�r den f�rdiga
		// prototypen i livel�ge
		if (useFile) {
			Sleep(fileLatency);
		}
	}
}