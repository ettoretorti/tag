#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <mathfu/glsl_mappings.h>

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
	"layout(location = 3) in vec4 aScaleAndTranslate;\n"
	"\n"
	"out vec4 fColor;\n"
	"\n"
	"void main() {\n"
	"    gl_Position = uMVP * vec4(((aPos * aScaleAndTranslate.xy) + aScaleAndTranslate.zw), 0.0, 1.0);\n"
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

void mainloop(SDL_Window* win) {
	using namespace std;
	using namespace glh;
	using namespace mathfu;
	using namespace geom;
	
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

	VArray circleVAO;
	circleVAO.enableVertexAttrib(0);
	circleVAO.vertexAttribPointer(circleVerts, 0, 2, GL_FLOAT);
	
	Shader basicVShader(GL_VERTEX_SHADER);
	basicVShader.source(vShaderSrc);
	if(!compileShader(basicVShader, "basic vertex")) return;

	Shader circleVShader(GL_VERTEX_SHADER);
	circleVShader.source(circleVShaderSrc);
	if(!compileShader(circleVShader, "circle vertex")) return;
	
	Shader basicFShader(GL_FRAGMENT_SHADER);
	basicFShader.source(fShaderSrc);
	if(!compileShader(basicFShader, "basic fragment")) return;
	
	Program basicProgram;
	basicProgram.setVertexShader(basicVShader);
	basicProgram.setFragmentShader(basicFShader);
	if(!linkProgram(basicProgram, "basic")) return;
	
	Program circleProgram;
	circleProgram.setVertexShader(circleVShader);
	circleProgram.setFragmentShader(basicFShader);
	if(!linkProgram(circleProgram, "circle")) return;

	vec2i size;
	SDL_GL_GetDrawableSize(win, &size.x(), &size.y());

	mat4 view = mat4::LookAt(vec3(0.0), vec3(0.0, 0.0, 0.5), vec3(0.0, 1.0, 0.0), 1.0);
	mat4 proj = mat4::Ortho(-20.0, 20.0, -20.0, 20.0, 0.1, 5.0);
	mat4 mvp = proj * view;
	
	glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);

	while(true) {
		SDL_Event e;
		while(SDL_PollEvent(&e)) {
			switch(e.type) {
			case SDL_QUIT: {
				return;
			}
			}
		}
		
		// handle framebuffer resizing
		{
			vec2i newSize;
			SDL_GL_GetDrawableSize(win, &newSize.x(), &newSize.y());
			if(newSize != size) {
				size = newSize;
				glViewport(0, 0, size.x(), size.y());
			}
		}
		
		glClear(GL_COLOR_BUFFER_BIT);
		basicProgram.use();
		glUniformMatrix4fv(basicProgram.getUniform("uMVP"), 1, GL_FALSE, &mvp[0]);
		circleVAO.bind();
		glVertexAttrib4f(1, 0.0, 1.0, 0.0, 1.0);
		glDrawArrays(GL_TRIANGLE_FAN, 0, CIRCLE_SEGMENTS);

		SDL_GL_SwapWindow(win);
	}
	
}
