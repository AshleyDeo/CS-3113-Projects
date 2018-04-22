#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <vector>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
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
//int Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize);
/*
player vs player
MMSF vs MMBN
*/

//functions
void Setup();
void Events(SDL_Event* event, bool& done);
void Update();
void Render(ShaderProgram *program);
GLuint LoadTexture(const char *filePath);
void DrawSprite(ShaderProgram *program, int index, int spriteCountX, int spriteCountY);
void PlaceEntity(std::string type, float x, float y);
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

//classes
struct Vector3 {
	Vector3() {}
	Vector3(float x, float y, float z) :x(x), y(y), z(z) {}
	float x = 0.0;
	float y = 0.0;
	float z = 0.0;
};
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
//Map
class Map {
public:
	Map() {}
	void Draw(ShaderProgram *program);
	Matrix model;
};
GLuint fontTexture;
//Text
class Text {
public:
	Text() {
		scale.x = 1.0;
		scale.y = 1.0;
		scale.z = 1.0;
	}

	void Draw(ShaderProgram *program, std::string str, float size, float spacing) {
		model.Identity();
		model.Translate(position.x, position.y, position.z);
		model.Scale(scale.x, scale.y, scale.z);
		program->SetModelMatrix(model);
		DrawText(program, fontTexture, str, size, spacing);
	}
	
	Matrix model;
	Vector3 position;
	Vector3 scale;
};

class Particles {
public:
	Particles(){
		scale.x = -1.0;
		scale.y = 1.0;
		scale.z = 1.0;
		
	}
	void Update(float elapsed);
	void Draw(ShaderProgram *program);

	Matrix model;
	Vector3 position;
	Vector3 scale;
	bool alive = false;
	int xIndex;
	int playerx;
	int arenax;
	int arenay;

	enum ParticleType { TYPE_IDLE, TYPE_WAVE, TYPE_EXPLODE };
	ParticleType pType = TYPE_IDLE;
	int spriteIndex = 0;
	int numFrames;
	float animationElapsed = 0.0f;
	float framesPerSecond;

	SheetSprite sprites_wave[7];
	SheetSprite sprites_expolde[3];

};

class Object {
public:
	Object() {}
	Object(float x, float y, float z) : position(x, y, z) {
		scale.x = 1.0;
		scale.y = 1.0;
		scale.z = 1.0;
	}
	Matrix modelMatrix;
	Vector3 position;
	Vector3 scale;

	enum ObjMode { MODE_IDLE, MODE_ATTACK, MODE_HIT };
	ObjMode objmode = MODE_IDLE;
	enum AttackMode { MODE_NORM_SHOOT, MODE_SHOOT, MODE_SMASH, MODE_THROW };

	bool dead = false;
};

//Player
class PlayerB : public Object {
public:
	PlayerB() : Object() {
		direction.y = 0;
		velocity.x = 5;
		velocity.y = 0.5;
		scale.x = 1.0;
		scale.y = 2.6;
		HP = 100;
	}
	void Update(float elapsed);
	void Draw(ShaderProgram *program);
	GLuint texture;
	Vector3 velocity;
	Vector3 direction;
	int HP;
	//Sprites
	SheetSprite sprites_idle;
	SheetSprite sprites_shoot[4];
	SheetSprite sprites_smash[6];
	SheetSprite sprites_throw[5];
	SheetSprite sprites_hit[5];

	AttackMode attack = MODE_NORM_SHOOT;
		//Animation
	int spriteIndex = 0;
	int numFrames;
	float animationElapsed = 0.0f;
	float framesPerSecond;
	bool pattack = false;
	float wavetimer = 0.0;
};
//ENEMIES
class Enemy : public Object {
public:
	Enemy() : Object() {
		scale.x = 1.0;
		scale.y = 1.0;
	}
	
