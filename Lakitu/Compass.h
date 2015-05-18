/*

Header för Compassmodulen. Innehåller inga värden; bara publika och privata metoder. 

All kod skriven av André Wallström.

*/

#pragma once

#include <OVR_CAPI.h> // Oculus-SDK:n
#include <Kernel\OVR_Math.h> // sin och tan
#include <thread> // std::thread
#include <windows.h> // For sleep
#include <iostream> // för att spara kompassvärden
#include <fstream>
#include <string>
#include <vector> 

class Compass {
private:
	// Vi behöver hålla reda på tråden som extraherar sensordata
	std::thread thread; 
	
	// Oculus Rift-objektet
	const ovrHmd* hmd;

	// Två olika sorters kompassriktning, den momentana och den filtrerade, 
	// finns tillgängliga
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

