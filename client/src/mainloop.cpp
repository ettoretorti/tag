#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <mathfu/glsl_mappings.h>
#include <enet/enet.h>
#include <flatbuffers/flatbuffers.h>

#include <tag/flatbuf/gamestate_generated.h>
#include <tag/flatbuf/inputs_generated.h>

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
	circleVAO.enableVertexAttrib(0);
	circleVAO.vertexAttribPointer(circleVerts, 0, 2, GL_FLOAT);
	glVertexAttribDivisor(1, 0);

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
	circleVAO.enableVertexAttrib(1);
	circleVAO.vertexAttribPointer(playerData, 1, 4, GL_UNSIGNED_BYTE, GL_TRUE, 16, 12);
	glVertexAttribDivisor(1, 1);
	
	circleVAO.enableVertexAttrib(2);
	circleVAO.vertexAttribPointer(playerData, 2, 3, GL_FLOAT, GL_FALSE, 16, 0);
	glVertexAttribDivisor(2, 1);

	PlayerState stateBuf[32];
	size_t nPlayers = 0;
	size_t pIdx = ~0u;
	
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
		}

		if(abs(input.x()) + abs(input.y()) > 1.0) {
			input.mutate_x(input.x() * 1.4142);
			input.mutate_y(input.y() * 1.4142);
		}


		flatbuffers::FlatBufferBuilder builder;
		auto vec2 = CreatePlayerInput(builder, &input);
		builder.Finish(vec2);


		ENetPacket* toSend = enet_packet_create(
					 builder.GetBufferPointer(),
					 builder.GetSize(),
					 0);

		enet_peer_send(server, 0, toSend);
		enet_host_flush(client);

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
		circleProgram.use();

		if(pIdx != ~0u) {
			vec3 pos(stateBuf[pIdx].pos().x(), stateBuf[pIdx].pos().y(), 0.0);
			view = mat4::LookAt(pos, pos + vec3(0.0, 0.0, 0.5), vec3(0.0, 1.0, 0.0), 1.0);
		}
		mvp = proj * view;
		glUniformMatrix4fv(basicProgram.getUniform("uMVP"), 1, GL_FALSE, &mvp[0]);

		circleVAO.bind();
		glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, CIRCLE_SEGMENTS, nPlayers);

		SDL_GL_SwapWindow(win);
	}
}
