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
#include <stdlib.h>   
#include <time.h>
#include <string>
SDL_Window* displayWindow;

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
float accumulator = 0.0f;
/*
player vs player
MMSF vs MMBN

Mix_VolumeMusic(30); 
Mix_VolumeChunk(shootSound, 10);
*/

//functions
void Setup();
void Events(SDL_Event* event, bool& done);
void Update(float elapsed);
void Render(ShaderProgram *program);
GLuint LoadTexture(const char *filePath);
void DrawSprite(ShaderProgram *program, int index, int spriteCountX, int spriteCountY);
float mapValue(float value, float srcMin, float srcMax, float dstMin, float dstMax) {
	float retVal = dstMin + ((value - srcMin) / (srcMax - srcMin) * (dstMax - dstMin));
	if (retVal < dstMin) {
		retVal = dstMin;
	}
	if (retVal > dstMax) {
		retVal = dstMax;
	}
	return retVal;
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
class BattleMap {
public:
	BattleMap() {}
	void Draw(ShaderProgram *program);
	Matrix model;
};
class WorldMap {
public:
	WorldMap() {}
	void Draw(ShaderProgram *program);
	Matrix model;
};
//Text
GLuint fontTexture;
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

//WORLD CLASS
class WorldObject {
public:
	WorldObject() {
		scale.x = 1.0;
		scale.y = 1.0;
		scale.z = 1.0;
	}
	WorldObject(float x, float y, float z) : position(x, y, z) {
		scale.x = 1.0;
		scale.y = 1.0;
		scale.z = 1.0;
	}
	Matrix modelMatrix;
	Vector3 position;
	Vector3 scale;

	enum MoveDir { MODE_IDLE, MODE_UP, MODE_DOWN, MODE_LEFT, MODE_RIGHT };
	MoveDir movement = MODE_IDLE;
	int gridX;
	int gridY;

	SheetSprite sprites_idle;
	int spriteIndex = 0;
	int numFrames;
	float framesPerSecond;
	float animationElapsed = 0.0f;
};
class PlayerW : public WorldObject {
public:
	PlayerW() {
		scale.x = 1.0;
		scale.y = 2.0;
	}
	int direction = 0;
	void Update(float elapsed);
	void Draw(ShaderProgram *program);
	GLuint texture;
	Vector3 velocity;

	//Sprites
	SheetSprite sprites_idle[4];
	SheetSprite sprites_up[6];
	SheetSprite sprites_down[6];
	SheetSprite sprites_left[6];
	SheetSprite sprites_right[6];
};
class Enemy : public WorldObject {
public:
	Enemy() {
		scale.x = 1.0;
		scale.y = 2.0;
	}
	void Update(float elapsed);
	void Draw(ShaderProgram* program) {
		modelMatrix.Identity();
		modelMatrix.Translate(position.x, position.y, position.z);
		modelMatrix.Scale(scale.x, scale.y, scale.z);
		program->SetModelMatrix(modelMatrix);
		sprites_idle.Draw(program);
	}
	bool dead = false;
	bool active = true;
};
//BATTLE CLASS
//Cards
class Card {
public:
	Card() {
		scale.x = 0.5;
		scale.y = 0.5;
	}
	void Update(float elapsed){

	}
	void Draw(ShaderProgram* program) {
		model.Identity();
		model.Translate(position.x, position.y, position.z);
		model.Scale(scale.x, scale.y, 1.0);
		program->SetModelMatrix(model);
		glBindTexture(GL_TEXTURE_2D, texture[sprite]);

		float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program->positionAttribute);
		float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
		glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
		glEnableVertexAttribArray(program->texCoordAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
		glDisableVertexAttribArray(program->texCoordAttribute);
	}

	Matrix model;
	Vector3 position;
	Vector3 scale;
	GLuint texture[3];
	int sprite;
	bool used = false;
};
class Particle {
public:
	Particle() {
		scale.x = -1.0;
		scale.y = 1.0;
		scale.z = 1.0;

	}
	void CollisionCheck();
	Particle& operator=(Particle& rhs) {
		for (int i = 0; i < 7; ++i) {
			sprites_wave[i] = rhs.sprites_wave[i];
		}
		for (int i = 0; i < 3; ++i) {
			sprites_explode[i] = rhs.sprites_explode[i];
		}
		return *this;
	}
	void Update(float elapsed);
	void Draw(ShaderProgram *program);

	Matrix model;
	Vector3 position;
	Vector3 scale;
	int gridX;
	int gridY;
	std::pair<int, int> index;
	int direction;

	enum ParticleType { TYPE_IDLE, TYPE_WAVE, TYPE_EXPLODE };
	ParticleType pType = TYPE_IDLE;
	int spriteIndex = 0;
	int numFrames;
	float animationElapsed = 0.0f;
	float framesPerSecond;

	SheetSprite sprites_wave[7];
	SheetSprite sprites_explode[3];

};
class BattleObject {
public:
	BattleObject() {}
	BattleObject(float x, float y, float z) : position(x, y, z) {
		scale.x = 1.0;
		scale.y = 1.0;
		scale.z = 1.0;
	}
	Matrix modelMatrix;
	Vector3 position;
	Vector3 scale;

	enum ObjMode { MODE_IDLE, MODE_ATTACK, MODE_HIT };
	ObjMode objmode = MODE_IDLE;
	enum AttackMode { MODE_NORM_SHOOT, MODE_SHOOT, MODE_WAVE, MODE_THROW, MODE_SLAM };

	Particle particle;
	int gridX;
	int gridY;
	
	int spriteIndex = 0;
	int numFrames;
	float framesPerSecond;
	float animationElapsed = 0.0f;

	int HP;
	Text HPBar;
	bool dead = true;
	AttackMode attack;
};
bool SortRender(BattleObject* abc, BattleObject* def);
//Player
class PlayerB1 : public BattleObject {
public:
	PlayerB1() : BattleObject() {
		velocity.x = 5;
		velocity.y = 0.5;
		scale.x = 1.0;
		scale.y = 2.6;
		HP = 100;
		attack = MODE_NORM_SHOOT;
	}
	void Update(float elapsed);
	void Draw(ShaderProgram *program);
	void Attack();
	void ActivateCards() {
		for (Card& card : cards) {
			card.sprite = rand() % 3;
			card.used = false;
		}
		cardIndex = 0;
	}
	void UseCard() {
		if (cardIndex < 5) {
			int power = cards[cardIndex].sprite;
			cards[cardIndex].used = true;
			if (power == 0 || power == 2) {
				attack = BattleObject::MODE_WAVE;
			}
			if (power == 1) {
				attack = BattleObject::MODE_SHOOT;
			}
			/*if (power == 2) {
				attack = BattleObject::MODE_SLAM;
			}*/
			cardIndex++;
		}
		else {
			objmode = BattleObject::MODE_IDLE;
		}
	}
	GLuint texture;
	Vector3 velocity;
	float speed;

	Card cards[5];
	int cardIndex = 0;

	//Sprites
	SheetSprite sprites_idle;
	SheetSprite sprites_shoot[4];
	SheetSprite sprites_wave[6];
	SheetSprite sprites_slam[5];
	SheetSprite sprites_hit[5];
};
class Player1B2 : public BattleObject {
public:
	Player1B2() : BattleObject() {
		velocity.x = 5;
		velocity.y = 0.5;
		scale.x = 1.0;
		scale.y = 2.6;
		HP = 100;
		attack = MODE_NORM_SHOOT;
	}
	void Update(float elapsed);
	void Draw(ShaderProgram *program);
	void Attack();
	void ActivateCards() {
		for (Card& card : cards) {
			card.sprite = rand() % 3;
		}
		cardIndex = 0;
	}
	void UseCard() {
		if (cardIndex < 5) {
			int power = cards[cardIndex].sprite;
			cards[cardIndex].used = true;
			if (power == 0) {
				attack = BattleObject::MODE_WAVE;
			}
			if (power == 1) {
				attack = BattleObject::MODE_SHOOT;
			}
			if (power == 2) {
				attack = BattleObject::MODE_SLAM;
			}
			cardIndex++;
		}
		else {
			objmode = BattleObject::MODE_IDLE;
		}
	}
	float speed;
	GLuint texture;
	Vector3 velocity;

	Card cards[5];
	int cardIndex = 0;

	//Sprites
	SheetSprite sprites_idle;
	SheetSprite sprites_shoot[4];
	SheetSprite sprites_wave[6];
	SheetSprite sprites_slam[5];
	SheetSprite sprites_hit[5];
};
class Player2B2 : public BattleObject {
public:
	Player2B2() : BattleObject() {
		velocity.x = 5;
		velocity.y = 0.5;
		scale.x = 1.0;
		scale.y = 2.6;
		HP = 100;
		attack = MODE_NORM_SHOOT;
	}
	void Update(float elapsed);
	void Draw(ShaderProgram *program);
	void Attack();
	void ActivateCards() {
		for (Card& card : cards) {
			card.sprite = rand() % 3;
		}
		cardIndex = 0;
	}
	void UseCard() {
		if (cardIndex < 5) {
			int power = cards[cardIndex].sprite;
			cards[cardIndex].used = true;
			if (power == 0) {
				attack = BattleObject::MODE_WAVE;
			}
			if (power == 1) {
				attack = BattleObject::MODE_SHOOT;
			}
			if (power == 2) {
				attack = BattleObject::MODE_SLAM;
			}
			cardIndex++;
		}
		else {
			objmode = BattleObject::MODE_IDLE;
		}
	}
	GLuint texture;
	Vector3 velocity;
	float speed;

	Card cards[5];
	int cardIndex = 0;

	//Sprites
	SheetSprite sprites_idle;
	SheetSprite sprites_shoot[4];
	SheetSprite sprites_wave[6];
	SheetSprite sprites_slam[5];
	SheetSprite sprites_hit[5];
};
//ENEMIES
class BattleEnemy : public BattleObject {
public:
	BattleEnemy() : BattleObject() {
		scale.x = 1.0;
		scale.y = 1.0;
	}

	BattleEnemy& operator=(BattleEnemy& rhs){
		for (SheetSprite* sprite : rhs.sprites_idle) {
			sprites_idle.push_back(sprite);
		}
		for (SheetSprite* sprite : rhs.sprites_attack) {
			sprites_attack.push_back(sprite);
		}
		return *this;
	}
	
	virtual void Update(float elapsed) = 0;
	virtual void Draw(ShaderProgram *program) = 0;
	virtual void Reset() = 0;
	std::vector <SheetSprite*> sprites_idle;
	std::vector <SheetSprite*> sprites_attack;
	float attackTimer = 0.0;
};
class Mettenna : public BattleEnemy {
public:
	void Reset() {
		scale.x = 2.0;
		scale.y = 1.5;
		HP = 200;
		attack = MODE_WAVE;
	}
	void Update(float elapsed);
	void Draw(ShaderProgram *program) {
		modelMatrix.Identity();
		modelMatrix.Translate(position.x, position.y, position.z);
		modelMatrix.Scale(scale.x, scale.y, scale.z);
		program->SetModelMatrix(modelMatrix);
		switch (objmode)
		{
		case Mettenna::MODE_IDLE:
			numFrames = sprites_attack.size() - 1;
			framesPerSecond = 15.0;
			spriteIndex = 0;
			position.y += 1;
			sprites_idle[spriteIndex]->Draw(program);
			break;
		case Mettenna::MODE_ATTACK:
			numFrames = sprites_attack.size() - 1;
			framesPerSecond = 15.0;
			sprites_attack[spriteIndex]->Draw(program);
			break;
		case Mettenna::MODE_HIT:
			if (spriteIndex != 0){
				sprites_attack[spriteIndex]->Draw(program);
			}
			else {
				position.y += 1;
				sprites_idle[spriteIndex]->Draw(program);
			}
			break;
		}
		HPBar.Draw(program, std::to_string(HP), 0.2f, -0.1f);
		if (particle.pType != Particle::TYPE_IDLE) {
			particle.direction = -1;
			particle.Draw(program);
		}
	}
	bool active = false;
	float moveTimer = 0.0;
};
class Zoomer : public BattleEnemy {
public:
	void Reset() {
		scale.x = 2.0;
		scale.y = 1.5;
		HP = 200;
		attack = MODE_WAVE;
	}
	void Attack();
	void Update(float elapsed);
	void Draw(ShaderProgram *program) {
		modelMatrix.Identity();
		modelMatrix.Translate(position.x, position.y, position.z);
		modelMatrix.Scale(scale.x, scale.y, scale.z);
		program->SetModelMatrix(modelMatrix);
		switch (objmode)
		{
		case Zoomer::MODE_IDLE:
			numFrames = sprites_idle.size() - 1;
			framesPerSecond = 5.0;
			speed = 0.0;
			sprites_idle[spriteIndex]->Draw(program);
			break;
		case Zoomer::MODE_ATTACK:
			numFrames = sprites_attack.size() - 1;
			framesPerSecond = 15.0;
			speed = 0.06;
			sprites_attack[spriteIndex]->Draw(program);
			break;
		case Zoomer::MODE_HIT:
			numFrames = sprites_idle.size() - 1;
			framesPerSecond = 5.0;
			speed = 0.0;
			sprites_idle[spriteIndex]->Draw(program);
			break;
		}
		HPBar.Draw(program, std::to_string(HP), 0.2f, -0.1f);
	}
	float speed;
	float moveTimer = 0.0;
	float moveTime;
};
class Vulcan : public BattleEnemy {
public:
	void Reset() {
		scale.x = 2.0;
		scale.y = 1.5;
		HP = 200;
		attack = MODE_WAVE;
	}
	void Update(float elapsed);
	void Attack();
	void Draw(ShaderProgram *program) {
		modelMatrix.Identity();
		modelMatrix.Translate(position.x, position.y, position.z);
		modelMatrix.Scale(scale.x, scale.y, scale.z);
		program->SetModelMatrix(modelMatrix);
		switch (objmode)
		{
		case Vulcan::MODE_IDLE:
			position.y += 1;
			sprites_idle[spriteIndex]->Draw(program);
			break;
		case Vulcan::MODE_ATTACK:
			numFrames = sprites_attack.size() - 1;
			framesPerSecond = 15.0;
			sprites_attack[spriteIndex]->Draw(program);
			break;
		case Vulcan::MODE_HIT:
			break;
		}
		HPBar.Draw(program, std::to_string(HP), 0.2f, -0.1f);
	}
	int attacks = 0;
};

//STATE/MODES
enum GameMode { STATE_MAIN_MENU, STATE_OVERWORLD, STATE_BATTLE_MODE, STATE_BATTLE_MODE_2, STATE_GAME_OVER };
GameMode mode = STATE_MAIN_MENU;
class MainMenu {
public:
	class Object{
	public:
		Object() {
			scale.x = 1.0;
			scale.y = 1.0;
		}
		void Draw(ShaderProgram* program);
		Matrix model;
		Vector3 position;
		Vector3 scale;
		GLuint texture;
	};
	void Events(SDL_Event* event, bool& done);
	void Update(float elapsed);
	void Render(ShaderProgram *program);

	Object Background;
	Object Logo;
	Text Start;
};
class GameOver {
public:
	class Object {
	public:
		Object() {
			scale.x = 1.0;
			scale.y = 1.0;
		}
		void Draw(ShaderProgram* program);
		Matrix model;
		Vector3 position;
		Vector3 scale;
		GLuint texture;
	};
	void Events(SDL_Event* event, bool& done);
	void Update(float elapsed);
	void Render(ShaderProgram *program);

	Object Background;
	Text Start;
};
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
				if (x == 0) { arena[x][y].first = 0.6; }
				if (x == 1) { arena[x][y].first = 1.8; }
				if (x == 2) { arena[x][y].first = 3.0; }
				if (x == 3) { arena[x][y].first = 4.2; }
				if (x == 4) { arena[x][y].first = 5.4; }
				if (x == 5) { arena[x][y].first = 6.6; }
				if (y == 0) { arena[x][y].second = 0.2; }
				if (y == 1) { arena[x][y].second = -0.6; }
				if (y == 2) { arena[x][y].second = -1.3; }
			}
		}
	}
	void Setup();
	void Events(SDL_Event* event, bool& done);
	void Update(float elapsed);
	void Render(ShaderProgram *program);
	void DrawMap(ShaderProgram *program);
	void ResetMap() {
		for (int x = 0; x < 6; ++x) {
			for (int y = 0; y < 4; ++y) {
				arenaMap[x][y] = nullptr;
			}
		}
	}
	void UpdateMap() {
		ResetMap();
		for (BattleEnemy* enemy : enemies) {
			if (enemy != nullptr) { arenaMap[enemy->gridX][enemy->gridY] = enemy; }
		}
	}
	
	~BattleMode() {
		for (BattleEnemy* enemy : enemies) {
			delete enemy;
		}
		for (Mix_Chunk* sfx : SFX) {
			Mix_FreeChunk(sfx);
		}
		for (Mix_Music* song : music) {
			Mix_FreeMusic(song);
		}
	}

	//MAP
	BattleMap map;
	GLuint mapTexture;
	FlareMap fmap;
	float TILE_SIZE = 0.3;
	std::pair<float,float> arena[6][3];
	BattleEnemy* arenaMap[6][3];
	std::vector<BattleObject*> sorts;
	//PLAYER
	PlayerB1 player;

	//ENEMIES
	BattleEnemy* enemies[3];
	Mettenna metas[3];
	Vulcan vulcs[3];
	Zoomer zooms[3];
	std::vector<BattleObject*> sort_objs; //sort for render
	int c_index = 0;
	float time;
	int meta_active = 0;

	//SOUND
	Mix_Music* music[5];
	Mix_Chunk* SFX[4];

};
class BattleMode2 {
public:
	BattleMode2() {
		/*
		Row1:	(0.6,0.2)	(1.8,0.2)	(3.0,0.2)	|	(4.2,0.2)	(5.4,0.2)	(6.6,0.2)
		Row2:	(0.6,-0.6)	(1.8,-0.6)	(3.0,-0.6)	|	(4.2,-0.6)	(5.4,-0.6)	(6.6,-0.6)
		Row3:	(0.6,-1.3)	(1.8,-1.3)	(3.0,-1.3)	|	(4.2,-1.3)	(5.4,-1.3)	(6.6,-1.3)
		*/
		for (int x = 0; x < 6; ++x) {
			for (int y = 0; y < 4; ++y) {
				if (x == 0) { arena[x][y].first = 0.6; }
				if (x == 1) { arena[x][y].first = 1.8; }
				if (x == 2) { arena[x][y].first = 3.0; }
				if (x == 3) { arena[x][y].first = 4.2; }
				if (x == 4) { arena[x][y].first = 5.4; }
				if (x == 5) { arena[x][y].first = 6.6; }
				if (y == 0) { arena[x][y].second = 0.2; }
				if (y == 1) { arena[x][y].second = -0.6; }
				if (y == 2) { arena[x][y].second = -1.3; }
			}
		}
	}

