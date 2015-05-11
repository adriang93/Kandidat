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
	void SetHeading(float newHeading);
	void SetCoords(Coords::Coord newCoord, int newDistance);

	void PrintLine(std::string text);
	void Land();
	void Stop();

private:
	float heading = 0;
	int port;

	Compass &compassModule;

	int distance;
	Coords::Coord coord;

	std::thread outputThread;
	std::mutex outputLock;
	std::mutex waitingLock;
	
	std::string newMessage;
	bool waitingMessage;

	bool connected;
	bool stopped = false;

	void OutputLoop();
};