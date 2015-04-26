/* 
Hanterar strömmar från videoenheter. Taget från exempel 13.2 från OculusRiftInAction

TODO: Licens för ovanstående.

*/

#include "stdafx.h"
#include "StreamHandler.h"

// Initera med ett bestämd devicenummer
StreamHandler::StreamHandler(int device) : device(device) {
}

// Standarddevice är noll.
StreamHandler::StreamHandler() {}

// Spawn capture thread and return webcam aspect ratio (width over height)
float StreamHandler::StartCapture() {
	videoCapture.open("gul.mp4");
	if (!videoCapture.isOpened()
		|| !videoCapture.read(frame)) {
		FAIL("Could not open video source to capture first frame");
	}
	float aspectRatio = (float)frame.cols / (float)frame.rows;
	captureThread = std::thread(&StreamHandler::CaptureLoop, this);
	
	// Koden returnerar aspect ratio för bildströmmen. Inte så snyggt. 
	return aspectRatio;
}

void StreamHandler::StopCapture() {
	stopped = true;
	captureThread.join();
	videoCapture.release();
}

void StreamHandler::SetFrame(const cv::Mat& newFrame) {
	std::lock_guard<std::mutex> guard(mutex);
	frame = newFrame;
	hasFrame = true;
}

bool StreamHandler::GetFrame(cv::Mat& out) {
	if (!hasFrame) {
		return false;
	}
	std::lock_guard<std::mutex> guard(mutex);
	out = frame;
	hasFrame = false;
	return true;
}

void StreamHandler::CaptureLoop() {
	cv::Mat captured;
	while (!stopped) {
		videoCapture.read(captured);
		cv::flip(captured.clone(), captured, 0);
		SetFrame(captured);
		Sleep(1000/30);
	}
}