	void Setup();
	void Events(SDL_Event* event, bool& done);
	void Update(float elapsed);
	void Render(ShaderProgram *program);
	void DrawMap(ShaderProgram *program);
	~BattleMode2() {
		for (Mix_Chunk* sfx : SFX) {
			Mix_FreeChunk(sfx);
		}
		Mix_FreeMusic(music);
	}

	//MAP
	BattleMap map;
	GLuint mapTexture;
	FlareMap fmap;
	float TILE_SIZE = 0.3;
	std::pair<float, float> arena[6][3];
	bool arenaMap[6][3];

	//PLAYER
	Player1B2 player;
	Player2B2 player2;
	
	std::vector<BattleObject*> sort_objs; //sort for render
	int c_index = 0;
	float time;
	int meta_active = 0;

	//SOUND
	Mix_Music* music;
	Mix_Chunk* SFX[4];

};
class Overworld {
public:
	Overworld() {}

	void Setup();
	void Events(SDL_Event* event, bool& done);
	void Update(float elapsed);
	void Render(ShaderProgram *program);
	void DrawMap(ShaderProgram *program);
	void PlaceEntity(std::string& type, float x, float y) {
		if (type == "Player1") {
			player1.position.x = (x - y) / 2.08;
			player1.position.y = (x + y) / -3.0;
		}
		if (type == "Enemy") {
			enemies[c_index].position.x = (x - y) / 2.08;
			enemies[c_index].position.y = (x + y) / -3.0;
			c_index++;
		}

		if (type == "Player2") {
			player2.position.x = (x - y) / 2.0;
			player2.position.y = (x + y) / -3.0;
		}
	}
	void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
		*gridX = (worldX  + (-1 * worldY)) * 0.25;
		*gridY = ((-1 * worldY ) - worldX) * 0.25;
	}
	~Overworld() {
		for (Mix_Chunk* sfx : SFX) {
			Mix_FreeChunk(sfx);
		}
		Mix_FreeMusic(music);
	}

	//MAP
	WorldMap map;
	GLuint mapTexture;
	FlareMap fmap;

	//PLAYER
	PlayerW player1;
	PlayerW player2;

	//ENEMIES
	Enemy enemies[10];
	int c_index = 0;

	std::vector<BattleObject*> sort_objs; //sort for render
	float TILE_SIZE = 1.0;

	//SOUND
	Mix_Music* music;
	Mix_Chunk* SFX[4];

};
//VARIABLES
MainMenu menu;
GameOver gameOver;
BattleMode battle;
BattleMode2 battle2P;
Overworld world;
float lastFrameTicks = 0.0f;
Matrix projectionMatrix;
Matrix viewMatrix;

