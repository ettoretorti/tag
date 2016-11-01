#include <tag/flatbuf/gamestate_generated.h>
#include <tag/flatbuf/inputs_generated.h>

#include <enet/enet.h>
#include <flatbuffers/flatbuffers.h>
#include <mathfu/vector.h>

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

static mathfu::Vector<float, 2> convert(const tag::flatbuf::Vec2& v) {
	return mathfu::Vector<float, 2>(v.x(), v.y());
}

static tag::flatbuf::Vec2 convert(const mathfu::Vector<float, 2>& v) {
	return tag::flatbuf::Vec2(v.x(), v.y());
}

static const tag::flatbuf::Color COLORS[] = {
	tag::flatbuf::Color(255,   0,   0, 255),
	tag::flatbuf::Color(255, 255,   0, 255),
	tag::flatbuf::Color(  0, 255,   0, 255),
	tag::flatbuf::Color(  0, 255, 255, 255),
	tag::flatbuf::Color(  0,   0, 255, 255),
	tag::flatbuf::Color(255,   0, 255, 255),
};

static const size_t N_COLORS = sizeof(COLORS)/sizeof(COLORS[0]);

int main(int argc, char** argv) {
	using namespace flatbuffers;
	using namespace tag::flatbuf;
	using namespace std::chrono;

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

	PlayerState players[32];
	mathfu::Vector<float, 2> inputs[32];
	ENetPeer* peers[32];
	size_t nPlayers = 0;


	const size_t TICKS_PER_SEC = 60;
	const auto TIMESTEP = duration<int, std::ratio<1,TICKS_PER_SEC>>(1);
	const double TIMESTEP_F = 1.0 / TICKS_PER_SEC;

	auto nextUpdate = high_resolution_clock::now() + TIMESTEP;

	while(!done) {
		ENetEvent ev;

		const int MAX_EVENTS = 64;
		int curEvent = 0;
		while((enet_host_service(server, &ev, 1) > 0) && (curEvent++ < MAX_EVENTS)) {
			switch(ev.type) {
			case ENET_EVENT_TYPE_CONNECT:
				std::cout << "New connection!" << std::endl;
				std::cout << "Assigning to slot: " << nPlayers << std::endl;
				peers[nPlayers] = ev.peer;
				ev.peer->data = (void*)nPlayers;

				players[nPlayers] = PlayerState(Vec2(0.0, 0.0), 1.0, COLORS[nPlayers % N_COLORS]);
				inputs[nPlayers] = mathfu::Vector<float, 2>(0.0, 0.0);
				nPlayers++;
				break;
			case ENET_EVENT_TYPE_RECEIVE: {
				size_t playerNo = (size_t)ev.peer->data;

				Vec2 input = *flatbuffers::GetRoot<PlayerInput>(ev.packet->data)->movement();
				inputs[playerNo].x() = input.x();
				inputs[playerNo].y() = input.y();

				enet_packet_destroy(ev.packet);
				break; }
			case ENET_EVENT_TYPE_DISCONNECT:
				std::cout << "Lost connection." << std::endl;

				size_t playerNo = (size_t)ev.peer->data;
				std::cout << "Player was in slot: " << playerNo << std::endl;

				std::swap(players[playerNo], players[nPlayers-1]);
				std::swap(peers[playerNo], peers[nPlayers-1]);
				nPlayers--;


				break;
			}
		}

		if(high_resolution_clock::now() >= nextUpdate) {
			nextUpdate += TIMESTEP;

			for(size_t i = 0; i < nPlayers; i++) {
				if(inputs[i].LengthSquared() > 1.0) {
					inputs[i].Normalize();
				}

				Vec2& pos = players[i].mutable_pos();

				pos.mutate_x(pos.x() + TIMESTEP_F * 5.0 * inputs[i].x());
				pos.mutate_y(pos.y() + TIMESTEP_F * 5.0 * inputs[i].y());
			}

			const size_t maxIterations = 20;
			bool noCollisions = false;

			for(size_t i = 0; i < maxIterations && !noCollisions; i++) {
				noCollisions = true;
				for(size_t j = 0; j < nPlayers; j++) {
					for(size_t k = j + 1; k < nPlayers; k++) {
						mathfu::Vector<float, 2> pos1 = convert(players[j].pos());
						mathfu::Vector<float, 2> pos2 = convert(players[k].pos());

						float rad1 = players[j].radius();
						float rad2 = players[k].radius();

						float dSqrd = (rad1 + rad2) * (rad1 + rad2);

						if((pos1 - pos2).LengthSquared() < dSqrd) {
							noCollisions = false;

							auto disp = pos1 - pos2;
							float len = disp.Length();

							if(len <= 0.001) {
								disp.x() = 0.5; disp.y() = 0.0;
							} else {
								float missing = rad1 + rad2 - len;
								disp = (missing * disp / 2.0 * len);
							}

							pos1 += disp;
							pos2 -= disp;

							players[j].mutable_pos() = convert(pos1);
							players[k].mutable_pos() = convert(pos2);
						}
					}
				}
			}
		}

		//Simple world
		FlatBufferBuilder builder;
		auto statesOffset = builder.CreateVectorOfStructs(players, nPlayers);
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
