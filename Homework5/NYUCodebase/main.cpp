#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <vector>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "FlareMap.h"
#include "Matrix.h"
#include "ShaderProgram.h"
//STB_image loads images
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif
#include <vector>
SDL_Window* displayWindow;

//functions
void Setup();
void Events(SDL_Event* event, bool& done);
void Update();
void Render(ShaderProgram *program);
GLuint LoadTexture(const char *filePath);
void DrawSprite(ShaderProgram *program, int index, int spriteCountX, int spriteCountY);

//classes
struct Vector3 {
	Vector3() {}
	Vector3(float x, float y, float z) :x(x), y(y), z(z) {}
	float x = 0.0;
	float y = 0.0;
	float z = 0.0;
};

class Object {
public:
	Object() {}
	Object(float x, float y, float z) : position(x, y, z) {
		scale.x = 1.0;
		scale.y = 1.0;
		scale.z = 1.0;
	}
	void Draw(ShaderProgram *program) {
		modelMatrix.Identity();
		modelMatrix.Translate(position.x, position.y, position.z);
		modelMatrix.Scale(scale.x, scale.y, scale.z);
		program->SetModelMatrix(modelMatrix);
		DrawSprite(program, spriteIndex, 16, 8);
	}
	Matrix modelMatrix;
	Vector3 position;
	Vector3 scale;
	int spriteIndex;
};

float lastFrameTicks = 0.0f;
Matrix projectionMatrix;
Matrix viewMatrix;

int main(int argc, char *argv[]) {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Collision Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	//Setup
	Setup();
	ShaderProgram program;
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	//program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	projectionMatrix.SetOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	program.SetProjectionMatrix(projectionMatrix);
	program.SetViewMatrix(viewMatrix);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(program.programID);
	//End Setup

	//Running program
	SDL_Event event;
	bool done = false;
	while (!done) {
		Events(&event, done);
		//Drawing
		Update();
		Render(&program);
		//End Drawing
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}

GLuint LoadTexture(const char *filePath) {
	int w, h, comp;
	unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);

	if (image == NULL) {
		std::cout << "Unable to load image. Make sure the path is correct\n";
		assert(false);
	}

	GLuint retTexture;
	glGenTextures(1, &retTexture);
	glBindTexture(GL_TEXTURE_2D, retTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(image);
	return retTexture;
}
void DrawSprite(ShaderProgram *program, int index, int spriteCountX, int spriteCountY) {
	float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
	float v = (float)(((int)index) / spriteCountX) / (float)spriteCountY;
	float spriteWidth = 1.0 / (float)spriteCountX;
	float spriteHeight = 1.0 / (float)spriteCountY;

	float texCoords[] = {
		u, v + spriteHeight,
		u + spriteWidth, v,
		u, v,
		u + spriteWidth, v,
		u, v + spriteHeight,
		u + spriteWidth, v + spriteHeight
	};

	float vertices[] = { -0.45f, -0.45f, 0.45f, 0.45f, -0.45f, 0.45f, 0.45f, 0.45f, -0.45f,
		-0.45f, 0.45f, -0.45f };

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}

//Main Functions
GLuint spriteSheet;
void Setup() {
	glViewport(0, 0, 640, 360);
	//spriteSheet = LoadTexture(RESOURCE_FOLDER"Images/arne_sprites.png");
	
}
void Events(SDL_Event* event, bool& done) {
	while (SDL_PollEvent(event)) {
		if (event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
	}
}
void Update() {
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	float elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;
	
}

void Render(ShaderProgram *program) {
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(255 / 255, 150 / 255, 100 / 255, 1.0f);
}
