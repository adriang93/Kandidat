#pragma once

#include "stdafx.h"
#include <mutex>
#include <thread>
#include <opencv2/opencv.hpp>
#include "Common.h"

class WebcamHandler {
private:
	int device = 0;
	bool hasFrame = false;
	bool stopped = false;
	cv::VideoCapture videoCapture;
	std::thread captureThread;
	std::mutex mutex;
	cv::Mat frame;

public:
	WebcamHandler(int device);
	WebcamHandler();
	float StartCapture();
	void StopCapture();
	void SetFrame(const cv::Mat& newFrame);
	bool GetFrame(cv::Mat& out);
	void CaptureLoop();
};