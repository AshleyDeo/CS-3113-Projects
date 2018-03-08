#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <vector>
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
void DrawText(ShaderProgram *program, int fontTexture, std::string text, float size, float spacing);
//functions
GLuint LoadTexture(const char *filePath);
class Player {
public:
	Player(float pos_x) : pos_x(pos_x) {}
	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	void Draw() {
		modelMatrix.Identity();
		modelMatrix.Translate(pos_x, pos_y, 0.0f);
		modelMatrix.Rotate(0.25*3.14 * 2);
		modelMatrix.Scale(1.0f, 0.25f, 1.0f);
	}

	float pos_x;
	float pos_y = 0.0;
	float height = 1;
	float width = 0.25;
	float direction = 0;
};

class Ball {
public:
	Ball() {}
	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;

	void Draw() {
		modelMatrix.Identity();
		modelMatrix.Translate(pos_x, pos_y, 0.0f);
		modelMatrix.Scale(0.25f, 0.25f, 1.0f);
	}

	float pos_x = 0.0;
	float pos_y = 0.0;
	float dir_x = 0.7;
	float dir_y = 0.5;
	float width = 0.25;
	float height = 0.25;
};

class Win {
public:
	Win(){}
	Matrix projectionMatrix;
	Matrix modelMatrix;
	Matrix viewMatrix;
	void Draw() {
		modelMatrix.Identity();
		modelMatrix.Translate(pos_x, pos_y, 0.0);
	}
	float pos_x = 4.0;
	float pos_y = 0.0;
};


