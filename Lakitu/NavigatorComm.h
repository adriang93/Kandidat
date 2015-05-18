#pragma once

/*

Kommunikationsmodul för att skicka beräknade data till navigationsmodulen.

Ej generaliserad då funktionen är så specifik för vår implementation.

All kod skriven av André Wallström.

*/

#include <boost/asio.hpp>
#include "Coords.h"
#include "Compass.h"
#include <string>
#include "Common.h"

class NavigatorComm {
public:
	NavigatorComm(Compass & compass, int port);
	~NavigatorComm();

	void SetCoords(Coords::Coord newCoord, int newDistance);

	void PrintLine(std::string text);
	void Land();
	void Stop();

private:
	// Kompassriktningen sätts till 0 vid uppstart
	float heading = 0;
	
	// Porten som skall användas för kommunikation med navigationsmodulen
	int port;

	// Kompassmodulen som skall anropas för kompassvärden
	Compass &compassModule;

	// Avståndet till beräknad koordinat
	int distance;
	// Senast beräknad koordinat. Kan vara ogiltig.
	Coords::Coord coord;

	// Tråd för huvudloopen i modulen
	std::thread outputThread;

	// Semaforer för att inte skicka ogiltig data
	std::mutex outputLock;
	std::mutex waitingLock;
	
	// Hjälpvariabler för att kunna skicka godtyckliga meddelanden
	std::string newMessage = "";
	bool waitingMessage = false;

	// Är vi anslutna?
	bool connected = false;
	// Har vi stoppats?
	bool stopped = false;

	void OutputLoop();
};