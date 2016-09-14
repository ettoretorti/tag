#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <mathfu/glsl_mappings.h>
#include <enet/enet.h>

#include <iostream>


#ifndef NDEBUG
  #ifndef APIENTRY
    #define APIENTRY
  #endif

void APIENTRY gl_error(GLenum src, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg, const void* uParam) {
	(void)src; (void)type; (void)id; (void)severity; (void)length; (void)uParam;
	std::cerr << msg << std::endl;
}

#endif

void mainloop(SDL_Window* win, ENetHost* client, ENetPeer* server);

int main() {
	using namespace std;

	if(enet_initialize()) {
		cerr << "Failed to init enet" << endl;
		return -1;
	}

	ENetHost* client = enet_host_create(NULL, 1, 2, 0, 0);
	if(!client) {
		cerr << "Failed to init enet client" << endl;
		return -1;
	}

	ENetPeer* server = nullptr;
	{
		ENetAddress address;
		enet_address_set_host(&address, "localhost");
		address.port = 9998;

		server = enet_host_connect(client, &address, 2, 0);

		ENetEvent event;
		if(!((enet_host_service(client, &event, 1000) > 0) && event.type == ENET_EVENT_TYPE_CONNECT)) {
			enet_peer_reset(server);
			server = nullptr;
		}
	}
	if(!server) {
		cerr << "Could not connect to localhost:9998" << endl;
		return -1;
	}

	//get the server to notice us
	enet_host_flush(client);

	if(SDL_Init(SDL_INIT_EVERYTHING)) {
		cerr << "Failed to init SDL" << endl;
		cerr << SDL_GetError() << endl;
		return -1;	
	}

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	
#ifndef NDEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	SDL_Window* win = SDL_CreateWindow(
	                      "tag",
	                      SDL_WINDOWPOS_UNDEFINED,
	                      SDL_WINDOWPOS_UNDEFINED,
	                      600,
	                      600,
	                      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	if(!win) {
		cerr << "Failed to create window" << endl;
		cerr << SDL_GetError() << endl;
		return -1;
	}

	SDL_GLContext glc = SDL_GL_CreateContext(win);
	if(!glc) {
		cerr << "Failed to create OpenGL context" << endl;
		cerr << SDL_GetError() << endl;
		return -1;
	}
	
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if(err != GLEW_OK) {
		cerr << "Could not init GLEW" << endl;
		cerr << glewGetErrorString(err) << endl;
		return -1;
	}

	glEnable(GL_MULTISAMPLE);

#ifndef NDEBUG
	if (GLEW_KHR_debug) {
		glDebugMessageCallback(gl_error, nullptr);
		glEnable(GL_DEBUG_OUTPUT);
		std::cout << "Registered gl debug callback" << std::endl;
	}
#endif

	//ready
	mainloop(win, client, server);
	
	enet_peer_disconnect_now(server, 0);
	enet_host_destroy(client);
	enet_deinitialize();

	SDL_GL_DeleteContext(glc);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}