void DrawText(ShaderProgram *program, int fontTexture, std::string text, float size, float spacing);
int main(int argc, char *argv[]){
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	//Setup
	glViewport(0, 0, 640, 360);

	ShaderProgram program;
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
	GLuint txtr_plyr1 = LoadTexture(RESOURCE_FOLDER"Images/paddleBlu.png");
	GLuint txtr_plyr2 = LoadTexture(RESOURCE_FOLDER"Images/paddleRed.png");
	GLuint txtr_ball = LoadTexture(RESOURCE_FOLDER"Images/ballBlue.png");
	GLuint txtr_win = LoadTexture(RESOURCE_FOLDER"Images/Win.png");
	GLuint fontTexture = LoadTexture(RESOURCE_FOLDER"Images/font1.png");


	float angle = 0.0;
	float speed = 1.0;
	float lastFrameTicks = 0.0f;
	bool game = true;

	//players
	Player plyr1(-3.4);
	Player plyr2(3.4);
	
	//ball
	Ball ball;
	Win win;
	Matrix model;
	
	plyr1.projectionMatrix.SetOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	plyr2.projectionMatrix.SetOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	ball.projectionMatrix.SetOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	win.projectionMatrix.SetOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(program.programID);
	
	//End Setup
	//Running program
	SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (game) {
					//player 2 keys
					if (event.key.keysym.scancode == SDL_SCANCODE_UP) { plyr2.direction = 1; }
					else if (event.key.keysym.scancode == SDL_SCANCODE_DOWN) { plyr2.direction = -1; }

					//player 1 keys
					if (event.key.keysym.scancode == SDL_SCANCODE_W) { plyr1.direction = 1; }
					else if (event.key.keysym.scancode == SDL_SCANCODE_S) { plyr1.direction = -1; }
				}
				else if (game == false) {
					if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
						game = true;
						//reset game
						plyr1.pos_y = 0;
						plyr2.pos_y = 0;
						ball.pos_x = 0;
						ball.pos_y = 0;
						speed = 1.0;
						ball.dir_x = 0.7;
						ball.dir_y = 0.7;
						win.pos_x = 4.0;
					}
				}
			}
			else if (event.type == SDL_KEYUP) {
				plyr1.direction = 0;
				plyr2.direction = 0;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0/250.0, 0/250.0, 0/250.0, 1.0f);

		//Drawing
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;

		//Collision Detection
		if (plyr1.pos_y + elapsed * plyr1.direction >=  1.5|| 
			plyr1.pos_y + elapsed * plyr1.direction <= -1.5) {
			plyr1.direction = 0;
		}
		if (plyr2.pos_y + elapsed * plyr2.direction >= 1.5 || 
			plyr2.pos_y + elapsed * plyr2.direction <= -1.5) {
			plyr2.direction = 0;
		}
		//Ball hits boundary behind player
		if (ball.pos_x + ball.width/2 >= 3.55 || ball.pos_x - ball.width/2 <= -3.55) {
			ball.dir_x = 0;
			ball.dir_y = 0;
			game = false;
			if (ball.pos_x + ball.width / 2 >= 3.55 ) {
				win.pos_x = 2.5;
				
			}
			else if (ball.pos_x - ball.width / 2 <= -3.55) {
				win.pos_x = -2.5;
			}
		}

		//ball hits player1
		if (ball.pos_x - ball.width / 2 <= plyr1.pos_x + plyr1.width / 2) {
			if (ball.pos_y - ball.height / 2 <= plyr1.pos_y + plyr1.height / 2 && ball.pos_y + ball.height / 2 >= plyr1.pos_y - plyr1.height / 2) {
				ball.dir_x *= -1;
				speed += 0.2;
			}
		}

		//ball hits player2
		if (ball.pos_x + ball.width / 2 >= plyr2.pos_x-plyr2.width/2) {
			if (ball.pos_y - ball.height / 2 <= plyr2.pos_y + plyr2.height / 2 && ball.pos_y + ball.height / 2 >= plyr2.pos_y - plyr2.height / 2) {
				ball.dir_x *= -1;
				speed += 0.2;
			}
		}

		//y-axis
		if (ball.pos_y + (elapsed*ball.dir_y*speed) - ball.height/2 <= -2.0 ||
			ball.pos_y + (elapsed*ball.dir_y*speed) + ball.height/2 >= 2.0) {
			ball.dir_y *= -1;
		}

		//player 1 mvment
		plyr1.pos_y += elapsed * plyr1.direction;
		plyr1.Draw();
				
		//player 2 mvment
		plyr2.pos_y += elapsed * plyr2.direction;
		plyr2.Draw();
				
		//ball mvmnt
		ball.pos_x += elapsed * ball.dir_x*speed;
		ball.pos_y += elapsed * ball.dir_y*speed;
		ball.Draw();
		model.Identity();
		program.SetModelMatrix(model);
		DrawText(&program, fontTexture, "Rounds: ", 1.0f, 0.5f);
		win.Draw();

		program.SetModelMatrix(plyr1.modelMatrix);
		program.SetProjectionMatrix(plyr1.projectionMatrix);
		program.SetViewMatrix(plyr1.viewMatrix);
		glBindTexture(GL_TEXTURE_2D, txtr_plyr1);

		float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);	

		program.SetModelMatrix(plyr2.modelMatrix);
		program.SetProjectionMatrix(plyr2.projectionMatrix);
		program.SetViewMatrix(plyr2.viewMatrix);
		glBindTexture(GL_TEXTURE_2D, txtr_plyr2);

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);
		
		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		program.SetModelMatrix(ball.modelMatrix);
		glBindTexture(GL_TEXTURE_2D, txtr_ball);

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

		program.SetModelMatrix(win.modelMatrix);
		glBindTexture(GL_TEXTURE_2D, txtr_win);

		glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program.positionAttribute);

		glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program.texCoordAttribute);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);

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
void DrawText(ShaderProgram *program, int fontTexture, std::string text, float size, float spacing) {
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	float texture_size = 1.0 / 16.0f;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (int i = 0; i < text.size(); i++) {
		int spriteIndex = (int)text[i];
		float texture_x = (float)(spriteIndex % 16) / 16.0f;
		float texture_y = (float)(spriteIndex / 16) / 16.0f;

		vertexData.insert(vertexData.end(), {
			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (0.5f * size), -0.5f * size,
			((size + spacing) * i) + (0.5f * size), 0.5f * size,
			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
			});
		texCoordData.insert(texCoordData.end(), {
			texture_x, texture_y,
			texture_x, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x + texture_size, texture_y + texture_size,
			texture_x + texture_size, texture_y,
			texture_x, texture_y + texture_size,
			});
	}

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, text.length() * 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}
