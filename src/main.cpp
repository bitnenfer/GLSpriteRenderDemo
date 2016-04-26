#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

// Comment this to render with naive approach.
#define USE_DYNAMIC_STREAMING

#if _WIN32
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE inline
#endif

#define SPRITE_COUNT 1036800

#pragma region Shader Source
const char* FAST_SPRITE_VSHADER =
"#version 120\n\n"
"attribute vec2 inVertexPos;"
"attribute vec4 inVertexCol;"
"varying vec4 outVertexCol;"
"uniform mat4 orthoView;"
"void main() {"
"	gl_Position = orthoView * vec4(inVertexPos, 1.0, 1.0);"
"	outVertexCol = inVertexCol;"
"}\n"
;
const char* FAST_SPRITE_FSHADER =
"#version 120\n\n"
"varying vec4 outVertexCol;"
"void main() {"
"	gl_FragColor = outVertexCol;"
"}\n"
;
#pragma endregion
#pragma region Shader Setup Functions
static void ogl_errorinfo(GLuint Object, PFNGLGETSHADERIVPROC Getiv, PFNGLGETSHADERINFOLOGPROC GetInfoLog)
{
	GLint info_length = 0;
	char *log = nullptr;
	Getiv(Object, GL_INFO_LOG_LENGTH, &info_length);
	log = new char[info_length + 1];
	GetInfoLog(Object, info_length, nullptr, log);
	log[info_length] = 0;
	fprintf(stderr, "Shader Error: %s", log);
	delete[] log;
}

GLuint compileShader(GLuint program, const char* source, GLenum type)
{
	GLint OK;
	GLuint glShader = glCreateShader(type);
	glShaderSource(glShader, 1, &source, nullptr);
	glCompileShader(glShader);
	glGetShaderiv(glShader, GL_COMPILE_STATUS, &OK);
	if (!OK)
	{
		ogl_errorinfo(glShader, glGetShaderiv, glGetShaderInfoLog);
		glDeleteShader(glShader);
		getchar();
		exit(-1);
	}
	glAttachShader(program, glShader);
	return glShader;
}
#pragma endregion
#pragma region Global Data
static SDL_Window* pSDLWindow;
static SDL_GLContext sdlOpenGLContext;
static const size_t appWidth = 1920;
static const size_t appHeight = 1080;
static const size_t appHalfWidth = appWidth / 2;
static const size_t appHalfHeight = appHeight / 2;
static const size_t vertPerQuad = 6;
static const size_t maxVertices = SPRITE_COUNT * vertPerQuad;
static float colorR = 1.0f;
static float colorG = 1.0f;
static float colorB = 1.0f;
static float colorA = 1.0f;
static size_t bufferDataIndex = 0;
static float* pVertexPosBufferData = nullptr;
static float* pVertexPosCurrent = nullptr;
static float* pVertexColBufferData = nullptr;
static float* pVertexColCurrent = nullptr;
static GLuint shaderProgram;
static GLuint vertexShader;
static GLuint fragmentShader;
static GLuint vertexPosVBO;
static GLuint vertexColVBO;
static GLuint locVertexPos;
static GLuint locVertexCol;
static GLbitfield allocFlag;
static size_t vpSize;
static size_t vcSize;
#pragma endregion
#pragma region Drawing API
FORCEINLINE void flushBufferData0()
{
	glBindBuffer(GL_ARRAY_BUFFER, vertexPosVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * bufferDataIndex * vertPerQuad * 2, pVertexPosBufferData);
	glBindBuffer(GL_ARRAY_BUFFER, vertexColVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * bufferDataIndex * vertPerQuad * 4, pVertexColBufferData);
	glDrawArrays(GL_TRIANGLES, 0, bufferDataIndex * vertPerQuad);
	bufferDataIndex = 0;
	pVertexPosCurrent = pVertexPosBufferData;
	pVertexColCurrent = pVertexColBufferData;
}

