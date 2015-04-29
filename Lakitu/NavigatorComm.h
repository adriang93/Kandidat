#pragma once

#include <boost/asio.hpp>
#include "Coords.h"
#include "Compass.h"
#include <string>
#include "Common.h"

class NavigatorComm {
public:
	NavigatorComm(Compass & compass, int port);
	~NavigatorComm();
	void SetHeading(int heading);
	void SetCoords(std::pair<int, int> coords, bool valid);
	void PrintLine(std::string text);
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