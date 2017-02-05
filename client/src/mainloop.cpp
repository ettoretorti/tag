#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <mathfu/glsl_mappings.h>
#include <enet/enet.h>
#include <flatbuffers/flatbuffers.h>

#include <tag/flatbuf/gamestate_generated.h>
#include <tag/flatbuf/inputs_generated.h>
#include <tag/flatbuf/map_generated.h>

#include <glh/Buffer.hpp>
#include <glh/VArray.hpp>
#include <glh/TextureUnit.hpp>
#include <glh/Texture1D.hpp>
#include <glh/Texture2D.hpp>
#include <glh/Shader.hpp>
#include <glh/Program.hpp>

#include <geom/Polygon.hpp>
#include <geom/Circle.hpp>

#include <iostream>
#include <vector>
#include <cmath>

namespace {

const GLchar* vShaderSrc =
	"#version 330 core\n"
	"\n"
	"uniform mat4 uMVP;\n"
	"layout(location = 0) in vec2 aPos;\n"
	"layout(location = 1) in vec4 aColor;\n"
	"\n"
	"out vec4 fColor;\n"
	"\n"
	"void main() {\n"
	"    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);\n"
	"    fColor = aColor;\n"
	"}\n";

const GLchar* fShaderSrc =
	"#version 330 core\n"
	"\n"
	"in vec4 fColor;\n"
	"\n"
	"layout(location = 0) out vec4 color;\n"
	"\n"
	"void main() {\n"
	"    color = fColor;\n"
	"}\n";

const GLchar* circleVShaderSrc =
	"#version 330 core\n"
	"\n"
	"uniform mat4 uMVP;\n"
	"layout(location = 0) in vec2 aPos;\n"
	"layout(location = 1) in vec4 aColor;\n"
	"layout(location = 2) in vec3 aScaleAndTranslate;\n"
	"\n"
	"out vec4 fColor;\n"
	"\n"
	"void main() {\n"
	"    gl_Position = uMVP * vec4(((aPos * aScaleAndTranslate.z) + aScaleAndTranslate.xy), 0.0, 1.0);\n"
	"    fColor = aColor;\n"
	"}\n";

const GLuint ATTR_POS = 0;
const GLuint ATTR_COL = 1;
const GLuint ATTR_SCLTRNS = 2;

struct CircleWColor {
	tag::flatbuf::Vec2 trans;
	float scale;
	tag::flatbuf::Color col;
};

struct PointWColor {
	tag::flatbuf::Vec2 pos;
	tag::flatbuf::Color col;
};

bool compileShader(glh::Shader& shader, const char* name) {
	if(!shader.compile()) {
		std::cerr << "Could not compile " << name << " shader" << std::endl;
		std::cerr << shader.infoLog() << std::endl;
		return false;
	}

	return true;
}

bool linkProgram(glh::Program& p, const char* name) {
	if(!p.link()) {
		std::cerr << "Could not link " << name << " program" << std::endl;
		std::cerr << p.infoLog() << std::endl;
		return false;
	}

	return true;
}

mathfu::mat4 projection(int winWidth, int winHeight, double worldSize) {
    double side = worldSize;

    if(winWidth >= winHeight) {
        double ratio = winWidth / (double) winHeight;
        return mathfu::mat4::Ortho(-side * ratio, side * ratio, -side, side, 0.1, 2.0);
    } else {
        double ratio = winHeight / (double) winWidth;
        return mathfu::mat4::Ortho(-side, side, -side * ratio, side * ratio, 0.1, 2.0);
    }
}

std::vector<CircleWColor> map2CirclesWColors(const tag::flatbuf::Map& map) {
	auto* obstacles = map.obstacles();
	int nObstacles = obstacles->size();

	std::vector<CircleWColor> toRet;

	for(int i = 0; i < nObstacles; i++) {
		auto* cur = obstacles->Get(i);
		if(cur->obstacle_type() == tag::flatbuf::Obstacle_Circle) {
			auto* circle = static_cast<const tag::flatbuf::Circle*>(cur->obstacle());
			CircleWColor cc;
			cc.trans = *circle->center();
			cc.scale = circle->radius();
			cc.col = *cur->color();
			toRet.push_back(cc);
		}
	}

	return toRet;
}

std::vector<PointWColor> map2TrianglesWColors(const tag::flatbuf::Map& map) {
	auto* obstacles = map.obstacles();
	int nObstacles = obstacles->size();

	std::vector<PointWColor> toRet;

	//two triangles for the background first
	{
		auto color = *map.background();
		auto dims = *map.dimensions();

		tag::flatbuf::Vec2 ll(-dims.x()/2, -dims.y()/2);
		tag::flatbuf::Vec2 lr( dims.x()/2, -dims.y()/2);
		tag::flatbuf::Vec2 ul(-dims.x()/2,  dims.y()/2);
		tag::flatbuf::Vec2 ur( dims.x()/2,  dims.y()/2);

		toRet.push_back({ll, color});
		toRet.push_back({ur, color});
		toRet.push_back({ul, color});

		toRet.push_back({lr, color});
		toRet.push_back({ur, color});
		toRet.push_back({ul, color});
	}

	for(int i = 0; i < nObstacles; i++) {
		auto* cur = obstacles->Get(i);
		if(cur->obstacle_type() == tag::flatbuf::Obstacle_Polygon) {
			auto* pol = static_cast<const tag::flatbuf::Polygon*>(cur->obstacle());
			auto col = *cur->color();
			auto* verts = pol->vertices();
			int nVerts = verts->size();

			for(int i = 1; i < nVerts - 1; i++) {
				toRet.push_back({*verts->Get(0),col});
				toRet.push_back({*verts->Get(i), col});
				toRet.push_back({*verts->Get(i + 1), col});
			}
		}
	}

	return toRet;
}

} // namespace

