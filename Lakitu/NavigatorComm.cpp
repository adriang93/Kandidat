#include "stdafx.h"
#include "NavigatorComm.h"

NavigatorComm::NavigatorComm(Compass &compass, int port) : compassModule(compass),
port(port), waitingMessage(false), newMessage(""), connected(false)
{
	coords.first = 0;
	coords.second = 0;
	connectMP();
}

void NavigatorComm::connectMP() {
	if (!connected) {
		if (outputThread.joinable()) {
			outputThread.join();
		}
		outputThread = std::thread(&NavigatorComm::OutputLoop, this);
		stopped = false;
	}
}

NavigatorComm::~NavigatorComm()
{
	stopped = true;
	outputThread.join();
}

void NavigatorComm::SetHeading(float newHeading)
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
}

void NavigatorComm::Stop() {
	std::lock_guard<std::mutex> guard(waitingLock);
	waitingMessage = true;
	newMessage = "STOP";
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
	while (true) {
		try {
			boost::asio::io_service ioService;
			tcp::resolver::query query("localhost", std::to_string(port));
			tcp::resolver resolver(ioService);
			tcp::resolver::iterator iterator = resolver.resolve(query);
			tcp::socket socket(ioService);
			boost::asio::connect(socket, iterator);
			connected = true;
			std::string message = "START\n";
			boost::asio::write(socket, boost::asio::buffer(message));
			while (true) {
				boost::system::error_code error;
				if (stopped) {
					socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
					socket.close();
					break;
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
				boost::asio::write(socket, boost::asio::buffer(message), error);
				if ((error == boost::asio::error::eof) || (error == boost::asio::error::connection_reset) || (error == boost::asio::error::connection_aborted)) {
					connected = false;
					break;
				}
				Sleep(1000 / 10);
			}
		}
		catch (...) {
			connected = false;
			// FAIL("Could not connect to Mission Planner script");
		}
		if (stopped) {
			break;
		}
		Sleep(1000);
	}
}
