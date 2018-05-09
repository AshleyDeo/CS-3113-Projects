#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <vector>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
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

#define MAX_ENEMIES 32

//functions
GLuint LoadTexture(const char *filePath);
void Setup();
void Events(SDL_Event* event, bool& done);
void Update();
void Render(ShaderProgram *program);


//classes
class SheetSprite {
public:
	SheetSprite() {}
	SheetSprite(unsigned int textureID, float u, float v, float width, float height, float
		size) : textureID(textureID), u(u), v(v), width(width), height(height), size(size) {}

	void Draw(ShaderProgram *program);

	float size;
	unsigned int textureID;
	float u;
	float v;
	float width;
	float height;
};
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
struct Vector3 {
	Vector3() {}
	Vector3(float x, float y, float z) :x(x), y(y), z(z) {}
	float x = 0.0;
	float y = 0.0;
	float z = 0.0;
};

class Object {
public:
	//Object
	Object() {
		scale.x = 1.0;
		scale.y = 1.0;
		scale.z = 1.0;
	}
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
		sprites[spriteIndex].Draw(program);

	}
	Matrix modelMatrix;
	Vector3 position;
	Vector3 scale;
	Vector3 velocity;
	Vector3 direction;
	std::vector<SheetSprite> sprites;
	float spriteIndex = 0;
	bool dead = false;
};

class Player : public Object {
public:
	Player(float x, float y, float z): Object(x, y, z) {
		scale.x = 0.3 * 1.75;
		scale.y = 1.75;
		velocity.x = 1.5;
	}
	void Update(float elapsed);
};

class Enemy : public Object {
public:
	Enemy() :Object() {}
	Enemy(float x, float y, float z) : Object(x, y, z) {
		scale.x = 0.5;
		scale.y = 1.0;
		velocity.x = 1.0;
	}
	void Update(float elapsed);
};

class Bullet : public Object {
public:
	Bullet() : Object() {
		scale.x = 0.2;
		scale.y = 0.2;
		velocity.y = 1.5;
	}
	void Update(float elapsed);
};
class Text : public Object {
public:
	Text() : Object() {}
	GLuint fontTexture;

	void Draw(ShaderProgram *program, std::string str) {
		modelMatrix.Identity();
		modelMatrix.Translate(position.x, position.y, position.z);
		modelMatrix.Scale(scale.x, scale.y, scale.z);
		program->SetModelMatrix(modelMatrix);
		DrawText(program, fontTexture, str, 0.3f, 0.0f);
	}
};

//states and modes
enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL };
GameMode mode = STATE_MAIN_MENU;
class GameLevel {
public:
	void Events(SDL_Event* event, bool& done);
	void Update(float elapsed);
	void Render(ShaderProgram *program);
	int checkAlive();
	void Reset();
	std::vector<Enemy*> enemies;
	int rounds = 1;
	Mix_Music* music;
	Mix_Chunk* pShootSound;
	Mix_Chunk* destroySound;
};
class MainMenu {
public:
	MainMenu() {}
	void Events(SDL_Event* event, bool& done);
	void Update(float elapsed);
	void Render(ShaderProgram *program);
	Enemy start;
	Mix_Music* music;
};

MainMenu mainMenu;
GameLevel gameLevel;
Text text;
//vars
float lastFrameTicks = 0.0f;
Player player(0.0, -1.8, 0.0);

//Bullets
#define MAX_BULLETS 20
int bulletIndex = 0;
Bullet bullets[MAX_BULLETS];
void shootBullet(Object* shooter);
float shoot_x;
float shoot_y;
Object* shotFrom;
Mix_Music* currMusic;

