/* 

Header f�r streamhandlern. Taget fr�n OculusRiftInActions exempel 13.2 med sm� modifikationer.

TODO: L�nk till licens f�r exemplet

*/

#pragma once

#include "stdafx.h"
#include <mutex> // semafor
#include <thread> // std::thread
#include <opencv2/opencv.hpp> 
#include "Common.h" // Resurser fr�n OculusRiftInAction

class StreamHandler {
private:
	//Vilken device skall vi �ppna? Standard, om inget annat s�tts, �r device 0.
	int device = 0;
	bool hasFrame = false;
	bool stopped = false;
	cv::VideoCapture videoCapture;
	std::thread captureThread;
	std::mutex mutex;
	cv::Mat frame;

public:
	StreamHandler(int device);
	StreamHandler();
	float StartCapture();
	void StopCapture();
	void SetFrame(const cv::Mat& newFrame);
	bool GetFrame(cv::Mat& out);
	void CaptureLoop();
};