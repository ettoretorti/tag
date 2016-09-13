#include <tag/flatbuf/gamestate_generated.h>

#include <enet/enet.h>
#include <flatbuffers/flatbuffers.h>

#include <iostream>
#include <string>
#include <cstdlib>
#include <csignal>
#include <cmath>
#include <chrono>

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
	using namespace flatbuffers;
	using namespace tag::flatbuf;

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
		log("Started server.");
	}
	
	PlayerState states[] = {
		PlayerState(Vec2( 0.0,  0.0), 1.0, Color(255, 255, 255, 255)),
		PlayerState(Vec2( 4.0,  4.0), 1.0, Color(255,   0,   0, 255)),
		PlayerState(Vec2( 4.0, -4.0), 1.0, Color(  0, 255,   0, 255)),
		PlayerState(Vec2(-4.0, -4.0), 1.0, Color(  0,   0, 255, 255)),
		PlayerState(Vec2(-4.0,  4.0), 1.0, Color(255, 255,   0, 255)),
	};
	int nStates = sizeof(states) / sizeof(states[0]);

	float sinv = sin(0.1 * 3.14159 / 180.0);
	float cosv = cos(0.1 * 3.14159 / 180.0);

	while(!done) {
		ENetEvent ev;
		
		const int MAX_EVENTS = 64;
		int curEvent = 0;
		while((enet_host_service(server, &ev, 10) > 0) && (curEvent++ < MAX_EVENTS)) {
			switch(ev.type) {
			case ENET_EVENT_TYPE_CONNECT:
				std::cout << "New connection!" << std::endl;
				break;
			case ENET_EVENT_TYPE_RECEIVE: {
				std::cout << "Received packet! ignoring..." << std::endl;
				enet_packet_destroy(ev.packet);
				break; }
			case ENET_EVENT_TYPE_DISCONNECT:
				std::cout << "Lost connection." << std::endl;
				break;
			}
		}

		for(int i = 0; i < nStates; i++) {
			float x = states[i].pos().x();
			float y = states[i].pos().y();

			states[i].mutable_pos().mutate_x(x * cosv - y * sinv);
			states[i].mutable_pos().mutate_y(x * sinv + y * cosv);
		}

		//Simple world
		FlatBufferBuilder builder;
		auto statesOffset = builder.CreateVectorOfStructs(states, nStates);
		auto snapshot = CreateSnapshot(builder, 0.0, statesOffset, 0);
		builder.Finish(snapshot);
		
		ENetPacket* toSend = enet_packet_create(
					 builder.GetBufferPointer(),
					 builder.GetSize(),
					 0);

		enet_host_broadcast(server, 0, toSend);

		enet_host_flush(server);
	}

	enet_host_destroy(server);
	enet_deinitialize();
	return 0;
}