FORCEINLINE void flushBufferData1()
{
	glDrawArrays(GL_TRIANGLES, 0, bufferDataIndex * vertPerQuad);
	bufferDataIndex = 0;
	pVertexPosCurrent = pVertexPosBufferData;
	pVertexColCurrent = pVertexColBufferData;
}

FORCEINLINE void flush()
{
	#if defined(USE_DYNAMIC_STREAMING)
	flushBufferData1();
	#else
	flushBufferData0();
	#endif
}

FORCEINLINE void setColor(float r, float g, float b, float a = 1.0f)
{
	colorR = r;
	colorG = g;
	colorB = b;
	colorA = a;
}

FORCEINLINE void drawRect(float x, float y, float width, float height)
{
	// first triangle
	pVertexPosCurrent[0] = x;
	pVertexPosCurrent[1] = y;
	pVertexPosCurrent[2] = x + width;
	pVertexPosCurrent[3] = y + height;
	pVertexPosCurrent[4] = x;
	pVertexPosCurrent[5] = y + height;
	// second triangle
	pVertexPosCurrent[6] = x;
	pVertexPosCurrent[7] = y;
	pVertexPosCurrent[8] = x + width;
	pVertexPosCurrent[9] = y;
	pVertexPosCurrent[10] = x + width;
	pVertexPosCurrent[11] = y + height;

	// first triangle
	pVertexColCurrent[0] = colorR;
	pVertexColCurrent[1] = colorG;
	pVertexColCurrent[2] = colorB;
	pVertexColCurrent[3] = colorA;
	pVertexColCurrent[4] = colorR;
	pVertexColCurrent[5] = colorG;
	pVertexColCurrent[6] = colorB;
	pVertexColCurrent[7] = colorA;
	pVertexColCurrent[8] = colorR;
	pVertexColCurrent[9] = colorG;
	pVertexColCurrent[10] = colorB;
	pVertexColCurrent[11] = colorA;

	// second triangle
	pVertexColCurrent[12] = colorR;
	pVertexColCurrent[13] = colorG;
	pVertexColCurrent[14] = colorB;
	pVertexColCurrent[15] = colorA;
	pVertexColCurrent[16] = colorR;
	pVertexColCurrent[17] = colorG;
	pVertexColCurrent[18] = colorB;
	pVertexColCurrent[19] = colorA;
	pVertexColCurrent[20] = colorR;
	pVertexColCurrent[21] = colorG;
	pVertexColCurrent[22] = colorB;
	pVertexColCurrent[23] = colorA;	

	pVertexPosCurrent = (float*)((char*)pVertexPosCurrent + (sizeof(float) * 12));
	pVertexColCurrent = (float*)((char*)pVertexColCurrent + (sizeof(float) * 24));

	++bufferDataIndex;
}
#pragma endregion
#pragma region SDL and OpenGL Setup
FORCEINLINE void initOpenGLContext()
{
	assert(!(SDL_Init(SDL_INIT_EVERYTHING) < 0));
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	pSDLWindow = SDL_CreateWindow(
		"Dynamic Streaming Demo",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		appWidth, appHeight,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
	);

	assert(pSDLWindow != nullptr);
	sdlOpenGLContext = SDL_GL_CreateContext(pSDLWindow);
	assert(sdlOpenGLContext != nullptr);
	assert(glewInit() == GLEW_OK);
	SDL_GL_MakeCurrent(pSDLWindow, sdlOpenGLContext);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, appWidth, appHeight);
	SDL_GL_SetSwapInterval(1);
}
#if defined(LD_RELEASE)
#define main SDL_main
#endif
#pragma endregion

