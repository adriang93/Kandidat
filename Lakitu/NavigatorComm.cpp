#include "stdafx.h"
#include "NavigatorComm.h"

NavigatorComm::NavigatorComm(Compass &compass) : compassModule(compass)
{
	outputThread = std::thread(&NavigatorComm::OutputLoop, this);
	coords.first = 0;
	coords.second = 0;
}

NavigatorComm::~NavigatorComm()
{
	stopped = true;
	outputThread.join();
}

void NavigatorComm::SetHeading(int newHeading)
{
	heading = newHeading;
}

void NavigatorComm::SetCoords(std::pair<int, int> newCoords, bool valid)
{
	coords = newCoords;
	validCoords = valid;
}

void NavigatorComm::Land() {
	stopped = true;
	std::lock_guard<std::mutex> guard(outputLock);
	std::cout << "LAND" << std::endl;
}

void NavigatorComm::Stop() {
	stopped = true;
	std::lock_guard<std::mutex> guard(outputLock);
	std::cout << "STOP" << std::endl;
}

void NavigatorComm::PrintLine(std::string text)
{
	std::lock_guard<std::mutex> guard(outputLock);
	std::cout << text << std::endl;
}

void NavigatorComm::OutputLoop()
{
	std::cout << "START" << std::endl;
	Sleep(2000);
	while (!stopped) {
		heading = compassModule.FilteredHeading();
		std::unique_lock<std::mutex> guard(outputLock);
		std::cout << "Heading: " << heading << ", Valid: " << validCoords 
			<< ", Coords: " << coords.first << ", " << coords.second << std::endl;
		guard.unlock();
		Sleep(1000/30);
	}
}
