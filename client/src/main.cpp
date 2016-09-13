#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <mathfu/glsl_mappings.h>

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

void mainloop(SDL_Window* win);

int main() {
	using namespace std;

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
	
#ifndef NDEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	SDL_Window* win = SDL_CreateWindow(
	                      "shadows",
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

#ifndef NDEBUG
	if (GLEW_KHR_debug) {
		glDebugMessageCallback(gl_error, nullptr);
		glEnable(GL_DEBUG_OUTPUT);
		std::cout << "Registered gl debug callback" << std::endl;
	}
#endif

	//ready
	mainloop(win);

	SDL_GL_DeleteContext(glc);
	SDL_DestroyWindow(win);
	SDL_Quit();
	return 0;
}