int main(int argc, char *args[])
{
	#pragma region Setting up SDL and OpenGL Context
	bool run = true;
	SDL_Event event;
	initOpenGLContext();
	#pragma endregion

	unsigned int factor = 0;
	float ortho2D[16] = {
		2.0f / appWidth, 0, 0, 0,
		0, -2.0f / appHeight, 0, 0,
		0, 0, 1.0f, 1.0f,
		-1.0f, 1.0f, 0, 0
	};

	// Setup Shader
	shaderProgram = glCreateProgram();
	vertexShader = compileShader(shaderProgram, FAST_SPRITE_VSHADER, GL_VERTEX_SHADER);
	fragmentShader = compileShader(shaderProgram, FAST_SPRITE_FSHADER, GL_FRAGMENT_SHADER);
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);
	locVertexPos = glGetAttribLocation(shaderProgram, "inVertexPos");
	locVertexCol = glGetAttribLocation(shaderProgram, "inVertexCol");

	// Setup 2D orthographic matrix view
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "orthoView"), 1, GL_FALSE, ortho2D);

	// Generate and Allocate Buffers
	glGenBuffers(1, &vertexPosVBO);
	glGenBuffers(1, &vertexColVBO);

	vpSize = SPRITE_COUNT * (sizeof(float) * 12);
	vcSize = SPRITE_COUNT * (sizeof(float) * 24);

	#pragma region Buffer Allocation
	#if defined(USE_DYNAMIC_STREAMING)
	GLbitfield mapFlags =
		GL_MAP_WRITE_BIT |
		GL_MAP_PERSISTENT_BIT |
		GL_MAP_COHERENT_BIT;
	GLbitfield createFlags = mapFlags | GL_DYNAMIC_STORAGE_BIT;	

	glBindBuffer(GL_ARRAY_BUFFER, vertexPosVBO);
	glBufferStorage(GL_ARRAY_BUFFER, vpSize, nullptr, createFlags);
	glEnableVertexAttribArray(locVertexPos);
	glVertexAttribPointer(locVertexPos, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	pVertexPosBufferData = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, vpSize, mapFlags);
	pVertexPosCurrent = pVertexPosBufferData;

	glBindBuffer(GL_ARRAY_BUFFER, vertexColVBO);
	glBufferStorage(GL_ARRAY_BUFFER, vcSize, nullptr, createFlags);
	glEnableVertexAttribArray(locVertexCol);
	glVertexAttribPointer(locVertexCol, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	pVertexColBufferData = (float*)glMapBufferRange(GL_ARRAY_BUFFER, 0, vcSize, mapFlags);
	pVertexColCurrent = pVertexColBufferData;

	#else
	pVertexPosBufferData = (float*)malloc(vpSize);
	pVertexColBufferData = (float*)malloc(vcSize);
	pVertexPosCurrent = pVertexPosBufferData;
	pVertexColCurrent = pVertexColBufferData;

	glBindBuffer(GL_ARRAY_BUFFER, vertexPosVBO);
	glBufferData(GL_ARRAY_BUFFER, vpSize, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(locVertexPos);
	glVertexAttribPointer(locVertexPos, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, vertexColVBO);
	glBufferData(GL_ARRAY_BUFFER, vcSize, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(locVertexCol);
	glVertexAttribPointer(locVertexCol, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	#endif
	#pragma endregion

	glClearColor(0, 0, 0, 1);
	while (run)
	{
		#pragma region SDL Event Polling
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
				run = false;
			else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
				run = false;
		}
		#pragma endregion
		glClear(GL_COLOR_BUFFER_BIT);
		factor++;
		for (size_t index = 0; index < SPRITE_COUNT; ++index)
		{
			setColor(((index + factor) % 255) / 255.0f, ((index + factor) % 250) / 255.0f, ((index + factor) % 200) / 255.0f);
			drawRect(index % appHalfWidth * 2, index / appHalfWidth * 2, 2, 2);
		}
		flush();
		#pragma region Swap Buffers
		SDL_GL_SwapWindow(pSDLWindow);
		#pragma endregion
	}
	// Housekeeping.
	#ifndef USE_DYNAMIC_STREAMING
	free(pVertexPosBufferData);
	free(pVertexColBufferData);
	#endif
	glDeleteBuffers(1, &vertexPosVBO);
	glDeleteBuffers(1, &vertexColVBO);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glDeleteProgram(shaderProgram);
	return 0;
}