	ObjMode emode = MODE_IDLE;
	AttackMode attack = MODE_SMASH;
	virtual void Update(float elapsed) = 0;
	virtual void Draw(ShaderProgram *program) = 0;
	int HP;
	Text EnemyHP;
};
class Mettenna : public Enemy {
public:
	Mettenna() :Enemy() {
		scale.x = 2.0;
		scale.y = 1.5; 
		HP = 100;
	}
	void Update(float elapsed);
	void Draw(ShaderProgram *program) {
		modelMatrix.Identity();
		modelMatrix.Translate(position.x, position.y, position.z);
		modelMatrix.Scale(scale.x, scale.y, scale.z);
		program->SetModelMatrix(modelMatrix);
		switch (emode)
		{
		case Mettenna::MODE_IDLE:
			spriteIndex = 0;
			position.y += 1;
			sprites_idle.Draw(program);
			break;
		case Mettenna::MODE_ATTACK:
			/*numFrames = 4;
			position.x += 1.0;
			framesPerSecond = 10.0;
			sprites_attck[spriteIndex].Draw(program);*/
			break;
		case Mettenna::MODE_HIT:
			/*spriteIndex = 0;
			position.y += 1;
			sprites_idle.Draw(program); SHADER*/
			break;
		}
		EnemyHP.Draw(program, std::to_string(HP), 0.2f, -0.1f);
	}
	SheetSprite sprites_idle;
	int spriteIndex = 0;
	float moveTimer = 0.0;
	float attackTimer = 0.0;
};

void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
	/**gridX = (int)(worldX / TILE_SIZE);
	*gridY = (int)(-worldY / TILE_SIZE);*/
}

//Game STATE/MODES
enum GameMode { STATE_MAIN_MENU, STATE_OVERWORLD, STATE_BATTLE_MODE, STATE_GAME_OVER };
GameMode mode = STATE_BATTLE_MODE;
class BattleMode {
public:
	BattleMode() {
		/*
		Row1:	(0.6,0.2)	(1.8,0.2)	(3.0,0.2)	|	(4.2,0.2)	(5.4,0.2)	(6.6,0.2)
		Row2:	(0.6,-0.6)	(1.8,-0.6)	(3.0,-0.6)	|	(4.2,-0.6)	(5.4,-0.6)	(6.6,-0.6)
		Row3:	(0.6,-1.3)	(1.8,-1.3)	(3.0,-1.3)	|	(4.2,-1.3)	(5.4,-1.3)	(6.6,-1.3)
		*/
		for (int x = 0; x < 6; ++x) {
			for (int y = 0; y < 4; ++y) {
				if (x == 0) {
					arena[x][y].first = 0.6;
				}
				if (x == 1) {
					arena[x][y].first = 1.8;
				}
				if (x == 2) {
					arena[x][y].first = 3.0;
				}
				if (x == 3) {
					arena[x][y].first = 4.2;
				}
				if (x == 4) {
					arena[x][y].first = 5.4;
				}
				if (x == 5) {
					arena[x][y].first = 6.6;
				}
				if (y == 0) {
					arena[x][y].second = 0.2;
				}
				if (y == 1) {
					arena[x][y].second = -0.6;
				}
				if (y == 2) {
					arena[x][y].second = -1.3;
				}
			}
		}
	}
	void Events(SDL_Event* event, bool& done);
	void Update(float elapsed);
	void Render(ShaderProgram *program);
	void DrawMap(ShaderProgram *program);
	void CollisionCheck() {
		if (player.objmode == Object::MODE_ATTACK) {
			if (player.attack == player.MODE_NORM_SHOOT) {
				if (enemyy == playery) {
					enemy1.HP -= 1;
				}
			}
			if (player.attack == player.MODE_SHOOT) {
				if (enemyy == playery) {
					enemy1.HP -= 3;
				}
			}
			if (player.attack == player.MODE_SMASH) {
				if (enemyx == particle.arenax + 1 && enemyy == particle.arenay) {
					enemy1.HP -= 5;
					enemy1.objmode = Object::MODE_HIT;
				}
			}
			if (player.attack == player.MODE_THROW) {
				if (enemyx == playerx + 3) {
					enemy1.HP -= 4;
				}
			}
		}
		if (enemy1.objmode == Object::MODE_ATTACK) {
			if (enemy1.attack == enemy1.MODE_NORM_SHOOT) {
				if (enemyy == playery) {
					player.HP -= 1;
				}
			}
			if (enemy1.attack == enemy1.MODE_SHOOT) {
				if (enemyy == playery) {
					player.HP -= 3;
				}
			}
			if (enemy1.attack == enemy1.MODE_SMASH) {
				if (playerx == particle.arenax - 1 && playery == particle.arenay) {
					player.HP -= 5;
					player.objmode = Object::MODE_HIT;
				}
			}
			if (enemy1.attack == enemy1.MODE_THROW) {
				if (enemyx == playerx + 3) {
					player.HP -= 4;
				}
			}
		}
	}
	~BattleMode() {
		for (Enemy* enemy : enemies) {
			delete enemy;
		}
	}

