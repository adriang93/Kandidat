#pragma once

#include "Coords.h"
#include "Compass.h"
#include <string>

class NavigatorComm {
public:
	NavigatorComm(Compass &);
	~NavigatorComm();
	void SetHeading(int);
	void SetCoords(std::pair<int, int>, bool);
	void PrintLine(std::string);
	void Land();
	void Stop();
private:
	int heading = 0;
	Compass &compassModule;
	std::pair<int, int> coords;
	std::thread outputThread;
	std::mutex outputLock;
	bool stopped = false;
	bool validCoords = false;
	void OutputLoop();
};