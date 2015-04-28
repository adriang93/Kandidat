#pragma once

#include <boost/asio.hpp>
#include "Coords.h"
#include "Compass.h"
#include <string>
#include "Common.h"

class NavigatorComm {
public:
	NavigatorComm(Compass &, int);
	~NavigatorComm();
	void SetHeading(int);
	void SetCoords(std::pair<int, int>, bool);
	void PrintLine(std::string);
	void Land();
	void Stop();
private:
	float heading = 0;
	Compass &compassModule;
	std::pair<int, int> coords;
	std::thread outputThread;
	std::mutex outputLock;
	std::mutex waitingLock;
	int port;
	std::string newMessage;
	bool waitingMessage;
	bool stopped = false;
	bool validCoords = false;
	void OutputLoop();
};