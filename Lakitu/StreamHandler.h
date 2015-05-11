/* 

Header för streamhandlern. Taget från OculusRiftInActions exempel 13.2 med små modifikationer.

TODO: Länk till licens för exemplet

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
	//Vilken device skall vi öppna? Standard, om inget annat sätts, är device 0.
	int device = 0;
	std::string file;
	bool useFile = false;

	bool hasFrame = false;
	bool stopped = false;

	int frameDelay;

	cv::VideoCapture videoCapture;
	std::thread captureThread;
	std::mutex mutex;
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