int main(int argc, char *argv[]){
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	displayWindow = SDL_CreateWindow("Space Invaders", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	//Setup
	Setup();

	ShaderProgram program;
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");

	Matrix projectionMatrix;
	projectionMatrix.SetOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	program.SetProjectionMatrix(projectionMatrix);
	Matrix viewMatrix;
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

//SpriteSheet
void SheetSprite::Draw(ShaderProgram *program) {
	glBindTexture(GL_TEXTURE_2D, textureID);
	GLfloat texCoords[] = {
		u, v + height,
		u + width, v,
		u, v,
		u + width, v,
		u, v + height,
		u + width, v + height
	};
	float aspect = width / height;
	float vertices[] = {
		-0.5f * size * aspect, -0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, 0.5f * size,
		0.5f * size * aspect, 0.5f * size,
		-0.5f * size * aspect, -0.5f * size ,
		0.5f * size * aspect, -0.5f * size 
	};

	// draw our arrays
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}
void shootBullet(Object* shooter) {
	Mix_PlayChannel(-1, gameLevel.pShootSound, 0);

	bullets[bulletIndex].position.x = shooter->position.x;
	bullets[bulletIndex].position.y = shooter->position.y;
	if (shooter == &player) {
		bullets[bulletIndex].direction.y = 1;
		bullets[bulletIndex].spriteIndex = 0;
	}
	else {
		bullets[bulletIndex].direction.y = -1;
		bullets[bulletIndex].spriteIndex = 1;
	}
	shotFrom = shooter;
	bulletIndex++;
	if (bulletIndex > MAX_BULLETS - 1) {
		bulletIndex = 0;
	}
}

//Player
void Player::Update(float elapsed) {
	// hit screen edge
	if (position.x + velocity.x * elapsed * direction.x <= -3.55 + scale.x / 2 ||
		position.x + velocity.x * elapsed * direction.x >= 3.55 - scale.x / 2) { 
		direction.x = 0;
	}
	position.x += velocity.x * elapsed * direction.x;
}
//Enemy
void Enemy::Update(float elapsed) {
	position.x += velocity.x * elapsed * direction.x;
}
//Bullet
void Bullet::Update(float elapsed) {
	//Mix_VolumeChunk(gameLevel.destroySound, 100);
	if (direction.y == 1 && mode == STATE_GAME_LEVEL) {
		for (Enemy* enemy : gameLevel.enemies) { //check if enemies get hit
			if (!enemy->dead) {
				if (position.x >= enemy->position.x - enemy->scale.x / 2 &&
					position.x <= enemy->position.x + enemy->scale.x / 2 &&
					position.y >= enemy->position.y - enemy->scale.y / 2 &&
					position.y <= enemy->position.y + enemy->scale.y / 2) {
					Mix_PlayChannel(-1, gameLevel.destroySound, 0);
					enemy->dead = true;
					position.y = -2.5f;
					direction.y = 0;
				}
			}

		}
	}
	else if (mode == STATE_MAIN_MENU) {
		if (position.x >= mainMenu.start.position.x - mainMenu.start.scale.x / 2 &&
			position.x <= mainMenu.start.position.x + mainMenu.start.scale.x / 2 &&
			position.y >= mainMenu.start.position.y - mainMenu.start.scale.y / 2 &&
			position.y <= mainMenu.start.position.y + mainMenu.start.scale.y / 2) {
			Mix_PlayChannel(-1, gameLevel.destroySound, 0);
			mode = STATE_GAME_LEVEL;
			Mix_PlayMusic(gameLevel.music, -1);
			position.y = -2.5f;
			direction.y = 0;
		}
	}
	else if (direction.y == -1){
		if (position.x >= player.position.x - player.scale.x / 2 &&
			position.x <= player.position.x + player.scale.x / 2 &&
			position.y >= player.position.y - player.scale.y / 2 &&
			position.y <= player.position.y + player.scale.y / 2) {
			Mix_PlayChannel(-1, gameLevel.destroySound, 0);
			position.y = -2.5f;
			direction.y = 0;
			player.dead = true;
		}
	}
	position.y += velocity.y * elapsed * direction.y;
}
//Main Functions
void Setup() {
	glViewport(0, 0, 640, 360);
	GLuint spriteSheetTexture = LoadTexture(RESOURCE_FOLDER"Images/space_invaders_sprite_sheet.png");
	player.sprites.push_back(SheetSprite(spriteSheetTexture, 141.0f / 617.0f, 452.0f / 2035.0f, 28.0f / 617.0f, 14.0f / 2035.0f, 0.2));
	//player.sprites.push_back(SheetSprite(spriteSheetTexture, 66.0f / 617.0f, 74.0f / 2035.0f, 32.0f / 617.0f, 17.0f / 2035.0f, 0.2));
	text.fontTexture = LoadTexture(RESOURCE_FOLDER"Images/font1.png");
	int index = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 8; j++) {
			if (index < 32) {
				gameLevel.enemies.push_back(new Enemy(-3.0 + j * 0.8, 1.5 - i * 0.4, 0.0));
				gameLevel.enemies[index]->sprites.push_back(SheetSprite(spriteSheetTexture, 7.0f / 617.0f, 225.0f / 2035.0f, 16.0f / 617.0f, 16.0f / 2035.0f, 0.2));
				//gameLevel.enemies[index]->sprites.push_back(SheetSprite(spriteSheetTexture, 37.0f / 617.0f, 75.0f / 2035.0f, 27.0f / 617.0f, 17.0f / 2035.0f, 0.2));
			}
			index++;
		}
	}
	for (Bullet& bullet : bullets) {
		bullet.position.y = -2.5f;
		bullet.sprites.push_back(SheetSprite(spriteSheetTexture, 183.0f / 617.0f, 444.0f / 2035.0f, 4.0f / 617.0f, 18.0f / 2035.0f, 0.8));
		bullet.sprites.push_back(SheetSprite(spriteSheetTexture, 201.0f / 617.0f, 600.0f / 2035.0f, 2.0f / 617.0f, 20.0f / 2035.0f, 1.0));
	}
	mainMenu.start.position.y = 1.0;
	mainMenu.start.scale.x = 1.2;
	mainMenu.start.scale.y = 1.5;
	mainMenu.start.sprites.push_back(SheetSprite(spriteSheetTexture, 172.0f / 617.0f, 0.0f / 2035.0f, 231.0f / 617.0f, 166.0f / 2035.0f, 1.0));

	//MUSIC
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
	gameLevel.pShootSound = Mix_LoadWAV("Music/shoot.wav");
	gameLevel.destroySound = Mix_LoadWAV("Music/kill.wav");
	mainMenu.music = Mix_LoadMUS("Music/RideOn_MMSF.wav");
	gameLevel.music = Mix_LoadMUS("Music/GameTheme.wav");
	
	Mix_PlayMusic(mainMenu.music, -1);
}
void Events(SDL_Event* event, bool& done) {
	switch (mode) {
	case STATE_MAIN_MENU:
		mainMenu.Events(event, done);
		break;
	case STATE_GAME_LEVEL:
		gameLevel.Events(event, done);
		break;
	}
}
void Update() {
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	float elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;

	switch (mode) {
	case STATE_MAIN_MENU:
		currMusic = mainMenu.music;
		mainMenu.Update(elapsed);
		break;
	case STATE_GAME_LEVEL:
		gameLevel.Update(elapsed);
		break;
	}
}
void Render(ShaderProgram *program) {
	switch (mode) {
	case STATE_MAIN_MENU:
		mainMenu.Render(program);
		break;
	case STATE_GAME_LEVEL:
		gameLevel.Render(program);
		break;
	}
}
//Main Menu
void MainMenu::Events(SDL_Event* event, bool& done) {
	while (SDL_PollEvent(event)) {
		if (event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT_CLOSE) { // check if window close
			done = true;
		}
		else if (event->type == SDL_KEYDOWN) {
			//player controls
			if (event->key.keysym.scancode == SDL_SCANCODE_RIGHT) { player.direction.x = 1; }
			else if (event->key.keysym.scancode == SDL_SCANCODE_LEFT) { player.direction.x = -1; }
			
			if (start.direction.x == 0) { //start enemy movement 
				start.direction.x = 1;
			}
			
		}
		else if (event->type == SDL_KEYUP) {
			
			if (event->key.keysym.scancode == SDL_SCANCODE_SPACE) { //Shooting bullets
				shootBullet(&player);
			}
			if (event->key.keysym.scancode == SDL_SCANCODE_RIGHT) { player.direction.x = 0; }
			else if (event->key.keysym.scancode == SDL_SCANCODE_LEFT) { player.direction.x = 0; }
			
		}
	}
}

