/*

Klass för kommunikation med navigationsmodulen. 

Helt skriven av projektet.

Syftet är så specifikt för implementationen att 
klassen inte gjorts generell, vilket gjort
att vissa delar av koden kunnat göras enklare.

All kod utom det som markerats skriven av André Wallström.

*/

#include "stdafx.h"
#include "NavigatorComm.h"

// Eftersom det beslutats att inte göra koden generell tillåter vi att en referns
// till kompassmodulen används som argument. 
NavigatorComm::NavigatorComm(Compass &compass, int port) : compassModule(compass), port(port) 
{
	// Starta tråden som kommunicerar
	outputThread = std::thread(&NavigatorComm::OutputLoop, this);
}

// Destruktorn avslutar tråden snyggt
NavigatorComm::~NavigatorComm()
{
	// Sätt flaggan till false för att avsluta tråden
	stopped = true;
	// Vänta till tråden avslutats
	outputThread.join();
}

// Sätt koordinaterna som ska skickas till navigationsmodulen
void NavigatorComm::SetCoords(Coords::Coord newCoord, int newDistance)
{
	coord = newCoord;
	distance = newDistance;
}

// Om ett landningsmeddelande skickas så sparas kommandot som ett väntande
// meddelande. Semafor låser kommunikationstråden om den ännu inte
// skickat i nuvarande loop så att den måste skicka meddelandet snarast möjligt
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
	// För att göra koden enklare importerar vi namesace från Boost.
	using boost::asio::ip::tcp;

	// Omslutande whileloop för att återansluta om anslutningen bryts. Om stopped har
	// satts till true så avslutas loopen.
	while (!stopped) {
		try {
			// Detta program och navigationsmodulen körs alltid på samma dator
			// Kan relativt enkelt modifieras för att tillåta att programmen
			// körs på olika datorer
			boost::asio::io_service ioService;

			// Skapa en query för adressen till localhost
			tcp::resolver::query query("localhost", std::to_string(port));
			tcp::resolver resolver(ioService);
			// Resolva adressen
			tcp::resolver::iterator iterator = resolver.resolve(query);
			// Skapa en socket till adressen
			tcp::socket socket(ioService);

			// Anslut till första träffen på queryn. Blockerar tills
			// anslutning lyckas.
			boost::asio::connect(socket, iterator);
			
			// Om vi är här har vi anslutit
			connected = true;

			// Skicka kontinuerligt meddelanden medan vi är anslutna
			while (true) {

				// Spara eventuellt felmeddelande för att sedan se om vi tappat
				// anslutningen
				boost::system::error_code error;
				std::string message; // Meddelandet som skall skickas
				// Har destruktorn satt stopped så behöver vi inte skicka meddelande utan
				// stänger anslutningen snyggt
				if (stopped) { 
					// Om programmet avslutas måste vi landa
					boost::asio::write(socket, boost::asio::buffer("LAND"));
					socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
					socket.close();
					break;
				}
				// Finns det ett väntande meddelande så skickas det
				if (waitingMessage) {
					// Vi låser här för att se till att vi inte skickar ett gammalt
					// meddelande. 
					std::unique_lock<std::mutex> guard(waitingLock);
					message = newMessage;
					waitingMessage = false;
					guard.unlock();
				}
				// Finns inget väntande meddelande skickas standardmeddelandet
				else
				{
					// Hämta kompassriktning
					heading = compassModule.FilteredHeading();
					// Skapa meddelande enligt bestämd syntax
					message = std::to_string(heading) + "," + std::to_string(coord.x)
						+ "," + std::to_string(coord.y) + "," + std::to_string(coord.valid) 
						+ "," + std::to_string(distance);
				}
				// Skicka meddelandet
				boost::asio::write(socket, boost::asio::buffer(message), error);
				// Om anslutningen avbrutits av motsatta sidan avslutar vi loopen och återgår
				// till att vänta på anslutning
				if ((error == boost::asio::error::eof) 
					|| (error == boost::asio::error::connection_reset) 
					|| (error == boost::asio::error::connection_aborted)) {
					connected = false;
					break;
				}
				// Vänta 10 ms vilket ger cirka 100 Hz
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
