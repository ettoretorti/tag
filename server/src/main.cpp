#include <enet/enet.h>

#include <iostream>
#include <string>
#include <cstdlib>
#include <csignal>

const int PORT = 9998;
volatile bool done = false;

static void sigCatcher(int sig) {
	std::cerr << "Caught signal. Shutting down." << std::endl;
	done = true;
}

static void log(const std::string& msg) {
	std::cerr << msg << std::endl;
}

static void log(const char* msg) {
	std::cerr << msg << std::endl;
}

static void logExit(const std::string& msg) {
	std::cerr << msg << std::endl;
	exit(-1);
}

static void logExit(const char* msg) {
	std::cerr << msg << std::endl;
	exit(-1);
}

int main(int argc, char** argv) {
	signal(SIGINT, sigCatcher);
	signal(SIGTERM, sigCatcher);

	if(enet_initialize()) {
		logExit("Could not init enet.");
	}

	ENetAddress addr;
	addr.host = ENET_HOST_ANY;
	addr.port = PORT;

	// 32 clients, 2 channels, no UP/DOWN bandwidth assumptions
	ENetHost* server = enet_host_create(&addr, 32, 2, 0, 0);

	if(!server) {
		logExit("Could not start server.");
	} else {
	}

	while(!done) {
		ENetEvent ev;
		
		const int MAX_EVENTS = 10;
		int curEvent = 0;
		while((enet_host_service(server, &ev, 10) > 0) && (curEvent++ < MAX_EVENTS)) {
			switch(ev.type) {
			case ENET_EVENT_TYPE_CONNECT:
				std::cout << "New connection!" << std::endl;
				break;
			case ENET_EVENT_TYPE_RECEIVE: {
				std::cout << "Received packet!" << std::endl;
				ENetPacket* toSend = enet_packet_create(
				                         ev.packet->data,
				                         ev.packet->dataLength,
				                         ENET_PACKET_FLAG_RELIABLE);
				enet_host_broadcast(server, 0, toSend);
				enet_packet_destroy(ev.packet);
				break; }
			case ENET_EVENT_TYPE_DISCONNECT:
				std::cout << "Lost connection" << std::endl;
				break;

			}
		}

		enet_host_flush(server);
	}

	enet_host_destroy(server);
	enet_deinitialize();
	return 0;
}
