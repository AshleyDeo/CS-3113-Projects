#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
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

SDL_Window* displayWindow;

//functions
GLuint LoadTexture(const char *filePath);

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	//Setup
	glViewport(0, 0, 640, 360);
	//untextured
	ShaderProgram program;
	program.Load(RESOURCE_FOLDER "vertex.glsl", RESOURCE_FOLDER "fragment.glsl");
	Matrix projectionMatrix;
	Matrix modelMatrix1;
	Matrix viewMatrix;

	modelMatrix1.Translate(2.0f, 0.0f, 0.0f);
	program.SetModelMatrix(modelMatrix1);

	//float aspectRatio = width / height; Multiply vv w/aspect ration to scale shape w/ screen size
	projectionMatrix.SetOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	glUseProgram(program.programID);

	//textured
	ShaderProgram program2;
	program2.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	GLuint texture1 = LoadTexture(RESOURCE_FOLDER"playerRight.png");
	GLuint texture2 = LoadTexture(RESOURCE_FOLDER"enemyUFO.png");
	GLuint texture3 = LoadTexture(RESOURCE_FOLDER"meteorBig.png");

	Matrix projectionMatrix2;
	Matrix modelMatrix2;
	Matrix viewMatrix2;

	modelMatrix2.Translate(-2.0f, 0.0f, 0.0f);
	program2.SetModelMatrix(modelMatrix2);
	float angle = 0.0;


	Matrix projectionMatrix3;
	Matrix modelMatrix3;
	Matrix viewMatrix3;
	modelMatrix3.Translate(0.0f, 1.5f, 0.0f);
	program.SetModelMatrix(modelMatrix3);

	Matrix projectionMatrix4;
	Matrix modelMatrix4;
	Matrix viewMatrix4;
	modelMatrix4.Translate(0.0f, -1.5f, 0.0f);
	program.SetModelMatrix(modelMatrix4);

	projectionMatrix2.SetOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	projectionMatrix3.SetOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	projectionMatrix4.SetOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(program2.programID);

	float lastFrameTicks = 0.0f;

	//End Setup
	//Running program
	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) { // check if window close
				done = true;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.25f, 0.3f, 0.25f, 1.0f);

		//Drawing
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		angle += elapsed*3.14*2; //multiply by mvmnt per sec
		modelMatrix3.Identity();
		modelMatrix3.Translate(0.0f, 1.5f, 0.0f);
		modelMatrix3.Rotate(angle);

		//Untextured
		program.SetModelMatrix(modelMatrix1);
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);
		float vertices1[] = { 0.5f, -0.5f, 0.0f, 0.5f, -0.5f, -0.5f };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices1);
		glEnableVertexAttribArray(program.positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(program.positionAttribute);
		
		//Textured
		program2.SetModelMatrix(modelMatrix2);
		program2.SetProjectionMatrix(projectionMatrix2);
		program2.SetViewMatrix(viewMatrix2);
		glBindTexture(GL_TEXTURE_2D, texture1);

		float vertices2[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program2.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
		glEnableVertexAttribArray(program2.positionAttribute);

		float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program2.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program2.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program2.positionAttribute);
		glDisableVertexAttribArray(program2.texCoordAttribute);	

		program2.SetModelMatrix(modelMatrix3);
		program2.SetProjectionMatrix(projectionMatrix3);
		program2.SetViewMatrix(viewMatrix3);
		glBindTexture(GL_TEXTURE_2D, texture2);

		glVertexAttribPointer(program2.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
		glEnableVertexAttribArray(program2.positionAttribute);
		
		glVertexAttribPointer(program2.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program2.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program2.positionAttribute);
		glDisableVertexAttribArray(program2.texCoordAttribute);

		program2.SetModelMatrix(modelMatrix4);
		glBindTexture(GL_TEXTURE_2D, texture3);

		glVertexAttribPointer(program2.positionAttribute, 2, GL_FLOAT, false, 0, vertices2);
		glEnableVertexAttribArray(program2.positionAttribute);

		glVertexAttribPointer(program2.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program2.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program2.positionAttribute);
		glDisableVertexAttribArray(program2.texCoordAttribute);

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