void mainloop(SDL_Window* win, ENetHost* client, ENetPeer* server) {
	using namespace std;
	using namespace glh;
	using namespace mathfu;
	using geom::Circle;
	using geom::Polygon;
	using namespace tag::flatbuf;

	Buffer circleVerts(GL_ARRAY_BUFFER);
	const int CIRCLE_SEGMENTS = 64;
	{
		Circle unitCircle(vec2(0.0, 0.0), 1.0);
		Polygon unitCircleTess(unitCircle, CIRCLE_SEGMENTS);

		VectorPacked<float, 2> vBuf[CIRCLE_SEGMENTS];

		for(int i = 0; i < CIRCLE_SEGMENTS; i++) {
			vBuf[i] = unitCircleTess.point(i);
		}

		circleVerts.data(CIRCLE_SEGMENTS * 2 * sizeof(GLfloat), vBuf, GL_STATIC_DRAW);
	}

	// Meant for instanced rendering of circles
	VArray circleVAO;
	circleVAO.enableVertexAttrib(ATTR_POS);
	circleVAO.vertexAttribPointer(circleVerts, ATTR_POS, 2, GL_FLOAT);
	glVertexAttribDivisor(ATTR_POS, 0);

	// Meant for rendering of everything else
	VArray basicVAO;

	Shader basicVShader(GL_VERTEX_SHADER);
	basicVShader.source(vShaderSrc);
	if(!compileShader(basicVShader, "basic vertex")) return;

	Shader circleVShader(GL_VERTEX_SHADER);
	circleVShader.source(circleVShaderSrc);
	if(!compileShader(circleVShader, "circle vertex")) return;

	Shader basicFShader(GL_FRAGMENT_SHADER);
	basicFShader.source(fShaderSrc);
	if(!compileShader(basicFShader, "basic fragment")) return;

	// Meant for arbitrary meshes
	Program basicProgram;
	basicProgram.setVertexShader(basicVShader);
	basicProgram.setFragmentShader(basicFShader);
	if(!linkProgram(basicProgram, "basic")) return;

	// Meant for instanced rendering of circles
	Program circleProgram;
	circleProgram.setVertexShader(circleVShader);
	circleProgram.setFragmentShader(basicFShader);
	if(!linkProgram(circleProgram, "circle")) return;

	vec2i size(0, 0);
	SDL_GL_GetDrawableSize(win, &size.x(), &size.y());

	mat4 view = mat4::LookAt(vec3(0.0), vec3(0.0, 0.0, 0.5), vec3(0.0, 1.0, 0.0), 1.0);
	mat4 proj = projection(size.x(), size.y(), 20.0);;
	mat4 mvp = proj * view;

	glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

	Buffer playerData(GL_ARRAY_BUFFER);
	circleVAO.enableVertexAttrib(ATTR_COL);
	circleVAO.vertexAttribPointer(playerData, ATTR_COL, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(CircleWColor), offsetof(CircleWColor, col));
	glVertexAttribDivisor(ATTR_COL, 1);

	circleVAO.enableVertexAttrib(ATTR_SCLTRNS);
	circleVAO.vertexAttribPointer(playerData, ATTR_SCLTRNS, 3, GL_FLOAT, GL_FALSE, sizeof(CircleWColor), 0);
	glVertexAttribDivisor(ATTR_SCLTRNS, 1);

	PlayerState stateBuf[32];
	size_t nPlayers = 0;
	size_t pIdx = ~0u;

	size_t nMapCircles = 0;
	size_t nMapPoints = 0;
	Buffer mapCircles(GL_ARRAY_BUFFER);
	Buffer mapPolys(GL_ARRAY_BUFFER);

	// Request map on a reliable channel
	flatbuffers::FlatBufferBuilder builder;
	auto req = CreateMapRequest(builder);
	builder.Finish(req);

	ENetPacket* toSend = enet_packet_create(
		builder.GetBufferPointer(),
		builder.GetSize(),
		ENET_PACKET_FLAG_RELIABLE
	);

	enet_peer_send(server, 1, toSend);
	enet_host_flush(client);

	while(true) {
		// Network events / player state update
		{
			ENetEvent e;
			const int MAX_EVENTS = 100;
			int curEvent = 0;
			while((enet_host_service(client, &e, 0) > 0) && (curEvent++ < MAX_EVENTS)) {
				switch(e.type) {
				case ENET_EVENT_TYPE_DISCONNECT: {
					cerr << "Disconnected from server!" << endl;
					return;
				}
				case ENET_EVENT_TYPE_RECEIVE: {
					if(e.channelID == 0) {
						auto snapshot = flatbuffers::GetRoot<Snapshot>(e.packet->data);
						pIdx = (size_t)snapshot->personalIdx();
						auto players = snapshot->players();
						nPlayers = players->size();

						for(size_t i = 0; i < nPlayers; i++) {
							memcpy(stateBuf + i, players->Get(i), sizeof(PlayerState));
						}

						// buffer orphaning
						playerData.data(32 * sizeof(PlayerState), nullptr, GL_STATIC_DRAW);
						playerData.subData(0, nPlayers * sizeof(PlayerState), stateBuf);
					} else {
						auto map = flatbuffers::GetRoot<Map>(e.packet->data);
						auto circles = map2CirclesWColors(*map);
						auto polys = map2TrianglesWColors(*map);

						nMapCircles = circles.size();
						nMapPoints = polys.size();

						mapCircles.data(nMapCircles * sizeof(CircleWColor), circles.data(), GL_STATIC_DRAW);
						mapPolys.data(nMapPoints * sizeof(PointWColor), polys.data(), GL_STATIC_DRAW);

						cout << "Received map" << endl;
					}

					enet_packet_destroy(e.packet);
					break;
				}
				default: break;
				}
			}
		}

		Vec2 input(0.0, 0.0);
		// Input events
		{
			SDL_Event e;
			while(SDL_PollEvent(&e)) {
				switch(e.type) {
				case SDL_QUIT: return;
				}
			}

			const Uint8* keyboard = SDL_GetKeyboardState(nullptr);
			if(keyboard[SDL_SCANCODE_D]) input.mutate_x(input.x() + 1.0);
			if(keyboard[SDL_SCANCODE_W]) input.mutate_y(input.y() + 1.0);
			if(keyboard[SDL_SCANCODE_A]) input.mutate_x(input.x() - 1.0);
			if(keyboard[SDL_SCANCODE_S]) input.mutate_y(input.y() - 1.0);

			// Normalize if necessary
			if(abs(input.x()) + abs(input.y()) > 1.0) {
				input.mutate_x(input.x() * (1.0/1.4142));
				input.mutate_y(input.y() * (1.0/1.4142));
			}
		}

		// Packet sending
		{
			flatbuffers::FlatBufferBuilder builder;
			auto vec2 = CreatePlayerInput(builder, &input);
			builder.Finish(vec2);


			ENetPacket* toSend = enet_packet_create(
						 builder.GetBufferPointer(),
						 builder.GetSize(),
						 0);

			enet_peer_send(server, 0, toSend);
			enet_host_flush(client);
		}

		// Framebuffer resizing
		{
			vec2i newSize;
			SDL_GL_GetDrawableSize(win, &newSize.x(), &newSize.y());
			if(newSize != size) {
				size = newSize;
				proj = projection(size.x(), size.y(), 20.0);;
				glViewport(0, 0, size.x(), size.y());
			}
		}

		glClear(GL_COLOR_BUFFER_BIT);

		// Update player camera to point at our player if defined
		{
			if(pIdx != ~0u) {
				vec3 pos(stateBuf[pIdx].pos().x(), stateBuf[pIdx].pos().y(), 0.0);
				view = mat4::LookAt(pos, pos + vec3(0.0, 0.0, 0.5), vec3(0.0, 1.0, 0.0), 1.0);
			}
			mvp = proj * view;
		}

		// Render map
		{
			if(nMapPoints > 0) {
				basicProgram.use();
				glUniformMatrix4fv(basicProgram.getUniform("uMVP"), 1, GL_FALSE, &mvp[0]);

				basicVAO.vertexAttribPointer(mapPolys, ATTR_POS, 2, GL_FLOAT, GL_FALSE, sizeof(PointWColor), 0);
				basicVAO.vertexAttribPointer(mapPolys, ATTR_COL, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(PointWColor), offsetof(PointWColor, col));
				basicVAO.bind();

				glDrawArrays(GL_TRIANGLES, 0, nMapPoints);
			}

			if(nMapCircles > 0) {
				circleProgram.use();
				glUniformMatrix4fv(circleProgram.getUniform("uMVP"), 1, GL_FALSE, &mvp[0]);

				circleVAO.vertexAttribPointer(mapCircles, ATTR_COL, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(CircleWColor), offsetof(CircleWColor, col));
				circleVAO.vertexAttribPointer(mapCircles, ATTR_SCLTRNS, 3, GL_FLOAT, GL_FALSE, sizeof(CircleWColor), 0);
				circleVAO.bind();

				glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, CIRCLE_SEGMENTS, nMapCircles);
			}
		}

		// Render players as circles
		{
			circleProgram.use();

			glUniformMatrix4fv(circleProgram.getUniform("uMVP"), 1, GL_FALSE, &mvp[0]);

			circleVAO.vertexAttribPointer(playerData, ATTR_COL, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(CircleWColor), offsetof(CircleWColor, col));
			circleVAO.vertexAttribPointer(playerData, ATTR_SCLTRNS, 3, GL_FLOAT, GL_FALSE, sizeof(CircleWColor), 0);
			circleVAO.bind();

			glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, CIRCLE_SEGMENTS, nPlayers);
		}

		SDL_GL_SwapWindow(win);
	}
}