	//MAP
	Map map;
	GLuint mapTexture;
	FlareMap fmap;
	float TILE_SIZE = 0.3;
	std::pair<float, float> arena[6][3];
	Particles particle;

	//PLAYER
	PlayerB player;
	int playerx = 1;
	int playery = 1;

	//ENEMIES
	int enemyx = 4;
	int enemyy = 1;
	std::vector<Enemy*> enemies;
	Mettenna enemy1;
	int c_index = 0;

	//UI
	Text PlayerHP;
};
BattleMode battle;

float lastFrameTicks = 0.0f;
Matrix projectionMatrix;
Matrix viewMatrix;
//float gravity = -2.0;

int main(int argc, char *argv[]) {
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("MMSF/BN Fan Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
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

	float vertices[] = { -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, -0.5f,
		-0.5f, 0.5f, -0.5f };

	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}
void PlaceEntity(std::string type, float x, float y) {
	/*if (type == "Player") {
		player.position.x = x;
		player.position.y = y;
		player.spriteIndex = 78;
	}
	if (type == "Collect") {
		enemies[c_index].position.x = x + 0.3;
		enemies[c_index].position.y = y;
		enemies[c_index].spriteIndex = 22;
		c_index++;
	}*/
}
void Map::Draw(ShaderProgram *program) {
	model.Identity();
	model.Scale(4.0, 1.0, 1.0);
	program->SetModelMatrix(model);
	glBindTexture(GL_TEXTURE_2D, battle.mapTexture);
	battle.DrawMap(program);
}
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
void BattleMode::DrawMap(ShaderProgram *program) {
	float spriteSize = 16.0;
	int SPRITE_COUNT_X = 12;
	int SPRITE_COUNT_Y = 16;
	float spriteWidth = 1.0 / 12;
	float spriteHeight = 1.0 / 16;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (int y = 0; y < fmap.mapHeight; y++) {
		for (int x = 0; x < fmap.mapWidth; x++) {
			int spriteIndex = (int)fmap.mapData[y][x];
			float u = (float)(((int)spriteIndex) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
			float v = (float)(((int)spriteIndex) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
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
void Particles::Update(float elapsed) {
	animationElapsed += elapsed;
	if (animationElapsed > 1.0 / framesPerSecond) {
		spriteIndex++;
		animationElapsed = 0.0;
		if (spriteIndex > numFrames - 1) {
			spriteIndex = 0;
			if (arenax == 5) {
				pType = TYPE_IDLE;
			}
			else {
				battle.CollisionCheck();
				arenax += 1;
			}
		}
	}
}
void Particles::Draw(ShaderProgram *program) {
	model.Identity();
	model.Translate(battle.arena[arenax][arenay].first, battle.arena[arenax][arenay].second, position.z);
	model.Scale(scale.x, scale.y, scale.z);
	program->SetModelMatrix(model);

	switch (pType) {
	case TYPE_IDLE:
		break;
	case TYPE_WAVE:
		numFrames = 7;
		framesPerSecond = 30.0;
		sprites_wave[spriteIndex].Draw(program);
		break;
	case TYPE_EXPLODE:
		numFrames = 4;
		framesPerSecond = 15.0;
		sprites_expolde[spriteIndex].Draw(program);
		break;
	default:
		break;
	}
}
void PlayerB::Update(float elapsed) {
	if (HP == 0) { dead = true; }
	animationElapsed += elapsed;	
	if (animationElapsed > 1.0 / framesPerSecond) {
		spriteIndex++;
		animationElapsed = 0.0;
		if (spriteIndex > numFrames - 1) {
			objmode = MODE_IDLE;
		}
	}
	if (objmode == MODE_ATTACK) {
		if (attack == MODE_SHOOT || attack == MODE_NORM_SHOOT) {
			if (spriteIndex == 0) {
				position.x += 0.2;
			}
			else {
				position.x += 0.5;
			}
		}
		if (attack == MODE_THROW) {
			position.x += 0.2;
		}
	}
	
}
void PlayerB::Draw(ShaderProgram *program) {
	modelMatrix.Identity();
	modelMatrix.Translate(position.x, position.y, position.z);
	modelMatrix.Scale(scale.x, scale.y, scale.z);
	program->SetModelMatrix(modelMatrix);
	switch (objmode)
	{
	case Object::MODE_IDLE:
		spriteIndex = 0;
		pattack = false;
		attack = MODE_NORM_SHOOT;
		sprites_idle.Draw(program);
		break;
	case Object::MODE_HIT:
		numFrames = 4;
		framesPerSecond = 10.0;
		sprites_hit[spriteIndex].Draw(program);
		break;
	case Object::MODE_ATTACK:
		switch (attack) {
		case Object::MODE_NORM_SHOOT:
			numFrames = 4;
			framesPerSecond = 15.0;
			sprites_shoot[spriteIndex].Draw(program);
			break;
		case Object::MODE_SHOOT:
			numFrames = 4;
			framesPerSecond = 15.0;
			sprites_shoot[spriteIndex].Draw(program);
			break;
		case Object::MODE_SMASH:
			numFrames = 6;
			framesPerSecond = 10.0;
			sprites_smash[spriteIndex].Draw(program);
			pattack = true;
			break;
		case Object::MODE_THROW:
			numFrames = 5;
			framesPerSecond = 10.0;
			sprites_throw[spriteIndex].Draw(program);
			break;
		default:
			break;
		}
	default:
		break;
	}
}
void Mettenna::Update(float elapsed) {
	if (HP <= 0) { dead = true; }
	EnemyHP.position.y = position.y + 0.1;

	if (HP > 10) { EnemyHP.position.x = position.x - 0.05; }
	else { EnemyHP.position.x = position.x - 0.05; }
}
//Main Functions
GLuint spriteSheet;
bool start = false;

void Setup() {
	glViewport(0, 0, 640, 360);
	/*for (int i = 0; i < fmap.entities.size(); i++) {
	PlaceEntity(fmap.entities[i].type, fmap.entities[i].x * TILE_SIZE, fmap.entities[i].y * -TILE_SIZE);
	}*/
	fontTexture = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/font1.png");
	//BATTLE
	//MUSIC

	//MAP
	battle.mapTexture = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/bn4_panels.png");
	battle.fmap.Load("Assets/BattleMap.txt");
	//Enemies
	battle.enemies.push_back(new Mettenna());
	battle.enemies.push_back(new Mettenna());
	battle.enemies.push_back(new Mettenna());
	//Mettenna
	GLuint mettennaTexture = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/Mettaur.png");
	//idle
	battle.enemy1.sprites_idle = SheetSprite(mettennaTexture, 43.0 / 376.0, 4.0 / 245.0, 22.0 / 376.0, 46.0 / 245.0, 0.8);
	battle.particle.sprites_wave[0] = SheetSprite(mettennaTexture, 28.0 / 376.0, 166.0 / 245.0, 35.0 / 376.0, 44.0 / 245.0, 0.8);
	battle.particle.sprites_wave[1] = SheetSprite(mettennaTexture, 68.0 / 376.0, 166.0 / 245.0, 35.0 / 376.0, 44.0 / 245.0, 0.8);
	battle.particle.sprites_wave[2] = SheetSprite(mettennaTexture, 112.0 / 376.0, 166.0 / 245.0, 35.0 / 376.0, 44.0 / 245.0, 0.8);
	battle.particle.sprites_wave[3] = SheetSprite(mettennaTexture, 157.0 / 376.0, 166.0 / 245.0, 35.0 / 376.0, 44.0 / 245.0, 0.8);
	battle.particle.sprites_wave[4] = SheetSprite(mettennaTexture, 203.0 / 376.0, 166.0 / 245.0, 35.0 / 376.0, 44.0 / 245.0, 0.8);
	battle.particle.sprites_wave[5] = SheetSprite(mettennaTexture, 251.0 / 376.0, 166.0 / 245.0, 35.0 / 376.0, 44.0 / 245.0, 0.8);
	battle.particle.sprites_wave[6] = SheetSprite(mettennaTexture, 297.0 / 376.0, 166.0 / 245.0, 35.0 / 376.0, 44.0 / 245.0, 0.8);
		
	//Player
	GLuint playerTextureB = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/Rockman_exeOSS_battle.png");
	//idle
	battle.player.sprites_idle = SheetSprite(playerTextureB, 85.0 / 350.0, 6.0 / 1000.0, 35.0 / 350.0, 50 / 1000.0, 0.5);
	//shoot
	battle.player.sprites_shoot[0] = SheetSprite(playerTextureB, 41.0 / 350.0, 136.0 / 1000.0, 50.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle.player.sprites_shoot[1] = SheetSprite(playerTextureB, 95.0 / 350.0, 136.0 / 1000.0, 70.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle.player.sprites_shoot[2] = SheetSprite(playerTextureB, 173.0 / 350.0, 136.0 / 1000.0, 70.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle.player.sprites_shoot[3] = SheetSprite(playerTextureB, 248.0 / 350.0, 136.0 / 1000.0, 70.0 / 350.0, 50.0 / 1000.0, 0.5);
	//throw
	battle.player.sprites_throw[0] = SheetSprite(playerTextureB, 40.0 / 350.0, 380.0 / 1000.0, 45.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_throw[1] = SheetSprite(playerTextureB, 90.0 / 350.0, 380.0 / 1000.0, 50.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_throw[2] = SheetSprite(playerTextureB, 147.0 / 350.0, 380.0 / 1000.0, 50.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_throw[3] = SheetSprite(playerTextureB, 202.0 / 350.0, 380.0 / 1000.0, 55.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_throw[4] = SheetSprite(playerTextureB, 266.0 / 350.0, 380.0 / 1000.0, 55.0 / 350.0, 55.0 / 1000.0, 0.5);
	//smash
	battle.player.sprites_smash[0] = SheetSprite(playerTextureB, 43.0 / 350.0, 451.0 / 1000.0, 40.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_smash[1] = SheetSprite(playerTextureB, 89.0 / 350.0, 451.0 / 1000.0, 40.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_smash[2] = SheetSprite(playerTextureB, 131.0 / 350.0, 451.0 / 1000.0, 40.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_smash[3] = SheetSprite(playerTextureB, 175.0 / 350.0, 451.0 / 1000.0, 36.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_smash[4] = SheetSprite(playerTextureB, 218.0 / 350.0, 451.0 / 1000.0, 40.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_smash[5] = SheetSprite(playerTextureB, 265.0 / 350.0, 451.0 / 1000.0, 45.0 / 350.0, 55.0 / 1000.0, 0.5);
	//hit
	battle.player.sprites_hit[0] = SheetSprite(playerTextureB, 85.0 / 350.0, 200.0 / 1000.0, 35.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle.player.sprites_hit[1] = SheetSprite(playerTextureB, 136.0 / 350.0, 200.0 / 1000.0, 40.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle.player.sprites_hit[2] = SheetSprite(playerTextureB, 183.0 / 350.0, 200.0 / 1000.0, 40.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle.player.sprites_hit[3] = SheetSprite(playerTextureB, 228.0 / 350.0, 200.0 / 1000.0, 40.0 / 350.0, 50.0 / 1000.0, 0.5);	
}

void Events(SDL_Event* event, bool& done) {
	switch (mode){
	case STATE_MAIN_MENU:
		break;
	case STATE_BATTLE_MODE:
		battle.Events(event, done);
		break;
	default:
		break;
	}
}

void Update() {
	float ticks = (float)SDL_GetTicks() / 1000.0f;
	float elapsed = ticks - lastFrameTicks;
	lastFrameTicks = ticks;
	switch (mode){
	case STATE_MAIN_MENU:
		break;
	case STATE_BATTLE_MODE:
		battle.Update(elapsed);
		break;
	default:
		break;
	}
}

void Render(ShaderProgram *program) {
	glClear(GL_COLOR_BUFFER_BIT);
	switch (mode){
	case STATE_MAIN_MENU:
		break;
	case STATE_BATTLE_MODE:
		battle.Render(program);
		break;
	default:
		break;
	}
}

void BattleMode::Events(SDL_Event* event, bool& done) {
	while (SDL_PollEvent(event)) {
		if (event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		if (event->type == SDL_KEYDOWN && player.objmode == player.MODE_IDLE) {
			//player controls
			//move or use card
			if (event->key.keysym.scancode == SDL_SCANCODE_RIGHT) { 
				if (playerx < 2) {
					playerx += 1;
				}
			}
			else if (event->key.keysym.scancode == SDL_SCANCODE_LEFT) {
				if (playerx > 0) {
					playerx -= 1;
					start = true;
				}
			}
			if (event->key.keysym.scancode == SDL_SCANCODE_UP) {
				if (playery > 0) {
					playery -= 1;
				}
			}
			else if (event->key.keysym.scancode == SDL_SCANCODE_DOWN) {
				if (playery < 2) {
					playery += 1;
				}
			}
			if (event->key.keysym.scancode == SDL_SCANCODE_Z) {
				player.objmode = Object::MODE_ATTACK;
				player.attack = Object::MODE_NORM_SHOOT;
				CollisionCheck();
			}
			if (event->key.keysym.scancode == SDL_SCANCODE_X) {
				player.objmode = Object::MODE_ATTACK;
				player.attack = Object::MODE_SMASH;
				particle.pType = Particles::TYPE_WAVE;
				particle.arenax = playerx + 1;
				particle.arenay = playery;
			}
			if (event->key.keysym.scancode == SDL_SCANCODE_SPACE) {
				player.objmode = player.MODE_HIT;
				player.HP -= 10;

			}
		}
	}
}
void BattleMode::Update(float elapsed) {
	enemy1.moveTimer += elapsed;
	if (!enemy1.dead) {
		enemy1.position.x = battle.arena[battle.enemyx][battle.enemyy].first;
		enemy1.position.y = battle.arena[battle.enemyx][battle.enemyy].second;
		enemy1.Update(elapsed);
	}

	player.position.x = arena[playerx][playery].first;
	player.position.y = arena[playerx][playery].second;
	player.Update(elapsed);
	if (enemy1.moveTimer >= 1.5 && enemy1.emode == Enemy::MODE_IDLE) {
		enemyy = playery;
		enemy1.moveTimer = 0.0;
	}
	if (particle.pType != Particles::TYPE_IDLE) {
		particle.Update(elapsed);
	}
	}
void BattleMode::Render(ShaderProgram *program) {
	glClearColor(0 / 255.0, 30 / 255.0, 79 / 255.0, 1.0f);
	viewMatrix.Identity();
	viewMatrix.Translate(-3.55, 0.6, 0.0);
	program->SetViewMatrix(viewMatrix);

	map.Draw(program);

	if (particle.pType != Particles::TYPE_IDLE ) {
		particle.Draw(program);
	}
		
	if (!player.dead) {
		player.Draw(program);
	}
	if (!enemy1.dead) {
		enemy1.Draw(program);
	}
	
	//Text
	PlayerHP.position.x = 0.1;
	PlayerHP.position.y = 1.3;
	PlayerHP.Draw(program, std::to_string(player.HP), 0.4f, -0.15f);
}