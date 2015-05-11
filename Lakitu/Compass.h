/*

Header f�r Compassmodulen. Inneh�ller inga v�rden; bara publika och privata metoder. 

*/


#pragma once

#include <OVR_CAPI.h> // Oculus-SDK:n
#include <Kernel\OVR_Math.h> // sin och tan
#include <thread> // std::thread
#include <windows.h> // For sleep
#include <iostream> // f�r att spara kompassv�rden
#include <fstream>
#include <string>
#include <vector> 

class Compass {
private:
	// Vi beh�ver h�lla reda p� tr�den som extraherar sensordata
	std::thread thread; 
	
	// Oculus Rift-objektet
	const ovrHmd* hmd;

	// Tv� olika sorters kompassriktning; den momentana och den filtrerade
	float filteredHeading, unfilteredHeading;
	float smoothing;
	void SensorLoop();
	static float Compass::CalculateHeading(const ovrVector3f&);
	static void Compass::CorrectForTilt(ovrVector3f&, const ovrVector3f&); 
public:
	Compass(const ovrHmd* hmd);
	Compass();
	~Compass();

	void Start();
	void SetHMD(ovrHmd * h);
	void SetSmoothing(int cutoffFreq);

	float Compass::FilteredHeading() const;
	float Compass::UnFilteredHeading() const;
};

