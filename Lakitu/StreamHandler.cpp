/* 
Hanterar str�mmar fr�n videoenheter. Taget fr�n exempel 13.2 fr�n OculusRiftInAction

TODO: Licens f�r ovanst�ende.

*/

#include "stdafx.h"
#include "StreamHandler.h"

// Initera med ett best�md devicenummer
StreamHandler::StreamHandler(int device) : device(device) {
}

// Standarddevice �r noll.
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
	
	// Koden returnerar aspect ratio f�r bildstr�mmen. Inte s� snyggt. 
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
		Sleep(1000);
	}
}