void MainMenu::Update(float elapsed) {
	player.Update(elapsed);
	start.Update(elapsed);
	
	for (int i = 0; i < MAX_BULLETS; i++) {
		bullets[i].Update(elapsed);
	}
}

void MainMenu::Render(ShaderProgram *program) {
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
	glClearColor(0/255, 0/255, 0/255, 1.0f);
	mainMenu.start.Draw(program);
	player.Draw(program);
	for (Bullet bullet : bullets) {
		bullet.Draw(program);
	}
	text.scale.x = 0.5;
	text.scale.y = 0.8;
	text.position.x = -2.0;
	std::string str = "Move: ARROW KEYS   Shoot: SPACE";
	text.Draw(program, str);

}
//Game Level
void GameLevel::Events(SDL_Event* event, bool& done) {
	while (SDL_PollEvent(event)) {
		if (event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT_CLOSE) { // check if window close
			done = true;
		}
		else if (event->type == SDL_KEYDOWN) {
			//player controls
			if (event->key.keysym.scancode == SDL_SCANCODE_RIGHT) { player.direction.x = 1; }
			else if (event->key.keysym.scancode == SDL_SCANCODE_LEFT) { player.direction.x = -1.5; }

			if (enemies[0]->direction.x == 0) { //start enemy movement 
				for (Enemy* enemy : gameLevel.enemies) {
					enemy->direction.x = 1;
				}
			}

		}
		else if (event->type == SDL_KEYUP) {
			player.direction.x = 0;
			if (event->key.keysym.scancode == SDL_SCANCODE_SPACE) { //Shooting bullets
				if (!player.dead) {
					shootBullet(&player);
					if (rounds > 0 && rounds < 10) {
						int enemyShot = rand() % 50;
						if (enemyShot < 32) {
							if (!enemies[enemyShot]->dead) {
								shootBullet(enemies[enemyShot]);
							}
						}
					}
				}
				else {
					player.dead = false;
					Reset();
					rounds = 1;
					mode = STATE_MAIN_MENU;
					Mix_PlayMusic(mainMenu.music, -1);
					mainMenu.start.dead = false;
				}
			}
		}
		
	}
}

