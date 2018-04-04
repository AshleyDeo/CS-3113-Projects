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
void PlaceEntity(std::string type, float x, float y);

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
//Map
class Map{
public:
	Map(){}
	void Draw(ShaderProgram *program);

	Matrix model;
};

FlareMap fmap;
Map map;
float TILE_SIZE = 0.45;

void DrawMap(ShaderProgram *program) {
	float spriteSize = 16.0;
	int SPRITE_COUNT_X = 15;
	int SPRITE_COUNT_Y = 7;
	float spriteWidth = 1.0 / 20;
	float spriteHeight = 1.0 / 11.0;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (int y = 0; y < fmap.mapHeight; y++) {
		for (int x = 0; x < fmap.mapWidth; x++) {
			int spriteIndex = (int)fmap.mapData[y][x];
			float u = (float)(((int)spriteIndex) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
			float v = (float)(((int)spriteIndex) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
			if (fmap.mapData[y][x] == 12) {
				u = (float)(((int)spriteIndex+1) % SPRITE_COUNT_X-0.1) / (float)SPRITE_COUNT_X-0.1;
			}
			vertexData.insert(vertexData.end(), {

				TILE_SIZE * x, -TILE_SIZE * y,
				TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
				(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,

				TILE_SIZE * x, -TILE_SIZE * y,
				(TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
				(TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
				});

			texCoordData.insert(texCoordData.end(), {
				u, v,
				u, v + (spriteHeight),
				u + spriteWidth, v + (spriteHeight),

				u, v,
				u + spriteWidth, v + (spriteHeight),
				u + spriteWidth, v
				});
		}
	}
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, &vertexData[0]);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, &texCoordData[0]);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, fmap.mapHeight * fmap.mapWidth * 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}
void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
	*gridX = (int)(worldX / TILE_SIZE);
	*gridY = (int)(-worldY / TILE_SIZE);
}

//Player
class Player : public Object {
public:
	Player() : Object() {
		direction.y = 0;
		velocity.x = 5;
		velocity.y = 0.5;
		scale.x = 0.5;
		scale.y = 0.5;
	}
	void Update(float elapsed);
	Vector3 velocity;
	Vector3 direction;
};
//Collectibles
class Collectible : public Object {
public:
	Collectible() : Object() {
		scale.x = 0.4;
		scale.y = 0.4;
	}
	bool taken = false;
};

Player player;
Collectible collectibles[28];
int c_index = 0;
float lastFrameTicks = 0.0f;
Matrix projectionMatrix;
Matrix viewMatrix;
float gravity = -2.0;

int main(int argc, char *argv[]) {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Platformer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
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
void PlaceEntity(std::string type, float x, float y) {
	if (type == "Player") {
		player.position.x = x;
		player.position.y = y;
		player.spriteIndex = 78;
	}
	if (type == "Collect") {
		collectibles[c_index].position.x = x + 0.3;
		collectibles[c_index].position.y = y;
		collectibles[c_index].spriteIndex = 22;
		c_index++;
	}
}
void Map::Draw(ShaderProgram *program) {
	model.Identity();
	program->SetModelMatrix(model);
	DrawMap(program);
}

void Player::Update(float elapsed) {
	velocity.y += gravity * elapsed;
	position.x += velocity.x * elapsed * direction.x;
	position.y += velocity.y * elapsed;
	for (Collectible& collectible : collectibles) {
		if (!collectible.taken) {
			if (collectible.position.x >= position.x - (scale.x / 2.0) && collectible.position.x <= position.x + (scale.x / 2.0)
				&& collectible.position.y >= position.y - (scale.y / 2.0) && collectible.position.y <= position.y + (scale.y / 2.0)) {
				collectible.taken = true;
			}
			if (collectible.position.x <= position.x - (scale.x / 2.0) && collectible.position.x >= position.x + (scale.x / 2.0)
				&& collectible.position.y >= position.y - (scale.y / 2.0) && collectible.position.y <= position.y + (scale.y / 2.0)) {
				collectible.taken = true;
			}
		}
	}
}
//Main Functions
GLuint spriteSheet;
void Setup() {
	glViewport(0, 0, 640, 360);
	spriteSheet = LoadTexture(RESOURCE_FOLDER"Images/arne_sprites.png");
	fmap.Load("Images/GameMap.txt");
	for (int i = 0; i < fmap.entities.size(); i++) {
		PlaceEntity(fmap.entities[i].type, fmap.entities[i].x * TILE_SIZE, fmap.entities[i].y * -TILE_SIZE);
	}
}
void Events(SDL_Event* event, bool& done) {
	while (SDL_PollEvent(event)) {
		if (event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		else if (event->type == SDL_KEYDOWN) {
			//player controls
			if (event->key.keysym.scancode == SDL_SCANCODE_RIGHT) {
				player.direction.x = 1; player.scale.x = 0.5;
			}
			else if (event->key.keysym.scancode == SDL_SCANCODE_LEFT) {
				player.direction.x = -1; player.scale.x = -0.5;
			}
			if (event->key.keysym.scancode == SDL_SCANCODE_SPACE) {
				player.velocity.y = 2.0;
			}
		}
		else if (event->type == SDL_KEYUP) {
			if (event->key.keysym.scancode == SDL_SCANCODE_RIGHT|| event->key.keysym.scancode == SDL_SCANCODE_LEFT) {
				player.direction.x = 0;
				player.direction.y = 0;
			}
		}
	}
}
void Update() {
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	float elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;
	int* gridX = new int(0);
	int* gridY = new int(0);
	worldToTileCoordinates(player.position.x, player.position.y, gridX, gridY);
	player.Update(elapsed);
	for (int y = 0; y < fmap.mapHeight; y++) {
		for (int x = 0; x < fmap.mapWidth; x++) {
			if (fmap.mapData[y][x] != 12 && fmap.mapData[y][x] != 33 && fmap.mapData[y][x] != 30
				&& fmap.mapData[y][x] != 31 && fmap.mapData[y][x] != 32) {
				if (player.position.x + player.scale.x / 2 >= (x * TILE_SIZE) && *gridX <= x && *gridY == y) {
					player.direction.x = 0;
				}
				else if (player.position.x - fabs(player.scale.x / 2) <= (x * TILE_SIZE) + TILE_SIZE && *gridX >= x && *gridY == y) {
					player.direction.x = 0;
					player.position.x += 0.005;
				}
				if (player.position.y - player.scale.y / 2 <= (y * -TILE_SIZE) && *gridY <= y && *gridX == x) {
					player.velocity.y = 0;
					player.position.y += 0.001;
				}
				else if (player.position.y + fabs(player.scale.y / 2) >= (y * -TILE_SIZE) - TILE_SIZE && *gridY >= y && *gridX == x) {
					player.velocity.y = 0;
					player.position.y -= 0.001;
				}
			}
		}
	}
}

void Render(ShaderProgram *program) {
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(136 / 255, 255 / 255, 255 / 255, 1.0f);
	viewMatrix.Identity();
	viewMatrix.Translate(-player.position.x, -player.position.y, 0.0);
	program->SetViewMatrix(viewMatrix);
	map.Draw(program);
	for (Collectible& collectible : collectibles) {
		if (!collectible.taken) {
			collectible.Draw(program);
		}
	}
	player.Draw(program);	
}
