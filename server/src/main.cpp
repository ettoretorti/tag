#include <tag/flatbuf/gamestate_generated.h>
#include <tag/flatbuf/inputs_generated.h>

#include <enet/enet.h>
#include <flatbuffers/flatbuffers.h>
#include <mathfu/vector.h>
#include <Box2D/Box2D.h>

#include <iostream>
#include <string>
#include <cstdlib>
#include <csignal>
#include <cmath>
#include <chrono>

const int PORT = 9998;
volatile bool done = false;

static void sigCatcher(int sig) {
	(void)sig;
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

	(void)argc; (void)argv;

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

	const double WIDTH = 40.0;
	const double HEIGHT = 40.0;

	b2World world(b2Vec2(0.0f, 0.0f));
	b2Body* outerPerim = nullptr;
	{
		b2ChainShape chain;
		b2Vec2 vs[5] = {
			b2Vec2(-WIDTH/2, -HEIGHT/2), b2Vec2( WIDTH/2, -HEIGHT/2),
			b2Vec2( WIDTH/2,  HEIGHT/2), b2Vec2(-WIDTH/2,  HEIGHT/2),
			b2Vec2(-WIDTH/2, -HEIGHT/2)
		};
		chain.CreateChain(vs, 5);
		chain.SetPrevVertex(vs[4]);
		chain.SetNextVertex(vs[0]);

		b2BodyDef def;
		def.position.Set(0.0f, 0.0f);

		outerPerim = world.CreateBody(&def);
		outerPerim->CreateFixture(&chain, 0.0);
	}

	struct {
		mathfu::Vector<float, 2> input;
		ENetPeer* peer;
		b2Body* body;
		Color color;
	} pInfo[32];
	size_t nPlayers = 0;
	size_t nextColor = 0;


	const size_t TICKS_PER_SEC = 60;
	const auto TIMESTEP = duration<int, std::ratio<1,TICKS_PER_SEC>>(1);
	const double TIMESTEP_F = 1.0 / TICKS_PER_SEC;

	auto startTime = high_resolution_clock::now();
	auto nextUpdate = startTime + TIMESTEP;

	while(!done) {
		ENetEvent ev;

		const int MAX_EVENTS = 64;
		int curEvent = 0;
		while((enet_host_service(server, &ev, 1) > 0) && (curEvent++ < MAX_EVENTS)) {
			switch(ev.type) {
			case ENET_EVENT_TYPE_CONNECT: {
				std::cout << "New connection!" << std::endl;
				std::cout << "Assigning to slot: " << nPlayers << std::endl;
				pInfo[nPlayers].peer = ev.peer;
				ev.peer->data = (void*)nPlayers;

				b2BodyDef def;
				def.position.Set(0.0, 0.0);
				def.type = b2_dynamicBody;
				b2CircleShape unitCircle;
				unitCircle.m_p.Set(0, 0);
				unitCircle.m_radius = 1.0;
				pInfo[nPlayers].body = world.CreateBody(&def);
				pInfo[nPlayers].body->CreateFixture(&unitCircle, 1.0);

				pInfo[nPlayers].input = mathfu::Vector<float, 2>(0.0, 0.0);

				pInfo[nPlayers].color = COLORS[nextColor++ % N_COLORS];

				nPlayers++;
				break; }
			case ENET_EVENT_TYPE_RECEIVE: {
				size_t playerNo = (size_t)ev.peer->data;

				Vec2 input = *flatbuffers::GetRoot<PlayerInput>(ev.packet->data)->movement();
				pInfo[playerNo].input.x() = input.x();
				pInfo[playerNo].input.y() = input.y();

				enet_packet_destroy(ev.packet);
				break; }
			case ENET_EVENT_TYPE_DISCONNECT: {
				std::cout << "Lost connection." << std::endl;

				size_t playerNo = (size_t)ev.peer->data;
				std::cout << "Player was in slot: " << playerNo << std::endl;

				world.DestroyBody(pInfo[playerNo].body);
				pInfo[playerNo] = pInfo[nPlayers-1];
				nPlayers--;


				break; }
			default: break;
			}
		}

		if(high_resolution_clock::now() >= nextUpdate) {
			nextUpdate += TIMESTEP;

			for(size_t i = 0; i < nPlayers; i++) {
				if(pInfo[i].input.LengthSquared() > 1.0) {
					pInfo[i].input.Normalize();
				}

				b2Vec2 bb(5 * pInfo[i].input.x(), 5 * pInfo[i].input.y());
				pInfo[i].body->SetLinearVelocity(bb);
			}

			world.Step(TIMESTEP_F, 10, 10);
		}

		//Simple world
		FlatBufferBuilder builder;
		double curTime = duration<double>(high_resolution_clock::now() - startTime).count();

		PlayerState players[32];
		for(size_t i = 0; i < nPlayers; i++) {
			b2Vec2 pos = pInfo[i].body->GetPosition();
			players[i] = PlayerState(Vec2(pos.x, pos.y), 1.0, pInfo[i].color);
		}

		for(size_t i = 0; i < nPlayers; i++) {
			builder.Clear();
			auto statesOffset = builder.CreateVectorOfStructs(players, nPlayers);
			auto snapshot = CreateSnapshot(builder, curTime, statesOffset, i);
			builder.Finish(snapshot);
			ENetPacket* toSend = enet_packet_create(
			                         builder.GetBufferPointer(),
						 builder.GetSize(),
						 0);

			enet_peer_send(pInfo[i].peer, 0, toSend);
		}

		enet_host_flush(server);
	}

	enet_host_destroy(server);
	enet_deinitialize();
	return 0;
}