void GameLevel::Update(float elapsed) {
	player.Update(elapsed);

	if (enemies[0]->position.x + enemies[0]->velocity.x * elapsed * enemies[0]->direction.x <= -3.55 + enemies[0]->scale.x / 2 ||
		enemies[7]->position.x + enemies[0]->velocity.x * elapsed * enemies[0]->direction.x >= 3.55 - enemies[0]->scale.x / 2) {
		for (Enemy* enemy : enemies) { //all enemies move together 
			enemy->direction.x *= -1;
		}
	}

	for (Enemy* enemy : enemies) {
		enemy->Update(elapsed);
	}
	for (int i = 0; i < MAX_BULLETS; i++) {
		bullets[i].Update(elapsed);
	}
	
	int alive = checkAlive();
	if (alive == 0) {
		Reset();
		rounds++;
	}
}
void GameLevel::Render(ShaderProgram *program) {
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0 / 255, 0 / 255, 0 / 255, 1.0f);
	std::string str = "Round " + std::to_string(rounds);
	if (!player.dead) {
		text.scale.x = 1.0;
		text.scale.y = 1.0;
		text.position.y = 1.9;
		text.position.x = -3.2;
		for (Enemy* enemy : enemies) {
			if (!enemy->dead) {
				enemy->Draw(program);
			}
		}
		player.Draw(program);
		for (Bullet bullet : bullets) {
			bullet.Draw(program);
		}
	}
	else {
		text.scale.x = 2.0;
		text.scale.y = 2.0;
		text.position.y = 0.0;
		text.position.x = -2.0;
	}
	text.Draw(program, str);
}
int GameLevel::checkAlive() {
	int alive = 0;
	for (Enemy* enemy : enemies) {
		if (!enemy->dead) {
			alive++;
		}
	}
	return alive;
}
void GameLevel::Reset() {
	int index = 0;
	player.position.x = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 8; j++) {
			if (index < 32) {
				enemies[index]->dead = false;
				enemies[index]->position.x = -3.0 + j * 0.8;
				enemies[index]->position.y = 1.5 - i * 0.4;
			}
			index++;
		}
	}
	for (Bullet bullet : bullets) {
		bullet.position.y = -2.5;
	}
}