/* 

Hanterar strömmar från videoenheter. Taget från exempel 13.2 från OculusRiftInAction
https://github.com/OculusRiftInAction/OculusRiftInAction/
Licensierad under Apache 2.0

All kod utom det som markerats skriven av André Wallström.

*/

#include "stdafx.h"
#include "StreamHandler.h"

// Standarddevice är noll.
StreamHandler::StreamHandler() {}

// Om klassen skapas med ett nummer antas det vara ett enhetsnummer
void StreamHandler::SetDevice(int dev)
{
	device = dev;
}

// Om klassen skapas med en sträng som argument antas det vara ett filnamn
void StreamHandler::SetFile(std::string filename)
{
	file = filename;
	useFile = true;
}

// Spawn capture thread and return webcam aspect ratio (width over height)
// Från exempelkoden med modifikationer
float StreamHandler::StartCapture() {
	// Skapat av projektet. Öppna enhet eller fil baserat på
	// konstruktorns argument
	if (useFile) {
		videoCapture.open(file);
	}
	else {
		videoCapture.open(device);
	}
	// Nedanstående från exempekod
	if (!videoCapture.isOpened()
		|| !videoCapture.read(frame)) {
		FAIL("Could not open video source to capture first frame");
	}
	float aspectRatio = (float)frame.cols / (float)frame.rows;
	captureThread = std::thread(&StreamHandler::CaptureLoop, this);
	return aspectRatio; // Inte så elegant att bildförhållandet returneras här
}

// Från exempelkoden
void StreamHandler::StopCapture() {
	stopped = true;
	captureThread.join();
	videoCapture.release();
}

// Från exempelkoden
void StreamHandler::SetFrame(const cv::Mat& newFrame) {
	std::lock_guard<std::mutex> guard(mutex);
	frame = newFrame;
	hasFrame = true;
}

// Från exempelkoden
bool StreamHandler::GetFrame(cv::Mat& out) {
	if (!hasFrame) {
		return false;
	}
	std::lock_guard<std::mutex> guard(mutex);
	out = frame;
	hasFrame = false;
	return true;
}

// Returnerar delay från senaste bildrutan
int StreamHandler::GetDelay() {
	return frameDelay;
}

// Huvudloopen. Modifierad från exempelkod.
void StreamHandler::CaptureLoop() {
	// Hämtade bildrutan
	cv::Mat captured;

	// Om vi läser från en fil är latencyn konstant. Metoden returnerar dock
	// noll om vi läser från fil så vi behöver kontrollera detta för att inte
	// riskera division med noll
	int fileLatency = videoCapture.get(CV_CAP_PROP_FPS);
	if (fileLatency) {
		fileLatency = 1000/fileLatency;
	}

	// Använder systemtimer för att räkna framerate. Låg noggrannhet
	// men tillräckligt för syftet
	double timer = GetTickCount();
	int diff;
	
	// Från exempelkod
	while (!stopped) {
		videoCapture.read(captured);

		// Projektkod. Hur lång tid tog det att få en ny bildruta?
		frameDelay = GetTickCount() - timer;
		timer = GetTickCount();
		
		// Från exempelkod
		cv::flip(captured.clone(), captured, 0);
		SetFrame(captured);
		
		// Projektkod. Om vi läser från en fil så måste vi vänta för att inte
		// filläsningen skall gå så snabbt som möjligt. Eftersom filläsningen
		// tar en viss tid men FPS-värdet som beräknas är helt korrekt för filen
		// kommer vi vänta aningen för länge. Detta är dock inte kritiskt eftersom
		// filläsningen enbart används för testning och inte för den färdiga
		// prototypen i liveläge
		if (useFile) {
			Sleep(fileLatency);
		}
	}
}