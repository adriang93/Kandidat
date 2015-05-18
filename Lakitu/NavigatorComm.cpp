/*

Klass f�r kommunikation med navigationsmodulen. 

Helt skriven av projektet.

Syftet �r s� specifikt f�r implementationen att 
klassen inte gjorts generell, vilket gjort
att vissa delar av koden kunnat g�ras enklare.

All kod utom det som markerats skriven av Andr� Wallstr�m.

*/

#include "stdafx.h"
#include "NavigatorComm.h"

// Eftersom det beslutats att inte g�ra koden generell till�ter vi att en referns
// till kompassmodulen anv�nds som argument. 
NavigatorComm::NavigatorComm(Compass &compass, int port) : compassModule(compass), port(port) 
{
	// Starta tr�den som kommunicerar
	outputThread = std::thread(&NavigatorComm::OutputLoop, this);
}

// Destruktorn avslutar tr�den snyggt
NavigatorComm::~NavigatorComm()
{
	// S�tt flaggan till false f�r att avsluta tr�den
	stopped = true;
	// V�nta till tr�den avslutats
	outputThread.join();
}

// S�tt koordinaterna som ska skickas till navigationsmodulen
void NavigatorComm::SetCoords(Coords::Coord newCoord, int newDistance)
{
	coord = newCoord;
	distance = newDistance;
}

// Om ett landningsmeddelande skickas s� sparas kommandot som ett v�ntande
// meddelande. Semafor l�ser kommunikationstr�den om den �nnu inte
// skickat i nuvarande loop s� att den m�ste skicka meddelandet snarast m�jligt
void NavigatorComm::Land() {
	std::lock_guard<std::mutex> guard(waitingLock);
	waitingMessage = true;
	newMessage = "LAND";
}

// Samma som ovan men med annat meddelande.
void NavigatorComm::Stop() {
	std::lock_guard<std::mutex> guard(waitingLock);
	waitingMessage = true;
	newMessage = "STOP";
}

// Skicka godtyckligt meddelande som skickas som argument till funktionen
void NavigatorComm::PrintLine(std::string text)
{
	std::lock_guard<std::mutex> guard(waitingLock);
	waitingMessage = true;
	newMessage = text;
}

// Huvudloopen i programmet. Skickar kontinuerligt meddelanden till navigationsmodulen.
void NavigatorComm::OutputLoop()
{
	// F�r att g�ra koden enklare importerar vi namesace fr�n Boost.
	using boost::asio::ip::tcp;

	// Omslutande whileloop f�r att �teransluta om anslutningen bryts. Om stopped har
	// satts till true s� avslutas loopen.
	while (!stopped) {
		try {
			// Detta program och navigationsmodulen k�rs alltid p� samma dator
			// Kan relativt enkelt modifieras f�r att till�ta att programmen
			// k�rs p� olika datorer
			boost::asio::io_service ioService;

			// Skapa en query f�r adressen till localhost
			tcp::resolver::query query("localhost", std::to_string(port));
			tcp::resolver resolver(ioService);
			// Resolva adressen
			tcp::resolver::iterator iterator = resolver.resolve(query);
			// Skapa en socket till adressen
			tcp::socket socket(ioService);

			// Anslut till f�rsta tr�ffen p� queryn. Blockerar tills
			// anslutning lyckas.
			boost::asio::connect(socket, iterator);
			
			// Om vi �r h�r har vi anslutit
			connected = true;

			// Skicka kontinuerligt meddelanden medan vi �r anslutna
			while (true) {

				// Spara eventuellt felmeddelande f�r att sedan se om vi tappat
				// anslutningen
				boost::system::error_code error;
				std::string message; // Meddelandet som skall skickas
				// Har destruktorn satt stopped s� beh�ver vi inte skicka meddelande utan
				// st�nger anslutningen snyggt
				if (stopped) { 
					// Om programmet avslutas m�ste vi landa
					boost::asio::write(socket, boost::asio::buffer("LAND"));
					socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
					socket.close();
					break;
				}
				// Finns det ett v�ntande meddelande s� skickas det
				if (waitingMessage) {
					// Vi l�ser h�r f�r att se till att vi inte skickar ett gammalt
					// meddelande. 
					std::unique_lock<std::mutex> guard(waitingLock);
					message = newMessage;
					waitingMessage = false;
					guard.unlock();
				}
				// Finns inget v�ntande meddelande skickas standardmeddelandet
				else
				{
					// H�mta kompassriktning
					heading = compassModule.FilteredHeading();
					// Skapa meddelande enligt best�md syntax
					message = std::to_string(heading) + "," + std::to_string(coord.x)
						+ "," + std::to_string(coord.y) + "," + std::to_string(coord.valid) 
						+ "," + std::to_string(distance);
				}
				// Skicka meddelandet
				boost::asio::write(socket, boost::asio::buffer(message), error);
				// Om anslutningen avbrutits av motsatta sidan avslutar vi loopen och �terg�r
				// till att v�nta p� anslutning
				if ((error == boost::asio::error::eof) 
					|| (error == boost::asio::error::connection_reset) 
					|| (error == boost::asio::error::connection_aborted)) {
					connected = false;
					break;
				}
				// V�nta 10 ms vilket ger cirka 100 Hz
				Sleep(1000 / 100);
			}
		}
		catch (...) {
			connected = false;
			// FAIL("Could not connect to Mission Planner script");
		}
		Sleep(1000);
	}
}
