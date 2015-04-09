#include "stdafx.h"
#include "WebcamHandler.h"


WebcamHandler::WebcamHandler(int device) : device(device) {
}

WebcamHandler::WebcamHandler() : device(0) {
}

// Spawn capture thread and return webcam aspect ratio (width over height)
float WebcamHandler::StartCapture() {
	videoCapture.open(device);
	if (!videoCapture.isOpened()
		|| !videoCapture.read(frame)) {
		FAIL("Could not open video source to capture first frame");
	}
	float aspectRatio = (float)frame.cols / (float)frame.rows;
	captureThread = std::thread(&WebcamHandler::CaptureLoop, this);
	return aspectRatio;
}

void WebcamHandler::StopCapture() {
	stopped = true;
	captureThread.join();
	videoCapture.release();
}

void WebcamHandler::SetFrame(const cv::Mat& newFrame) {
	std::lock_guard<std::mutex> guard(mutex);
	frame = newFrame;
	hasFrame = true;
}

bool WebcamHandler::GetFrame(cv::Mat& out) {
	if (!hasFrame) {
		return false;
	}
	std::lock_guard<std::mutex> guard(mutex);
	out = frame;
	hasFrame = false;
	return true;
}

void WebcamHandler::CaptureLoop() {
	cv::Mat captured;
	while (!stopped) {
		videoCapture.read(captured);
		cv::flip(captured.clone(), captured, 0);
		SetFrame(captured);
	}
}