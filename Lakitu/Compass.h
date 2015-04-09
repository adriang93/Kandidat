#pragma once

#include <OVR_CAPI.h>
#include <Kernel\OVR_Math.h>
#include <thread>
#include <windows.h> // For sleep
#include <vector>

class Compass {
private:
	std::thread thread;
	const ovrHmd* hmd;
	float filteredHeading, unfilteredHeading;
	void SensorLoop();
	static float Compass::CalculateHeading(const ovrVector3f&);
	static void Compass::CorrectForTilt(ovrVector3f&, const ovrVector3f&); 
public:
	Compass(const ovrHmd* hmd);
	Compass();
	void Start();
	void SetHMD(ovrHmd * h);
	~Compass();
	float Compass::FilteredHeading();
	float Compass::UnFilteredHeading();
};

