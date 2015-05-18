#pragma once

/*

Kommunikationsmodul f�r att skicka ber�knade data till navigationsmodulen.

Ej generaliserad d� funktionen �r s� specifik f�r v�r implementation.

All kod skriven av Andr� Wallstr�m.

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
	// Kompassriktningen s�tts till 0 vid uppstart
	float heading = 0;
	
	// Porten som skall anv�ndas f�r kommunikation med navigationsmodulen
	int port;

	// Kompassmodulen som skall anropas f�r kompassv�rden
	Compass &compassModule;

	// Avst�ndet till ber�knad koordinat
	int distance;
	// Senast ber�knad koordinat. Kan vara ogiltig.
	Coords::Coord coord;

	// Tr�d f�r huvudloopen i modulen
	std::thread outputThread;

	// Semaforer f�r att inte skicka ogiltig data
	std::mutex outputLock;
	std::mutex waitingLock;
	
	// Hj�lpvariabler f�r att kunna skicka godtyckliga meddelanden
	std::string newMessage = "";
	bool waitingMessage = false;

	// �r vi anslutna?
	bool connected = false;
	// Har vi stoppats?
	bool stopped = false;

	void OutputLoop();
};