int main(int argc, char *argv[]) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	displayWindow = SDL_CreateWindow("MM BattleX", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
	glewInit();
#endif

	//Setup
	srand(time(NULL));
	Setup();
	ShaderProgram program;
	program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
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
		float ticks = (float)SDL_GetTicks() / 1000.0f;
		float elapsed = ticks - lastFrameTicks;
		lastFrameTicks = ticks;
		elapsed += accumulator;
		if (elapsed < FIXED_TIMESTEP) {
			accumulator = elapsed;
			continue;
		}
		while (elapsed >= FIXED_TIMESTEP) {
			Update(FIXED_TIMESTEP);
			elapsed -= FIXED_TIMESTEP;
		}
		accumulator = elapsed;
		Render(&program);
		//End Drawing
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
//Functions
bool SortRender(BattleObject* abc, BattleObject* def) {
	return (abc->gridY <= def->gridY);
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
//Main Functions
void Setup() {
	glViewport(0, 0, 640, 360);
	menu.Logo.texture = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/GameLogo.png");
	gameOver.Background.texture = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/GameOver.png");

	fontTexture = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/font1.png");

	//WORLD
	world.mapTexture = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/WorldTiles.png");
	world.fmap.Load(RESOURCE_FOLDER"Assets/WorldMap.txt");
	for (int i = 0; i < world.fmap.entities.size(); ++i) {
		world.PlaceEntity(world.fmap.entities[i].type, world.fmap.entities[i].x, world.fmap.entities[i].y);
	}
	//Player
	GLuint playerTextureA = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/Rockman_RnR3.png");
	//idle
	world.player1.sprites_idle[0] = SheetSprite(playerTextureA, 434.0 / 840.0, 571.0 / 1864.0, 17.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_idle[1] = SheetSprite(playerTextureA, 349.0 / 840.0, 571.0 / 1864.0, 17.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_idle[2] = SheetSprite(playerTextureA, 516.0 / 840.0, 571.0 / 1864.0, 17.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_idle[3] = SheetSprite(playerTextureA, 289.0 / 840.0, 571.0 / 1864.0, 17.0 / 840.0, 41.0 / 1864.0, 0.5);
	//up
	world.player1.sprites_up[0] = SheetSprite(playerTextureA, 260.0 / 840.0, 717.0 / 1864.0, 25.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_up[1] = SheetSprite(playerTextureA, 292.0 / 840.0, 717.0 / 1864.0, 25.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_up[2] = SheetSprite(playerTextureA, 319.0 / 840.0, 717.0 / 1864.0, 22.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_up[3] = SheetSprite(playerTextureA, 342.0 / 840.0, 717.0 / 1864.0, 22.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_up[4] = SheetSprite(playerTextureA, 367.0 / 840.0, 717.0 / 1864.0, 25.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_up[5] = SheetSprite(playerTextureA, 392.0 / 840.0, 717.0 / 1864.0, 25.0 / 840.0, 41.0 / 1864.0, 0.5);
	//down
	world.player1.sprites_down[0] = SheetSprite(playerTextureA, 257.0 / 840.0, 813.0 / 1864.0, 25.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_down[1] = SheetSprite(playerTextureA, 282.0 / 840.0, 813.0 / 1864.0, 25.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_down[2] = SheetSprite(playerTextureA, 308.0 / 840.0, 813.0 / 1864.0, 27.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_down[3] = SheetSprite(playerTextureA, 337.0 / 840.0, 813.0 / 1864.0, 25.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_down[4] = SheetSprite(playerTextureA, 364.0 / 840.0, 813.0 / 1864.0, 25.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_down[5] = SheetSprite(playerTextureA, 390.0 / 840.0, 813.0 / 1864.0, 25.0 / 840.0, 41.0 / 1864.0, 0.5);
	//left
	world.player1.sprites_left[0] = SheetSprite(playerTextureA, 269.0 / 840.0, 861.0 / 1864.0, 25.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_left[1] = SheetSprite(playerTextureA, 295.0 / 840.0, 861.0 / 1864.0, 17.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_left[2] = SheetSprite(playerTextureA, 319.0 / 840.0, 861.0 / 1864.0, 25.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_left[3] = SheetSprite(playerTextureA, 341.0 / 840.0, 861.0 / 1864.0, 22.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_left[4] = SheetSprite(playerTextureA, 367.0 / 840.0, 861.0 / 1864.0, 22.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_left[5] = SheetSprite(playerTextureA, 391.0 / 840.0, 861.0 / 1864.0, 17.0 / 840.0, 41.0 / 1864.0, 0.5);
	//right
	world.player1.sprites_right[0] = SheetSprite(playerTextureA, 264.0 / 840.0, 767.0 / 1864.0, 25.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_right[1] = SheetSprite(playerTextureA, 292.0 / 840.0, 767.0 / 1864.0, 25.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_right[2] = SheetSprite(playerTextureA, 316.0 / 840.0, 767.0 / 1864.0, 17.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_right[3] = SheetSprite(playerTextureA, 336.0 / 840.0, 767.0 / 1864.0, 25.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_right[4] = SheetSprite(playerTextureA, 364.0 / 840.0, 767.0 / 1864.0, 25.0 / 840.0, 41.0 / 1864.0, 0.5);
	world.player1.sprites_right[5] = SheetSprite(playerTextureA, 391.0 / 840.0, 767.0 / 1864.0, 20.0 / 840.0, 41.0 / 1864.0, 0.5);

	//Player2
	GLuint jammerTexture = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/HeelFM.png");
	world.player2.sprites_idle[0] = SheetSprite(playerTextureA, 359.0 / 840.0, 1279.0 / 1864.0, 28.0 / 840.0, 58.0 / 1864.0, 0.5);

	//Enemies
	for (Enemy& enemy : world.enemies) {
		enemy.sprites_idle = SheetSprite(jammerTexture, 82.0 / 286.0, 8.0 / 685.0, 25.0 / 286.0, 55.0 / 685.0, 0.5);
	}

	//BATTLE
	//MAP
	battle.mapTexture = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/bn4_panels.png");
	battle.fmap.Load(RESOURCE_FOLDER"Assets/BattleMap.txt");

	//Enemies
	//Mettenna
	GLuint mettennaTexture = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/Mettaur.png");
	//idle
	battle.metas[0].sprites_idle.push_back(new SheetSprite(mettennaTexture, 43.0 / 376.0, 4.0 / 245.0, 22.0 / 376.0, 46.0 / 245.0, 0.8));
	//attack
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 71.0 / 376.0, 4.0 / 245.0, 28.0 / 376.0, 46.0 / 245.0, 0.8));
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 105.0 / 376.0, 4.0 / 245.0, 28.0 / 376.0, 46.0 / 245.0, 0.8));
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 137.0 / 376.0, 4.0 / 245.0, 28.0 / 376.0, 46.0 / 245.0, 0.8));
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 169.0 / 376.0, 4.0 / 245.0, 28.0 / 376.0, 46.0 / 245.0, 0.8));
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 236.0 / 376.0, 4.0 / 245.0, 28.0 / 376.0, 46.0 / 245.0, 0.8));
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 273.0 / 376.0, 4.0 / 245.0, 32.0 / 376.0, 46.0 / 245.0, 0.8));
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 313.0 / 376.0, 4.0 / 245.0, 38.0 / 376.0, 46.0 / 245.0, 0.8));
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 4.0 / 376.0, 58.0 / 245.0, 38.0 / 376.0, 53.0 / 245.0, 0.8));
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 47.0 / 376.0, 58.0 / 245.0, 38.0 / 376.0, 53.0 / 245.0, 0.8));
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 94.0 / 376.0, 58.0 / 245.0, 38.0 / 376.0, 53.0 / 245.0, 0.8));
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 141.0 / 376.0, 58.0 / 245.0, 38.0 / 376.0, 53.0 / 245.0, 0.8));
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 183.0 / 376.0, 58.0 / 245.0, 28.0 / 376.0, 53.0 / 245.0, 0.8));
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 227.0 / 376.0, 58.0 / 245.0, 28.0 / 376.0, 53.0 / 245.0, 0.8));
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 271.0 / 376.0, 58.0 / 245.0, 28.0 / 376.0, 53.0 / 245.0, 0.8));
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 309.0 / 376.0, 58.0 / 245.0, 28.0 / 376.0, 53.0 / 245.0, 0.8));
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 314.0 / 376.0, 58.0 / 245.0, 28.0 / 376.0, 53.0 / 245.0, 0.8));
	battle.metas[0].sprites_attack.push_back(new SheetSprite(mettennaTexture, 350.0 / 376.0, 58.0 / 245.0, 28.0 / 376.0, 53.0 / 245.0, 0.8));
	battle.metas[1] = battle.metas[0];
	battle.metas[2] = battle.metas[0];

	//Zoomers
	GLuint zoomerTexture = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/Fishy.png");
	//idle
	battle.zooms[0].sprites_idle.push_back(new SheetSprite(zoomerTexture, 0.0 / 280.0, 3.0 / 157.0, 70.0 / 280.0, 46.0 / 157.0, 0.6));
	battle.zooms[0].sprites_idle.push_back(new SheetSprite(zoomerTexture, 70.0 / 280.0, 3.0 / 157.0, 70.0 / 280.0, 46.0 / 157.0, 0.6));
	battle.zooms[0].sprites_idle.push_back(new SheetSprite(zoomerTexture, 140.0 / 280.0, 3.0 / 157.0, 70.0 / 280.0, 46.0 / 157.0, 0.6));
	battle.zooms[0].sprites_idle.push_back(new SheetSprite(zoomerTexture, 210.0 / 280.0, 3.0 / 157.0, 70.0 / 280.0, 46.0 / 157.0, 0.6));
	//attack
	battle.zooms[0].sprites_attack.push_back(new SheetSprite(zoomerTexture, 0.0 / 280.0, 58.0 / 157.0, 70.0 / 280.0, 46.0 / 157.0, 0.6));
	battle.zooms[0].sprites_attack.push_back(new SheetSprite(zoomerTexture, 70.0 / 280.0, 58.0 / 157.0, 70.0 / 280.0, 46.0 / 157.0, 0.6));
	battle.zooms[0].sprites_attack.push_back(new SheetSprite(zoomerTexture, 140.0 / 280.0, 58.0 / 157.0, 70.0 / 280.0, 46.0 / 157.0, 0.6));
	battle.zooms[0].sprites_attack.push_back(new SheetSprite(zoomerTexture, 210.0 / 280.0, 58.0 / 157.0, 70.0 / 280.0, 46.0 / 157.0, 0.6));

	battle.zooms[1] = battle.zooms[0];
	battle.zooms[2] = battle.zooms[0];
	
	//Vulcans
	GLuint vulcanTexture = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/Gunner.png");
	//idle
	battle.vulcs[0].sprites_idle.push_back(new SheetSprite(vulcanTexture, 30.0 / 274.0, 0.0 / 308.0, 55.0 / 274.0, 56.0 / 308.0, 0.6));
	//attack
	battle.vulcs[0].sprites_attack.push_back(new SheetSprite(vulcanTexture, 86.0 / 274.0, 3.0 / 308.0, 50.0 / 274.0, 56.0 / 308.0, 0.6));
	battle.vulcs[0].sprites_attack.push_back(new SheetSprite(vulcanTexture, 138.0 / 274.0, 3.0 / 308.0, 50.0 / 274.0, 56.0 / 308.0, 0.6));
	battle.vulcs[0].sprites_attack.push_back(new SheetSprite(vulcanTexture, 190.0 / 274.0, 3.0 / 308.0, 50.0 / 274.0, 56.0 / 308.0, 0.6));
	battle.vulcs[0].sprites_attack.push_back(new SheetSprite(vulcanTexture, 0.0 / 274.0, 58.0 / 308.0, 70.0 / 274.0, 56.0 / 308.0, 0.6));
	battle.vulcs[0].sprites_attack.push_back(new SheetSprite(vulcanTexture, 7.0 / 274.0, 58.0 / 308.0, 70.0 / 274.0, 56.0 / 308.0, 0.6));
	battle.vulcs[0].sprites_attack.push_back(new SheetSprite(vulcanTexture, 59.0 / 274.0, 70.0 / 308.0, 70.0 / 274.0, 56.0 / 308.0, 0.6));
	battle.vulcs[0].sprites_attack.push_back(new SheetSprite(vulcanTexture, 130.0 / 274.0, 58.0 / 308.0, 72.0 / 274.0, 56.0 / 308.0, 0.6));
	battle.vulcs[0].sprites_attack.push_back(new SheetSprite(vulcanTexture, 204.0 / 274.0, 58.0 / 308.0, 63.0 / 274.0, 56.0 / 308.0, 0.6));
	battle.vulcs[0].sprites_attack.push_back(new SheetSprite(vulcanTexture, 7.0 / 274.0, 58.0 / 308.0, 72.0 / 274.0, 56.0 / 308.0, 0.6));
	battle.vulcs[0].sprites_attack.push_back(new SheetSprite(vulcanTexture, 59.0 / 274.0, 114.0 / 308.0, 72.0 / 274.0, 56.0 / 308.0, 0.6));
	battle.vulcs[0].sprites_attack.push_back(new SheetSprite(vulcanTexture, 130.0 / 274.0, 114.0 / 308.0, 72.0 / 274.0, 56.0 / 308.0, 0.6));
	battle.vulcs[0].sprites_attack.push_back(new SheetSprite(vulcanTexture, 204.0 / 274.0, 114.0 / 308.0, 72.0 / 274.0, 56.0 / 308.0, 0.6));

	battle.vulcs[1] = battle.vulcs[0];
	battle.vulcs[2] = battle.vulcs[0];
		
	//Particle
	//wave
	battle.player.particle.sprites_wave[0] = SheetSprite(mettennaTexture, 28.0 / 376.0, 166.0 / 245.0, 35.0 / 376.0, 44.0 / 245.0, 0.8);
	battle.player.particle.sprites_wave[1] = SheetSprite(mettennaTexture, 68.0 / 376.0, 166.0 / 245.0, 35.0 / 376.0, 44.0 / 245.0, 0.8);
	battle.player.particle.sprites_wave[2] = SheetSprite(mettennaTexture, 112.0 / 376.0, 166.0 / 245.0, 35.0 / 376.0, 44.0 / 245.0, 0.8);
	battle.player.particle.sprites_wave[3] = SheetSprite(mettennaTexture, 157.0 / 376.0, 166.0 / 245.0, 35.0 / 376.0, 44.0 / 245.0, 0.8);
	battle.player.particle.sprites_wave[4] = SheetSprite(mettennaTexture, 203.0 / 376.0, 166.0 / 245.0, 35.0 / 376.0, 44.0 / 245.0, 0.8);
	battle.player.particle.sprites_wave[5] = SheetSprite(mettennaTexture, 251.0 / 376.0, 166.0 / 245.0, 35.0 / 376.0, 44.0 / 245.0, 0.8);
	battle.player.particle.sprites_wave[6] = SheetSprite(mettennaTexture, 297.0 / 376.0, 166.0 / 245.0, 35.0 / 376.0, 44.0 / 245.0, 0.8);
	battle.player.particle.direction = 1;
	
	//Player
	GLuint playerTextureB = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/Rockman_exeOSS_battle.png");
	//idle
	battle.player.sprites_idle = SheetSprite(playerTextureB, 85.0 / 350.0, 6.0 / 1000.0, 35.0 / 350.0, 50 / 1000.0, 0.5);
	battle2P.player.sprites_idle = SheetSprite(playerTextureB, 85.0 / 350.0, 6.0 / 1000.0, 35.0 / 350.0, 50 / 1000.0, 0.5);
	//shoot
	battle.player.sprites_shoot[0] = SheetSprite(playerTextureB, 41.0 / 350.0, 136.0 / 1000.0, 50.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle2P.player.sprites_shoot[0] = SheetSprite(playerTextureB, 41.0 / 350.0, 136.0 / 1000.0, 50.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle.player.sprites_shoot[1] = SheetSprite(playerTextureB, 95.0 / 350.0, 136.0 / 1000.0, 70.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle2P.player.sprites_shoot[1] = SheetSprite(playerTextureB, 95.0 / 350.0, 136.0 / 1000.0, 70.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle.player.sprites_shoot[2] = SheetSprite(playerTextureB, 173.0 / 350.0, 136.0 / 1000.0, 70.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle2P.player.sprites_shoot[2] = SheetSprite(playerTextureB, 173.0 / 350.0, 136.0 / 1000.0, 70.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle.player.sprites_shoot[3] = SheetSprite(playerTextureB, 248.0 / 350.0, 136.0 / 1000.0, 70.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle2P.player.sprites_shoot[3] = SheetSprite(playerTextureB, 248.0 / 350.0, 136.0 / 1000.0, 70.0 / 350.0, 50.0 / 1000.0, 0.5);
	//throw
	battle.player.sprites_slam[0] = SheetSprite(playerTextureB, 40.0 / 350.0, 380.0 / 1000.0, 45.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle2P.player.sprites_slam[0] = SheetSprite(playerTextureB, 40.0 / 350.0, 380.0 / 1000.0, 45.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_slam[1] = SheetSprite(playerTextureB, 90.0 / 350.0, 380.0 / 1000.0, 50.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle2P.player.sprites_slam[1] = SheetSprite(playerTextureB, 90.0 / 350.0, 380.0 / 1000.0, 50.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_slam[2] = SheetSprite(playerTextureB, 147.0 / 350.0, 380.0 / 1000.0, 50.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle2P.player.sprites_slam[2] = SheetSprite(playerTextureB, 147.0 / 350.0, 380.0 / 1000.0, 50.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_slam[3] = SheetSprite(playerTextureB, 202.0 / 350.0, 380.0 / 1000.0, 55.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle2P.player.sprites_slam[3] = SheetSprite(playerTextureB, 202.0 / 350.0, 380.0 / 1000.0, 55.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_slam[4] = SheetSprite(playerTextureB, 266.0 / 350.0, 380.0 / 1000.0, 55.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle2P.player.sprites_slam[4] = SheetSprite(playerTextureB, 266.0 / 350.0, 380.0 / 1000.0, 55.0 / 350.0, 55.0 / 1000.0, 0.5);
	//smash
	battle.player.sprites_wave[0] = SheetSprite(playerTextureB, 43.0 / 350.0, 451.0 / 1000.0, 40.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle2P.player.sprites_wave[0] = SheetSprite(playerTextureB, 43.0 / 350.0, 451.0 / 1000.0, 40.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_wave[1] = SheetSprite(playerTextureB, 89.0 / 350.0, 451.0 / 1000.0, 40.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle2P.player.sprites_wave[1] = SheetSprite(playerTextureB, 89.0 / 350.0, 451.0 / 1000.0, 40.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_wave[2] = SheetSprite(playerTextureB, 131.0 / 350.0, 451.0 / 1000.0, 40.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle2P.player.sprites_wave[2] = SheetSprite(playerTextureB, 131.0 / 350.0, 451.0 / 1000.0, 40.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_wave[3] = SheetSprite(playerTextureB, 175.0 / 350.0, 451.0 / 1000.0, 36.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle2P.player.sprites_wave[3] = SheetSprite(playerTextureB, 175.0 / 350.0, 451.0 / 1000.0, 36.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_wave[4] = SheetSprite(playerTextureB, 218.0 / 350.0, 451.0 / 1000.0, 40.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle2P.player.sprites_wave[4] = SheetSprite(playerTextureB, 218.0 / 350.0, 451.0 / 1000.0, 40.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle.player.sprites_wave[5] = SheetSprite(playerTextureB, 265.0 / 350.0, 451.0 / 1000.0, 45.0 / 350.0, 55.0 / 1000.0, 0.5);
	battle2P.player.sprites_wave[5] = SheetSprite(playerTextureB, 265.0 / 350.0, 451.0 / 1000.0, 45.0 / 350.0, 55.0 / 1000.0, 0.5);
	//hit
	battle.player.sprites_hit[0] = SheetSprite(playerTextureB, 85.0 / 350.0, 200.0 / 1000.0, 35.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle2P.player.sprites_hit[0] = SheetSprite(playerTextureB, 85.0 / 350.0, 200.0 / 1000.0, 35.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle.player.sprites_hit[1] = SheetSprite(playerTextureB, 136.0 / 350.0, 200.0 / 1000.0, 40.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle2P.player.sprites_hit[1] = SheetSprite(playerTextureB, 136.0 / 350.0, 200.0 / 1000.0, 40.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle.player.sprites_hit[2] = SheetSprite(playerTextureB, 183.0 / 350.0, 200.0 / 1000.0, 40.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle2P.player.sprites_hit[2] = SheetSprite(playerTextureB, 183.0 / 350.0, 200.0 / 1000.0, 40.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle.player.sprites_hit[3] = SheetSprite(playerTextureB, 228.0 / 350.0, 200.0 / 1000.0, 40.0 / 350.0, 50.0 / 1000.0, 0.5);
	battle2P.player.sprites_hit[3] = SheetSprite(playerTextureB, 228.0 / 350.0, 200.0 / 1000.0, 40.0 / 350.0, 50.0 / 1000.0, 0.5);

	//Player
	GLuint player2TextureB = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/MM_EXE1_b.png");
	//idle
	battle2P.player2.sprites_idle = SheetSprite(player2TextureB, 159.0 / 450.0, 14.0 / 1020.0, 45.0 / 450.0, 42.0 / 1020.0, 0.5);
	//shoot
	battle2P.player2.sprites_shoot[0] = SheetSprite(player2TextureB, 126.0 / 450.0, 120.0 / 1020.0, 45.0 / 450.0, 50.0 / 1020.0, 0.5);
	battle2P.player2.sprites_shoot[1] = SheetSprite(player2TextureB, 178.0 / 450.0, 120.0 / 1020.0, 45.0 / 450.0, 50.0 / 1020.0, 0.5);
	battle2P.player2.sprites_shoot[2] = SheetSprite(player2TextureB, 254.0 / 450.0, 120.0 / 1020.0, 45.0 / 450.0, 50.0 / 1020.0, 0.5);
	battle2P.player2.sprites_shoot[3] = SheetSprite(player2TextureB, 330.0 / 450.0, 120.0 / 1020.0, 45.0 / 450.0, 50.0 / 1020.0, 0.5);
	//throw
	battle2P.player2.sprites_slam[0] = SheetSprite(player2TextureB, 40.0 / 450.0, 380.0 / 1020.0, 45.0 / 450.0, 55.0 / 1020.0, 0.5);
	battle2P.player2.sprites_slam[1] = SheetSprite(player2TextureB, 90.0 / 450.0, 380.0 / 1020.0, 50.0 / 450.0, 55.0 / 1020.0, 0.5);
	battle2P.player2.sprites_slam[2] = SheetSprite(player2TextureB, 147.0 / 450.0, 380.0 / 1020.0, 50.0 / 450.0, 55.0 / 1020.0, 0.5);
	battle2P.player2.sprites_slam[3] = SheetSprite(player2TextureB, 202.0 / 450.0, 380.0 / 1020.0, 55.0 / 450.0, 55.0 / 1020.0, 0.5);
	battle2P.player2.sprites_slam[4] = SheetSprite(player2TextureB, 266.0 / 450.0, 380.0 / 1020.0, 55.0 / 450.0, 55.0 / 1020.0, 0.5);
	//smash
	battle2P.player2.sprites_wave[0] = SheetSprite(player2TextureB, 114.0 / 450.0, 448.0 / 1020.0, 40.0 / 450.0, 55.0 / 1020.0, 0.5);
	battle2P.player2.sprites_wave[1] = SheetSprite(player2TextureB, 157.0 / 450.0, 451.0 / 1020.0, 40.0 / 450.0, 55.0 / 1020.0, 0.5);
	battle2P.player2.sprites_wave[2] = SheetSprite(player2TextureB, 200.0 / 450.0, 451.0 / 1020.0, 40.0 / 450.0, 55.0 / 1020.0, 0.5);
	battle2P.player2.sprites_wave[3] = SheetSprite(player2TextureB, 242.0 / 450.0, 451.0 / 1020.0, 36.0 / 450.0, 55.0 / 1020.0, 0.5);
	battle2P.player2.sprites_wave[4] = SheetSprite(player2TextureB, 278.0 / 450.0, 451.0 / 1020.0, 40.0 / 450.0, 55.0 / 1020.0, 0.5);
	battle2P.player2.sprites_wave[5] = SheetSprite(player2TextureB, 327.0 / 450.0, 451.0 / 1020.0, 45.0 / 450.0, 55.0 / 1020.0, 0.5);
	//hit
	battle2P.player2.sprites_hit[0] = SheetSprite(player2TextureB, 168.0 / 450.0, 172.0 / 1020.0, 35.0 / 450.0, 48.0 / 1020.0, 0.5);
	battle2P.player2.sprites_hit[1] = SheetSprite(player2TextureB, 136.0 / 450.0, 172.0 / 1020.0, 40.0 / 450.0, 48.0 / 1020.0, 0.5);
	battle2P.player2.sprites_hit[2] = SheetSprite(player2TextureB, 183.0 / 450.0, 172.0 / 1020.0, 40.0 / 450.0, 48.0 / 1020.0, 0.5);
	battle2P.player2.sprites_hit[3] = SheetSprite(player2TextureB, 228.0 / 450.0, 172.0 / 1020.0, 40.0 / 450.0, 48.0 / 1020.0, 0.5);
	
	battle.player.velocity.x = battle2P.player.velocity.x = 0.06;
	battle2P.player2.velocity.x = -0.06;
	for (Mettenna& meta : battle.metas) {
		meta.particle = battle.player.particle;
	}
	//Cards
	for (int i = 0; i < 5; ++i) {
		battle.player.cards[i].texture[0] = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/s031 GroundWave1.png");
		battle2P.player.cards[i].texture[0] = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/s031 GroundWave1.png");
		battle2P.player2.cards[i].texture[0] = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/s031 GroundWave1.png");
		battle.player.cards[i].texture[1] = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/008_madvulcan1.png");
		battle2P.player.cards[i].texture[1] = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/008_madvulcan1.png");
		battle2P.player2.cards[i].texture[1] = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/008_madvulcan1.png");
		battle.player.cards[i].texture[2] = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/s034 JetAttack1.png");
		battle2P.player.cards[i].texture[2] = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/s034 JetAttack1.png");
		battle2P.player2.cards[i].texture[2] = LoadTexture(RESOURCE_FOLDER"Assets/Sprites/s034 JetAttack1.png");
	}
	battle2P.Setup();
	//MUSIC
	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096); 
	//Load
	battle.music[0] = Mix_LoadMUS("Assets/Music/RideOn_MMSF.wav");
	battle.music[1] = Mix_LoadMUS("Assets/Music/WaveBattle(Boss)_MMSF2.wav");
	battle.music[2] = Mix_LoadMUS("Assets/Music/MMSF Theme.wav");
	battle.music[3] = Mix_LoadMUS("Assets/Music/MMSF FM-ian.wav");
	battle.music[4] = Mix_LoadMUS("Assets/Music/MMSF Game Over.wav");
	battle.SFX[0] = Mix_LoadWAV("Assets/Music/shoot.wav");	battle.SFX[1] = Mix_LoadWAV("Assets/Music/wave.wav");	battle.SFX[2] = Mix_LoadWAV("Assets/Music/hit.wav");	battle.SFX[3] = Mix_LoadWAV("Assets/Music/Zoom.wav");	
	Mix_VolumeMusic(30);
	Mix_VolumeChunk(battle.SFX[1], 10);
	Mix_VolumeChunk(battle.SFX[2], 50);
	Mix_PlayMusic(battle.music[2], 1);

}
void Events(SDL_Event* event, bool& done) {
	switch (mode){
	case STATE_MAIN_MENU:
		menu.Events(event, done);
		break;
	case STATE_OVERWORLD:
		world.Events(event, done);
		break;
	case STATE_BATTLE_MODE:
		battle.Events(event, done);
		break;
	case STATE_BATTLE_MODE_2:
		battle2P.Events(event, done);
		break;
	case STATE_GAME_OVER:
		gameOver.Events(event, done);
		break;
	default:
		break;
	}
}
void Update(float elapsed) {
	switch (mode){
	case STATE_MAIN_MENU:
		menu.Update(elapsed);
		break;
	case STATE_OVERWORLD:
		world.Update(elapsed);
		break; 
	case STATE_BATTLE_MODE:
		battle.Update(elapsed);
		break;
	case STATE_BATTLE_MODE_2:
		battle2P.Update(elapsed);
		break;
	case STATE_GAME_OVER:
		gameOver.Update(elapsed);
		break;
	default:
		break;
	}
}
void Render(ShaderProgram *program) {
	glClear(GL_COLOR_BUFFER_BIT);
	switch (mode){
	case STATE_MAIN_MENU:
		menu.Render(program);
		break;
	case STATE_OVERWORLD:
		world.Render(program);
		break;
	case STATE_BATTLE_MODE:
		battle.Render(program);
		break;
	case STATE_BATTLE_MODE_2:
		battle2P.Render(program);
		break;
	case STATE_GAME_OVER:
		gameOver.Render(program);
		break;
	default:
		break;
	}
}
//MainMenu Functions
void MainMenu::Events(SDL_Event* event, bool& done){
	while (SDL_PollEvent(event)) {
		if (event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		if (event->type == SDL_KEYDOWN) {
			if (event->key.keysym.scancode == SDL_SCANCODE_RSHIFT) {
				Mix_PlayMusic(battle.music[3], -1);
				battle.player.dead = false;
				world.Setup();
				mode = STATE_OVERWORLD;
			}
		}
	}
}
void MainMenu::Update(float elapsed){
	Logo.position.y = 0.5;
	Logo.scale.x = 3.0*1.5;
	Logo.scale.y = 2.0*1.5;
	Start.position.x = -0.2;
	Start.position.y = -1.0;
}
void MainMenu::Render(ShaderProgram *program){
	glClearColor(0 / 255.0, 30 / 255.0, 79 / 255.0, 1.0f);
	viewMatrix.Identity();
	program->SetViewMatrix(viewMatrix);
	Logo.Draw(program);
	Start.Draw(program, "Start", 0.4f, -0.15f);
}
void MainMenu::Object::Draw(ShaderProgram *program) {
	model.Identity();
	model.Translate(position.x, position.y, position.z);
	model.Scale(scale.x, scale.y, 1.0);
	program->SetModelMatrix(model);
	glBindTexture(GL_TEXTURE_2D, texture);

	float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}
//GameOver Functions
void GameOver::Events(SDL_Event* event, bool& done) {
	while (SDL_PollEvent(event)) {
		if (event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		if (event->type == SDL_KEYDOWN) {
			if (event->key.keysym.scancode == SDL_SCANCODE_RSHIFT) {
				Mix_PlayMusic(battle.music[2], -1);
				mode = STATE_MAIN_MENU;
			}
			if (event->key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				done = true;
			}
		}
	}
}
void GameOver::Update(float elapsed) {
	Background.scale.x = 3.55*2;
	Background.scale.y = 4.0;
	Start.position.x = -0.6;
	Start.position.y = -1.2;
}
void GameOver::Render(ShaderProgram *program) {
	glClearColor(0 / 255.0, 30 / 255.0, 79 / 255.0, 1.0f);
	viewMatrix.Identity();
	program->SetViewMatrix(viewMatrix);
	Background.Draw(program);
	Start.Draw(program, "Restart", 0.4f, -0.15f);
}
void GameOver::Object::Draw(ShaderProgram *program) {
	model.Identity();
	model.Translate(position.x, position.y, position.z);
	model.Scale(scale.x, scale.y, 1.0);
	program->SetModelMatrix(model);
	glBindTexture(GL_TEXTURE_2D, texture);

	float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
	glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
	glEnableVertexAttribArray(program->positionAttribute);
	float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
	glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
	glEnableVertexAttribArray(program->texCoordAttribute);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(program->positionAttribute);
	glDisableVertexAttribArray(program->texCoordAttribute);
}
//World Functions
void WorldMap::Draw(ShaderProgram *program) {
	model.Identity();
	program->SetModelMatrix(model);
	glBindTexture(GL_TEXTURE_2D, world.mapTexture);
	world.DrawMap(program);
}
void Overworld::DrawMap(ShaderProgram *program) {
	float TILE_SIZE1 = 1.0;
	float TILE_SIZE2 = 1.0;
	int SPRITE_COUNT_X = 3;
	int SPRITE_COUNT_Y = 1;
	float spriteWidth = 1.0 / 3;
	float spriteHeight = 1.0;
	std::vector<float> vertexData;
	std::vector<float> texCoordData;

	for (int y = 0; y < fmap.mapHeight; y++) {
		for (int x = 0; x < fmap.mapWidth; x++) {
			int spriteIndex = (int)fmap.mapData[y][x];
			float xpos = (x - y) * (TILE_SIZE1 / 2.08);
			float ypos = (x + y) * (TILE_SIZE2 / 2.89);
			float u = (float)(((int)spriteIndex) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
			float v = (float)(((int)spriteIndex) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
			vertexData.insert(vertexData.end(), {
				TILE_SIZE1 * xpos, -TILE_SIZE2 * ypos,
				TILE_SIZE1 * xpos, (-TILE_SIZE2 * ypos) - TILE_SIZE2,
				(TILE_SIZE1 * xpos) + TILE_SIZE1, (-TILE_SIZE2 * ypos) - TILE_SIZE2,

				TILE_SIZE1 * xpos, -TILE_SIZE2 * ypos,
				(TILE_SIZE1 * xpos) + TILE_SIZE1, (-TILE_SIZE2 * ypos) - TILE_SIZE2,
				(TILE_SIZE1 * xpos) + TILE_SIZE1, -TILE_SIZE2 * ypos
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
void PlayerW::Update(float elapsed) {
	animationElapsed += elapsed;
	if (animationElapsed > 1.0 / framesPerSecond) {
		spriteIndex++;
		animationElapsed = 0.0;
		if (spriteIndex > numFrames - 1) {
			spriteIndex = 0;
		}
	}
	int gridX;
	int gridY;

	world.worldToTileCoordinates(position.x, position.y, &gridX, &gridY);
	for (Enemy& enemy : world.enemies) {
		if (!enemy.dead) {
			if (position.x - 0.5 <= enemy.position.x + 0.5 && position.x + 0.5 >= enemy.position.x - 0.5
				&& position.y - 0.25 >= enemy.position.y - 0.5 && position.y - 0.5 <= enemy.position.y - 0.25) {
				enemy.active = true;
				Mix_PlayMusic(battle.music[0], -1);
				enemy.dead = true;
				mode = STATE_BATTLE_MODE;
				battle.Setup();
			}
		}
	}
}
void PlayerW::Draw(ShaderProgram *program) {
	modelMatrix.Identity();
	modelMatrix.Translate(position.x, position.y, position.z);
	modelMatrix.Scale(scale.x, scale.y, scale.z);
	program->SetModelMatrix(modelMatrix);

	switch (movement) {
	case MODE_IDLE:
		spriteIndex = 0;
		sprites_idle[direction].Draw(program);
		break;
	case MODE_UP:
		numFrames = 6;
		framesPerSecond = 10.0;
		sprites_up[spriteIndex].Draw(program);
		break;
	case MODE_DOWN:
		numFrames = 6;
		framesPerSecond = 10.0;
		sprites_down[spriteIndex].Draw(program);
		break;
	case MODE_LEFT:
		numFrames = 6;
		framesPerSecond = 10.0;
		sprites_left[spriteIndex].Draw(program);
		break;
	case MODE_RIGHT:
		numFrames = 6;
		framesPerSecond = 10.0;
		sprites_right[spriteIndex].Draw(program);
		break;
	default:
		break;
	}
}
void Enemy::Update(float elapsed) {

}
void Overworld::Setup() {

}
void Overworld::Events(SDL_Event* event, bool& done) {
	while (SDL_PollEvent(event)) {
		if (event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		if (event->type == SDL_KEYDOWN) {
			if (event->key.keysym.scancode == SDL_SCANCODE_RIGHT) {
				player1.position.x += 0.1;
				player1.movement = WorldObject::MODE_RIGHT;
				player1.direction = 1;
			}
			else if (event->key.keysym.scancode == SDL_SCANCODE_LEFT) {
				player1.position.x -= 0.1;
				player1.movement = WorldObject::MODE_LEFT;
				player1.direction = 2;
			}
			if (event->key.keysym.scancode == SDL_SCANCODE_UP) {
				player1.position.y += 0.1;
				player1.movement = WorldObject::MODE_UP;
				player1.direction = 3;
			}
			else if (event->key.keysym.scancode == SDL_SCANCODE_DOWN) {
				player1.position.y -= 0.1;
				player1.movement = WorldObject::MODE_DOWN;
				player1.direction = 0;
			}
			if (event->key.keysym.scancode == SDL_SCANCODE_Z) {
				if (player1.position.x - 0.5 <= world.player2.position.x + 0.5 && player1.position.x + 0.5 >= world.player2.position.x - 0.5
					&& player1.position.y - 0.25 >= world.player2.position.y - 0.5 && player1.position.y - 0.5 <= world.player2.position.y - 0.25) {
					battle2P.Setup();
					mode = STATE_BATTLE_MODE_2;
				}
			}
			if (event->key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				done = true;
			}
		}
		if (event->type == SDL_KEYUP) {
			player1.movement = WorldObject::MODE_IDLE;
		}
	}
}
void Overworld::Update(float elapsed) {
	player1.Update(elapsed);
	player2.Update(elapsed);
	for (Enemy& enemy : enemies) { enemy.Update(elapsed); }
}
void Overworld::Render(ShaderProgram *program) {
	glClearColor(0 / 255.0, 30 / 255.0, 79 / 255.0, 1.0f);
	viewMatrix.Identity();
	viewMatrix.Translate(-player1.position.x, -player1.position.y, 0.0);
	program->SetViewMatrix(viewMatrix);
	map.Draw(program);
	player1.Draw(program);
	player2.Draw(program);
	for (Enemy& enemy : enemies) { enemy.Draw(program); }
}

//Battle Funcions
//1 Player
void BattleMap::Draw(ShaderProgram *program) {
	model.Identity();
	model.Scale(4.0, 1.0, 1.0);
	program->SetModelMatrix(model);
	glBindTexture(GL_TEXTURE_2D, battle.mapTexture);
	battle.DrawMap(program);
}
void BattleMode::DrawMap(ShaderProgram *program) {
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
void Particle::Update(float elapsed) {
	position.x = battle.arena[gridX][gridY].first;
	position.y = battle.arena[gridX][gridY].second;

	animationElapsed += elapsed;
	if (animationElapsed > 1.0 / framesPerSecond) {
		CollisionCheck();
		spriteIndex++;
		animationElapsed = 0.0;
		if (spriteIndex > numFrames - 1) {
			spriteIndex = 0;
			gridX += direction;
			if (gridX == 6 || gridX == -1) { pType = TYPE_IDLE; }
		}
	}
}
void Particle::Draw(ShaderProgram *program) {
	model.Identity();
	model.Translate(position.x, position.y, position.z);
	model.Scale(scale.x*direction, scale.y, scale.z);
	program->SetModelMatrix(model);

	switch (pType) {
	case TYPE_IDLE:
		break;
	case TYPE_WAVE:
		numFrames = 7;
		framesPerSecond = 35.0;
		sprites_wave[spriteIndex].Draw(program);
		break;
	case TYPE_EXPLODE:
		numFrames = 4;
		framesPerSecond = 15.0;
		sprites_explode[spriteIndex].Draw(program);
		break;
	default:
		break;
	}
}
void Particle::CollisionCheck() {
	if (mode == STATE_BATTLE_MODE) {
		PlayerB1* player = &battle.player;
		for (Mettenna& meta : battle.metas) {
			if (meta.active) {
				if (direction == -1) {
					if (player->gridX == gridX && player->gridY == gridY
						&& player->objmode != BattleObject::MODE_HIT) {
						Mix_PlayChannel(-1, battle.SFX[2], 0);
						player->HP -= 5;
						player->objmode = BattleObject::MODE_HIT;
					}
				}
			}
		}

		for (BattleEnemy* meta : battle.enemies) {
			if (!meta->dead) {
				if (direction == 1) {
					if (meta->gridX == gridX && meta->gridY == gridY) {
						Mix_PlayChannel(-1, battle.SFX[2], 0);
						meta->HP -= 20;
					}
				}
			}
		}
	}
	if (mode == STATE_BATTLE_MODE_2) {
		if (battle2P.player.gridX == gridX && battle2P.player.gridY == gridY
			&& direction == -1) {
			Mix_PlayChannel(-1, battle.SFX[2], 0);
			battle2P.player.HP -= 20;
		}
		if (battle2P.player2.gridX == gridX && battle2P.player2.gridY == gridY
			&& direction == 1) {
			Mix_PlayChannel(-1, battle.SFX[2], 0);
			battle2P.player2.HP -= 20;
		}
	}
}

void PlayerB1::Update(float elapsed) {
	if (HP <= 0) {
		dead = true; 
		Mix_PlayMusic(battle.music[4], 1);
		mode = STATE_GAME_OVER;
	}

	if (particle.pType == Particle::TYPE_IDLE) {
		particle.gridX = gridX;
		particle.gridY = gridY;
	}

	if (objmode == BattleObject::MODE_ATTACK && attack == BattleObject::MODE_SLAM) {
		position.x += speed;
		Attack();
		if (position.x < 0 || position.x > 20.0) {
			objmode = BattleObject::MODE_IDLE;
		}
	}
	else {
		position.x = battle.arena[gridX][gridY].first;
		position.y = battle.arena[gridX][gridY].second;
	}

	animationElapsed += elapsed;
	if (animationElapsed > 1.0 / framesPerSecond) {
		spriteIndex++;
		animationElapsed = 0.0;
		if (spriteIndex > numFrames - 1) {
			if (objmode == BattleObject::MODE_ATTACK && attack == BattleObject::MODE_WAVE) {
				particle.pType = Particle::TYPE_WAVE;
				particle.gridX = gridX + 1;
				particle.gridY = gridY;
			}
			objmode = BattleObject::MODE_IDLE;
		}
	}
	if (objmode == MODE_ATTACK) {
		if (attack == MODE_SHOOT || attack == MODE_NORM_SHOOT) {
			if (spriteIndex == 0) { position.x += 0.2; }
			else { position.x += 0.5; }
		}
		if (attack == MODE_THROW) { position.x += 0.2; }
	}
	particle.Update(elapsed);
}
void PlayerB1::Attack() {
	BattleObject* row1 = nullptr;
	int index = 0;
	while (index < 6 && row1 == nullptr) {
		row1 = battle.arenaMap[index][gridY];
		index++;
	}
	if (row1 != nullptr) {
		if (attack == BattleObject::MODE_NORM_SHOOT) {
			Mix_PlayChannel(-1, battle.SFX[2], 0);
			row1->HP -= 2;
		}
		if (attack == BattleObject::MODE_SHOOT) {
			Mix_PlayChannel(-1, battle.SFX[2], 0);
			row1->HP -= 10;
		}
	}
	for (BattleEnemy* enemy : battle.enemies) {
		if (attack == BattleObject::MODE_WAVE) {
			if (enemy->gridY == particle.gridY && enemy->gridX == particle.gridX) {
				Mix_PlayChannel(-1, battle.SFX[2], 0);
				enemy->HP -= 20;
			}
		}
		if (attack == BattleObject::MODE_SLAM) {
			if (enemy->gridY == gridY && battle.arena[enemy->gridX]->first - 0.6 <= position.x
				&& battle.arena[enemy->gridX]->first + 0.6 >= position.x) {
				enemy->HP -= 10;
			}
		}
	}
}
void PlayerB1::Draw(ShaderProgram *program) {
	modelMatrix.Identity();
	modelMatrix.Translate(position.x, position.y, position.z);
	modelMatrix.Scale(scale.x, scale.y, scale.z);
	program->SetModelMatrix(modelMatrix);

	switch (objmode){
	case BattleObject::MODE_IDLE:
		spriteIndex = 0;
		speed = 0.0;
		attack = MODE_NORM_SHOOT;
		sprites_idle.Draw(program);
		break;
	case BattleObject::MODE_HIT:
		numFrames = 4;
		framesPerSecond = 10.0;
		speed = 0.0;
		sprites_hit[spriteIndex].Draw(program);
		break;
	case BattleObject::MODE_ATTACK:
		switch (attack) {
		case BattleObject::MODE_NORM_SHOOT:
			numFrames = 4;
			framesPerSecond = 15.0;
			speed = 0.0;
			sprites_shoot[spriteIndex].Draw(program);
			break;
		case BattleObject::MODE_SHOOT:
			numFrames = 4;
			framesPerSecond = 15.0;
			speed = 0.0;
			sprites_shoot[spriteIndex].Draw(program);
			break;
		case BattleObject::MODE_WAVE:
			numFrames = 6;
			framesPerSecond = 10.0;
			speed = 0.0;
			sprites_wave[spriteIndex].Draw(program);
			break;
		case BattleObject::MODE_SLAM:
			numFrames = 5;
			framesPerSecond = 10.0;
			speed = 0.06;
			sprites_slam[spriteIndex].Draw(program);
			break;
		default:
			break;
		}
	default:
		break;
	}
	if (particle.pType != Particle::TYPE_IDLE) {
		particle.direction = 1;
		particle.Draw(program);
	}
}

void Player1B2::Update(float elapsed) {
	if (HP <= 0) {
		dead = true;
		Mix_PlayMusic(battle.music[3], -1);
		mode = STATE_OVERWORLD;
	}
	if (objmode == BattleObject::MODE_ATTACK && attack == BattleObject::MODE_SLAM) {
		position.x += velocity.x;
		Attack();
		if (position.x < 0 || position.x > 20.0) {
			objmode = BattleObject::MODE_IDLE;
		}
	}
	else {
		position.x = battle.arena[gridX][gridY].first;
		position.y = battle.arena[gridX][gridY].second;
	}

	if (particle.pType == Particle::TYPE_IDLE) {
		particle.gridX = gridX;
		particle.gridY = gridY;
	}
	else {
		if (objmode == BattleObject::MODE_ATTACK) {
			if (battle2P.player2.gridX == gridX && battle2P.player2.gridY == gridY) {
				Mix_PlayChannel(-1, battle.SFX[2], 0);
				battle2P.player2.HP -= 20;
			}
		}
	}


	animationElapsed += elapsed;
	if (animationElapsed > 1.0 / framesPerSecond) {
		spriteIndex++;
		animationElapsed = 0.0;
		if (spriteIndex > numFrames - 1) {
			if (objmode == BattleObject::MODE_ATTACK && attack == BattleObject::MODE_WAVE) {
				particle.pType = Particle::TYPE_WAVE;
				particle.gridX = gridX + 1;
				particle.gridY = gridY;
			}
			objmode = BattleObject::MODE_IDLE;
		}
	}
	if (objmode == MODE_ATTACK) {
		position.x -= 0.4;
	}
	if (objmode == MODE_ATTACK) {
		if (attack == MODE_SHOOT || attack == MODE_NORM_SHOOT) {
			if (spriteIndex == 0) { position.x += 0.2; }
			else { position.x += 0.5; }
		}
		if (attack == MODE_THROW) { position.x += 0.2; }
	}
	particle.Update(elapsed);
}
void Player1B2::Attack() {
	if (objmode == BattleObject::MODE_ATTACK) {
		if (attack == BattleObject::MODE_NORM_SHOOT) {
			Mix_PlayChannel(-1, battle.SFX[2], 0);
			battle2P.player2.HP -= 2;
		}
		if (attack == BattleObject::MODE_SHOOT) {
			Mix_PlayChannel(-1, battle.SFX[2], 0);
			battle2P.player2.HP -= 5;
		}
		if (attack == BattleObject::MODE_SLAM) {
			if (battle2P.player2.gridY == gridY && battle.arena[battle2P.player2.gridX]->first - 0.6 <= position.x
				&& battle.arena[battle2P.player.gridX]->first + 0.6 >= position.x) {
				Mix_PlayChannel(-1, battle.SFX[2], 0);
				battle2P.player2.HP -= 10;
			}
		}
	}
}
void Player1B2::Draw(ShaderProgram *program) {
	modelMatrix.Identity();
	modelMatrix.Translate(position.x, position.y, position.z);
	modelMatrix.Scale(scale.x, scale.y, scale.z);
	program->SetModelMatrix(modelMatrix);

	switch (objmode) {
	case BattleObject::MODE_IDLE:
		spriteIndex = 0;
		attack = MODE_NORM_SHOOT;
		speed = 0.0;
		sprites_idle.Draw(program);
		break;
	case BattleObject::MODE_HIT:
		numFrames = 4;
		framesPerSecond = 10.0;
		speed = 0.0;
		sprites_hit[spriteIndex].Draw(program);
		break;
	case BattleObject::MODE_ATTACK:
		switch (attack) {
		case BattleObject::MODE_NORM_SHOOT:
			numFrames = 4;
			framesPerSecond = 15.0;
			speed = 0.0;
			sprites_shoot[spriteIndex].Draw(program);
			break;
		case BattleObject::MODE_SHOOT:
			numFrames = 4;
			framesPerSecond = 15.0;
			speed = 0.0;
			sprites_shoot[spriteIndex].Draw(program);
			break;
		case BattleObject::MODE_WAVE:
			numFrames = 6;
			framesPerSecond = 10.0;
			speed = 0.0;
			sprites_wave[spriteIndex].Draw(program);
			break;
		case BattleObject::MODE_SLAM:
			numFrames = 5;
			framesPerSecond = 10.0;
			speed = 0.06;
			sprites_slam[spriteIndex].Draw(program);
			break;
		default:
			break;
		}
	default:
		break;
	}
	if (particle.pType != Particle::TYPE_IDLE) {
		particle.direction = 1;
		particle.Draw(program);
	}
}

void Player2B2::Update(float elapsed) {
	if (HP <= 0) {
		dead = true;
		Mix_PlayMusic(battle.music[3], -1);
		mode = STATE_OVERWORLD;
	}
	position.x = battle.arena[gridX][gridY].first;
	position.y = battle.arena[gridX][gridY].second;

	if (particle.pType == Particle::TYPE_IDLE) {
		particle.gridX = gridX;
		particle.gridY = gridY;
	}
	else {
		if (objmode == BattleObject::MODE_ATTACK) {
			if (battle2P.player.gridX == gridX && battle2P.player.gridY == gridY) {
				Mix_PlayChannel(-1, battle.SFX[2], 0);
				battle2P.player.HP -= 20;
			}
		}
	}

	if (objmode == BattleObject::MODE_ATTACK && attack == BattleObject::MODE_SLAM) {
		position.x += velocity.x;
		Attack();
		if (position.x < 0 || position.x > 20.0) {
			objmode = BattleObject::MODE_IDLE;
		}
	}

	animationElapsed += elapsed;
	if (animationElapsed > 1.0 / framesPerSecond) {
		spriteIndex++;
		animationElapsed = 0.0;
		if (spriteIndex > numFrames - 1) {
			if (objmode == BattleObject::MODE_ATTACK && attack == BattleObject::MODE_WAVE) {
				particle.pType = Particle::TYPE_WAVE;
				particle.gridX = gridX + 1;
				particle.gridY = gridY;
			}
			if (attack == BattleObject::MODE_SLAM) {
				spriteIndex = 0;
			}
			else {
				objmode = BattleObject::MODE_IDLE;
			}
		}
	}
	if (objmode == MODE_IDLE) {
		position.x -= 0.15;
	}
	if (objmode == MODE_ATTACK) {
		position.x -= 0.4;
	}
	if (objmode == MODE_ATTACK) {
		if (attack == MODE_SHOOT || attack == MODE_NORM_SHOOT) {
			if (spriteIndex == 0) { position.x += 0.2; }
			else { position.x += 0.5; }
		}
		if (attack == MODE_THROW) { position.x += 0.2; }
	}
	particle.Update(elapsed);
}
void Player2B2::Attack() {
	if (objmode == BattleObject::MODE_ATTACK) {
		if (attack == BattleObject::MODE_NORM_SHOOT) {
			Mix_PlayChannel(-1, battle.SFX[2], 0);
			battle2P.player.HP -= 2;
		}
		if (attack == BattleObject::MODE_SHOOT) {
			Mix_PlayChannel(-1, battle.SFX[2], 0);
			battle2P.player.HP -= 5;
		}
		if (attack == BattleObject::MODE_SLAM) {
			if (battle2P.player.gridY == gridY && battle.arena[battle2P.player.gridX]->first - 0.6 <= position.x
				&& battle.arena[battle2P.player.gridX]->first + 0.6 >= position.x) {
				battle2P.player.HP -= 10;
			}
		}
	}
}
void Player2B2::Draw(ShaderProgram *program) {
	modelMatrix.Identity();
	modelMatrix.Translate(position.x, position.y, position.z);
	modelMatrix.Scale(scale.x, scale.y, scale.z);
	program->SetModelMatrix(modelMatrix);

	switch (objmode) {
	case BattleObject::MODE_IDLE:
		spriteIndex = 0;
		attack = MODE_NORM_SHOOT;
		speed = 0.0;
		sprites_idle.Draw(program);
		break;
	case BattleObject::MODE_HIT:
		numFrames = 4;
		framesPerSecond = 10.0;
		speed = 0.0;
		sprites_hit[spriteIndex].Draw(program);
		break;
	case BattleObject::MODE_ATTACK:
		switch (attack) {
		case BattleObject::MODE_NORM_SHOOT:
			numFrames = 4;
			framesPerSecond = 15.0;
			speed = 0.0;
			sprites_shoot[spriteIndex].Draw(program);
			break;
		case BattleObject::MODE_SHOOT:
			numFrames = 4;
			framesPerSecond = 15.0;
			speed = 0.0;
			sprites_shoot[spriteIndex].Draw(program);
			break;
		case BattleObject::MODE_WAVE:
			numFrames = 6;
			framesPerSecond = 10.0;
			speed = 0.0;
			sprites_wave[spriteIndex].Draw(program);
			break;
		case BattleObject::MODE_SLAM:
			numFrames = 5;
			framesPerSecond = 10.0;
			speed = -0.06;
			sprites_slam[spriteIndex].Draw(program);
			break;
		default:
			break;
		}
	default:
		break;
	}
	if (particle.pType != Particle::TYPE_IDLE) {
		particle.direction = 1;
		particle.Draw(program);
	}
}

void Mettenna::Update(float elapsed) {
	position.x = battle.arena[gridX][gridY].first;
	position.y = battle.arena[gridX][gridY].second;
	if (active) {
		moveTimer += elapsed;
		attackTimer += elapsed;

		battle.UpdateMap();
		if (moveTimer >= 1.2 && objmode == BattleObject::MODE_IDLE && gridY != battle.player.gridY) {
			if (battle.player.gridY > gridY) {
				if (battle.arenaMap[gridX][gridY + 1] == nullptr ) { gridY += 1; }
				else if (battle.player.gridY - gridY == 2 && battle.arenaMap[gridX][gridY + 2] == nullptr) {
					gridY += 2;
				}
			}
			else if (battle.player.gridY < gridY) {
				if (battle.arenaMap[gridX][gridY - 1] == nullptr) { gridY -= 1; }
				else if (gridY - battle.player.gridY == 2 && battle.arenaMap[gridX][gridY - 2] == nullptr) {
					gridY -= 2;
				}
			}
			moveTimer = 0.0;
		}
	}
	if (attackTimer >= 5 && objmode == BattleObject::MODE_IDLE && gridY == battle.player.gridY) {
		objmode = BattleObject::MODE_ATTACK;
		attackTimer = 0.0;
		//active = false;
	}
	if (particle.pType != Particle::TYPE_IDLE) {
		particle.Update(elapsed);
	}
	if (HP == 0) { dead = true; }
	animationElapsed += elapsed;
	if (animationElapsed > 1.0 / framesPerSecond) {
		spriteIndex++;
		animationElapsed = 0.0;
		if (spriteIndex > numFrames - 1) {
			objmode = MODE_IDLE;
		}
	}
	if (8 <= spriteIndex && spriteIndex != 13) {
		position.x -= 0.4;
	}
	if (spriteIndex == 9) {
		particle.pType = Particle::TYPE_WAVE;
		particle.direction = -1;
		particle.gridX = gridX - 1;
		particle.gridY = gridY;
		Mix_PlayChannel(-1, battle.SFX[2], 0);
	}

	if (HP <= 0) { dead = true; }
	HPBar.position.y = position.y + 0.1;

	if (HP > 10) { HPBar.position.x = position.x - 0.05; }
	else { HPBar.position.x = position.x - 0.05; }
}
void Zoomer::Attack() {
	PlayerB1* player = &battle.player;
	if (player->gridY == gridY && battle.arena[player->gridX]->first - 0.6 <= position.x
		&& battle.arena[player->gridX]->first + 0.6 >= position.x
		&& player->objmode != BattleObject::MODE_HIT) {

		player->objmode = BattleObject::MODE_HIT;
		player->HP -= 10;
	}
}
void Zoomer::Update(float elapsed) {
	if (objmode != BattleObject::MODE_ATTACK) {
		position.x = battle.arena[gridX][gridY].first;
		position.y = battle.arena[gridX][gridY].second;
	}
	else if (objmode == BattleObject::MODE_ATTACK) {
		position.x -= speed;
		Attack();
	}
	if (gridY == battle.player.gridY && attackTimer > 2.0) {
		Mix_PlayChannel(-1, battle.SFX[3], 0);
		objmode = BattleObject::MODE_ATTACK;
		attackTimer = 0.0;
	}
	moveTimer += elapsed;
	if (objmode == BattleObject::MODE_IDLE) { attackTimer += elapsed; }
	if (moveTimer >= 1.5 && objmode == BattleObject::MODE_IDLE) {
		gridX = rand() % 3 + 3;
		gridY = rand() % 3;
		while (battle.arenaMap[gridX][gridY] != nullptr) {
			gridX = rand() % 3 + 3;
			gridY = rand() % 3;
		}
		battle.UpdateMap();
		moveTimer = 0;
	}
	if (HP == 0) { dead = true; }
	animationElapsed += elapsed;
	if (animationElapsed > 1.0 / framesPerSecond) {
		spriteIndex++;
		animationElapsed = 0.0;
		if (spriteIndex > numFrames - 1) {
			spriteIndex = 0;
			if (objmode == BattleObject::MODE_ATTACK && position.x > 0) {
				objmode = BattleObject::MODE_ATTACK;
			}
			else { objmode = BattleObject::MODE_IDLE; }
		}
	}
	if (objmode == BattleObject::MODE_IDLE) {
		position.y -= 0.07;
		position.x += 0.02;
	}

	if (HP <= 0) { dead = true; }
	HPBar.position.y = position.y + 0.1;

	if (HP > 10) { HPBar.position.x = position.x - 0.05; }
	else { HPBar.position.x = position.x - 0.05; }
}
void Vulcan::Update(float elapsed) {
	if (HP == 0) { dead = true; }
	if (!dead) {
		position.x = battle.arena[gridX][gridY].first;
		position.y = battle.arena[gridX][gridY].second;

		animationElapsed += elapsed;
		if (animationElapsed > 1.0 / framesPerSecond) {
			spriteIndex++;
			animationElapsed = 0.0;
			if (spriteIndex > numFrames - 1) {
				spriteIndex = 0;
			}
		}

		if (battle.player.gridY == gridY) {
			attackTimer += elapsed;
			if (attackTimer <= 2.5) {
				objmode = BattleObject::MODE_IDLE;
			}
			else if (attackTimer <= 5.5 && attackTimer > 2.5) {
				objmode = BattleObject::MODE_ATTACK;
				if (attackTimer >= 2.5 + 0.5 * attacks && attacks < 5) {
					Attack();
					attacks++;
					if (attacks <= 5) {
						attacks = 0;
						attackTimer = 0.0;
						objmode = BattleObject::MODE_IDLE;
					}
				}
			}
		}

		position.y -= 0.1;
		position.x -= 0.1;

		if (objmode == BattleObject::MODE_ATTACK) {
			if (spriteIndex == 4) {
				Mix_PlayChannel(-1, battle.SFX[0], 0);
			}
		}
		if (HP <= 0) { dead = true; }
		HPBar.position.y = position.y + 0.1;

		if (HP > 10) { HPBar.position.x = position.x - 0.05; }
		else { HPBar.position.x = position.x - 0.05; }
	}
}
void Vulcan::Attack() {
	PlayerB1* player = &battle.player;
	if (player->gridY == gridY) {
		player->objmode = BattleObject::MODE_HIT;
		player->HP -= 5;
	}
}
void BattleMode::Setup() {
	player.gridX = 1;
	player.gridY = 1;
	player.dead = false;
	player.HP = 100;
	player.ActivateCards();
	sorts.push_back(&player);

	for (int i = 0; i < 3; ++i) {
		int e = rand() % 3;
		if (e == 0) {
			battle.enemies[i] = &battle.metas[i];
			battle.enemies[i]->gridX = rand() % 3 + 3;
			battle.enemies[i]->gridY = rand() % 3;
			while (battle.arenaMap[battle.enemies[i]->gridX][battle.enemies[i]->gridY] != nullptr) {
				battle.enemies[i]->gridX = rand() % 3 + 3;
				battle.enemies[i]->gridY = rand() % 3;
			}
			battle.metas[i].active = true;
		}
		else if (e == 1) {
			battle.enemies[i] = &battle.zooms[i];
			battle.enemies[i]->gridX = rand() % 3 + 3;
			battle.enemies[i]->gridY = rand() % 3;
			while (battle.arenaMap[battle.enemies[i]->gridX][battle.enemies[i]->gridY] != nullptr) {
				battle.enemies[i]->gridX = rand() % 3 + 3;
				battle.enemies[i]->gridY = rand() % 3;
			}
		}
		else if (e == 2) {
			battle.enemies[i] = &battle.vulcs[i];
			battle.enemies[i]->gridX = rand() % 3 + 3;
			battle.enemies[i]->gridY = i;
			while (battle.arenaMap[battle.enemies[i]->gridX][battle.enemies[i]->gridY] != nullptr) {
				battle.enemies[i]->gridX = rand() % 3 + 3;
			}
		}
		battle.enemies[i]->dead = false;
		battle.UpdateMap();
		battle.enemies[i]->Reset();
		sorts.push_back(battle.enemies[i]);
	}
	Mix_PlayMusic(music[0], -1);

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
				if (player.gridX < 2) {
					player.gridX += 1;
				}
			}
			else if (event->key.keysym.scancode == SDL_SCANCODE_LEFT) {
				if (player.gridX > 0) {
					player.gridX -= 1;
				}
			}
			if (event->key.keysym.scancode == SDL_SCANCODE_UP) {
				if (player.gridY > 0) {
					player.gridY -= 1;
				}
			}
			else if (event->key.keysym.scancode == SDL_SCANCODE_DOWN) {
				if (player.gridY < 2) {
					player.gridY += 1;
				}
			}
			if (event->key.keysym.scancode == SDL_SCANCODE_Z) {
				player.objmode = BattleObject::MODE_ATTACK;
				player.attack = BattleObject::MODE_NORM_SHOOT;
				player.Attack();
				Mix_PlayChannel(-1, SFX[0], 0);
			}
			if (event->key.keysym.scancode == SDL_SCANCODE_X) {
				player.objmode = BattleObject::MODE_ATTACK;
				player.UseCard();
			}
			if (event->key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				done = true;
			}
		}
	}
}
void BattleMode::Update(float elapsed) {
	metas[meta_active].active = true;
	int alive = 0;
	for (BattleEnemy* zoom : enemies) {
		if (!zoom->dead) { 
			zoom->Update(elapsed); 
			alive++;
		}
		if (meta_active > 2) { meta_active = 0; }
	}
	if (!player.dead) { player.Update(elapsed); }
	if (alive == 0) {
		Mix_PlayMusic(battle.music[3], -1);
		mode = STATE_OVERWORLD;
	}
}
void BattleMode::Render(ShaderProgram *program) {
	glClearColor(0 / 255.0, 30 / 255.0, 79 / 255.0, 1.0f);
	viewMatrix.Identity();
	viewMatrix.Translate(-3.55, 0.6, 0.0);
	program->SetViewMatrix(viewMatrix);

	//  renders first
	map.Draw(program);
	
	//Text
	player.HPBar.position.x = 0.1;
	player.HPBar.position.y = 1.3;
	player.HPBar.Draw(program, std::to_string(player.HP), 0.4f, -0.15f);
	for (int i = 0; i < 5; ++i) {
		if (!player.cards[i].used) {
			player.cards[i].position.y = player.HPBar.position.y - 0.5;
			player.cards[i].position.x = 0.6 + 0.6 * i;
			player.cards[i].Draw(program);
		}
	}
	for (BattleEnemy* enemy : battle.enemies) {
		if (enemy != nullptr) {
			if (!enemy->dead) { enemy->Draw(program); }
		}
	}
	if (!player.dead) { player.Draw(program); }
	else { mode = STATE_MAIN_MENU; }

	
}
//2 Player
void BattleMode2::Setup() {
	player.gridX = 1;
	player.gridY = 1;
	player.dead = false;
	player.ActivateCards();

	player2.scale.x = -1.0; 
	player2.gridX = 4;
	player2.gridY = 1;
	player2.dead = false;
	player2.ActivateCards();

	Mix_PlayMusic(battle.music[1], -1);
}
void BattleMode2::Events(SDL_Event* event, bool& done) {
	while (SDL_PollEvent(event)) {
		if (event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT_CLOSE) {
			done = true;
		}
		if (event->type == SDL_KEYDOWN) {
			//player 1 controls
			if (player.objmode == BattleObject::MODE_IDLE) {
				if (event->key.keysym.scancode == SDL_SCANCODE_D) {
					if (player.gridX < 2) {
						player.gridX += 1;
					}
				}
				else if (event->key.keysym.scancode == SDL_SCANCODE_A) {
					if (player.gridX > 0) {
						player.gridX -= 1;
					}
				}
				if (event->key.keysym.scancode == SDL_SCANCODE_W) {
					if (player.gridY > 0) {
						player.gridY -= 1;
					}
				}
				else if (event->key.keysym.scancode == SDL_SCANCODE_S) {
					if (player.gridY < 2) {
						player.gridY += 1;
					}
				}
				if (event->key.keysym.scancode == SDL_SCANCODE_Q) {
					player.objmode = BattleObject::MODE_ATTACK;
					player.attack = BattleObject::MODE_NORM_SHOOT;
					player.Attack();
					Mix_PlayChannel(-1, SFX[0], 0);
				}
				if (event->key.keysym.scancode == SDL_SCANCODE_E) {
					player.objmode = BattleObject::MODE_ATTACK;
					player.UseCard();
				}
			}
			//Player 2
			if (player2.objmode == BattleObject::MODE_IDLE) {
				if (event->key.keysym.scancode == SDL_SCANCODE_RIGHT) {
					if (player2.gridX < 5) {
						player2.gridX += 1;
					}
				}
				else if (event->key.keysym.scancode == SDL_SCANCODE_LEFT) {
					if (player2.gridX > 3) {
						player2.gridX -= 1;
					}
				}
				if (event->key.keysym.scancode == SDL_SCANCODE_UP) {
					if (player2.gridY > 0) {
						player2.gridY -= 1;
					}
				}
				else if (event->key.keysym.scancode == SDL_SCANCODE_DOWN) {
					if (player2.gridY < 2) {
						player2.gridY += 1;
					}
				}
				if (event->key.keysym.scancode == SDL_SCANCODE_N) {
					player2.objmode = BattleObject::MODE_ATTACK;
					player2.attack = BattleObject::MODE_NORM_SHOOT;
					player2.Attack();
					Mix_PlayChannel(-1, SFX[0], 0);
				}
				if (event->key.keysym.scancode == SDL_SCANCODE_M) {
					player2.objmode = BattleObject::MODE_ATTACK;
					player2.UseCard();
				}
			}
			if (event->key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				done = true;
			}
		}
	}
}
void BattleMode2::Update(float elapsed) {
	if (!player.dead) { player.Update(elapsed); }
	if (!player2.dead) { player2.Update(elapsed); }
}
void BattleMode2::Render(ShaderProgram *program) {
	glClearColor(0 / 255.0, 30 / 255.0, 79 / 255.0, 1.0f);
	viewMatrix.Identity();
	viewMatrix.Translate(-3.55, 0.6, 0.0);
	program->SetViewMatrix(viewMatrix);

	//Text
	player.HPBar.position.x = 0.5;
	player.HPBar.position.y = 1.3;
	player.HPBar.Draw(program, std::to_string(player.HP), 0.4f, -0.15f);
	for (int i = 0; i < 5; ++i) {
		if (!player.cards[i].used) {
			player.cards[i].position.y = player.HPBar.position.y - 0.5;
			player.cards[i].position.x = 0.6 + 0.6 * i;
			player.cards[i].Draw(program);
		}
	}

	player2.HPBar.position.x = 6.0;
	player2.HPBar.position.y = 1.3;
	player2.HPBar.Draw(program, std::to_string(player2.HP), 0.4f, -0.15f);
	for (int i = 0; i < 5; ++i) {
		if (!player2.cards[i].used) {
			player2.cards[i].position.y = player2.HPBar.position.y - 0.5;
			player2.cards[i].position.x = player2.HPBar.position.x - 1.8 + 0.6 * i;
			player2.cards[i].Draw(program);
		}
	}
	// smaller renders first
	map.Draw(program);
	if (!player.dead) { player.Draw(program); }
	if (!player2.dead) { player2.Draw(program); }

	
}