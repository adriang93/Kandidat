#include "stdafx.h"
#include "NavigatorComm.h"

NavigatorComm::NavigatorComm(Compass &compass, int port) : compassModule(compass),
port(port), waitingMessage(false), newMessage("")
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
	std::lock_guard<std::mutex> guard(waitingLock);
	waitingMessage = true;
	newMessage = "LAND";
	stopped = true;
}

void NavigatorComm::Stop() {
	std::lock_guard<std::mutex> guard(waitingLock);
	waitingMessage = true;
	newMessage = "STOP";
	stopped = true;
}

void NavigatorComm::PrintLine(std::string text)
{
	std::lock_guard<std::mutex> guard(waitingLock);
	waitingMessage = true;
	newMessage = text;
}

void NavigatorComm::OutputLoop()
{
	using boost::asio::ip::tcp;
	try {
		boost::asio::io_service ioService;
		tcp::resolver::query query("localhost", std::to_string(port));
		tcp::resolver resolver(ioService);
		tcp::resolver::iterator iterator = resolver.resolve(query);
		tcp::socket socket(ioService);
		boost::asio::connect(socket, iterator);
		boost::system::error_code error;
		
		std::string message = "START";
		boost::asio::write(socket, boost::asio::buffer(message), error);
		Sleep(2000);
		while (true) {
			if (error == boost::asio::error::eof) {
				FAIL("Mission Planner stopped responding!");
			}
			if (waitingMessage) {
				std::unique_lock<std::mutex> guard(waitingLock);
				message = newMessage;
				waitingMessage = false;
				guard.unlock();
			}
			else
			{
				std::unique_lock<std::mutex> guard(outputLock);
				heading = compassModule.FilteredHeading();
				message = std::to_string(heading) + "," + std::to_string(coords.first)
					+ "," + std::to_string(coords.second) + "," + std::to_string(validCoords);
				guard.unlock();
			}
			if (stopped) { // nya meddelandet kommer alltid innehålla STOP eller LAND om vi är här.
				boost::asio::write(socket, boost::asio::buffer(newMessage), error);
				socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
				socket.close();
				break;
			}
			else {
				boost::asio::write(socket, boost::asio::buffer(message), error);
			}
			Sleep(1000 / 10);
		} 
	}
	catch (...) {
		FAIL("Could not connect to Mission Planner script");
	}
}
