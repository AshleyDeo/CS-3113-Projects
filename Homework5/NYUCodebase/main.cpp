#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <vector>
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#include "SatCollision.h"
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

//classes
class Vector3 {
public:
	Vector3() {}
	Vector3(float x, float y, float z) :x(x), y(y), z(z) {}
	Vector3 operator*(const Matrix& matrix) {
		Vector3 newVect;

		newVect.x = x + matrix.m[0][0] + matrix.m[1][0] + matrix.m[2][0];
		newVect.y = y + matrix.m[0][1] + matrix.m[1][1] + matrix.m[2][1];
		
		return newVect;
	}
	Vector3 operator*(const Vector3& vec) {
		Vector3 newVect;

		newVect.x = x  * vec.x;
		newVect.y = y * vec.y;

		return newVect;
	}
	float x = 0.0;
	float y = 0.0;
	float z = 0.0;
};

float lastFrameTicks = 0.0f;
Matrix projection;
Matrix view;

class Object {
public:
	Object() {
		scale.x = 1.0;
		scale.y = 1.0;
		scale.z = 1.0;
	}

	void Collide() {
		velocity.x *= -1;
		velocity.y *= -1;

	}

	void CalcNorms() {
		for (int i = 0; i < 4; ++i) {
			float tempX = objpoints[i].first - position.x;
			float tempY = objpoints[i].second - position.y ;
		
			edges[i].x = tempX * cos(angle) - tempY * sin(angle);
			edges[i].y = tempX * sin(angle) + tempY * cos(angle);
		}
	}

	void Update(float elapsed) {
		if (position.x >= 3.55 || position.x <= -3.55 || position.y >= 2.0 || position.y <= -2.0) {
			Collide();
			if (position.x >= 3.55) { position.x = 3.549; }
			if (position.x <= -3.55) { position.x = -3.549; }
			if (position.y >= 2.0) { position.y = 1.999; }
			if (position.y <= -2.0) { position.y = -1.999; }
		}
		position.x += elapsed * velocity.x;
		position.y += elapsed * velocity.y;
		//angle += elapsed;
	}
	void Draw(ShaderProgram *program) {
		model.Identity();
		model.Translate(position.x, position.y, position.z);
		model.Scale(scale.x, scale.y, scale.z);
		model.Rotate(cos(angle));
		program->SetModelMatrix(model);

		float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
		glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
		glEnableVertexAttribArray(program->positionAttribute);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(program->positionAttribute);
	}

	Matrix model;
	Vector3 position;
	Vector3 scale;
	Vector3 velocity;
	Vector3 edges[4];
	float angle = 0.0;
	//std::pair<float, float> edges[4];
	std::pair<float, float> objpoints[4];
};

Object objects[3];

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
	program.Load(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
	projection.SetOrthoProjection(-3.55, 3.55, -2.0f, 2.0f, -1.0f, 1.0f);
	program.SetProjectionMatrix(projection);
	program.SetViewMatrix(view);
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

//Main Functions
void Setup() {
	glViewport(0, 0, 640, 360);
	//set position
	objects[0].position.x = 2.0;
	objects[1].position.x = 0.0;
	objects[2].position.x = -2.0;
	//set velocity
	objects[0].velocity.x = 1.0;
	objects[0].velocity.y = 0.2;
	objects[1].velocity.x = -1.0;
	objects[1].velocity.y = -1.0;
	objects[2].velocity.x = -1.0;
	objects[2].velocity.y = -0.2;
	//set rotation
	objects[0].angle = 3.14/4;
	objects[1].angle = 0.0;
	objects[2].angle = -3.14/4;

	for (Object& object : objects) {
		//-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 
		object.objpoints[0].first = -0.5;
		object.objpoints[0].second = -0.5;
		object.objpoints[1].first = 0.5;
		object.objpoints[1].second = -0.5;
		object.objpoints[2].first = 0.5;
		object.objpoints[2].second = 0.5;
		object.objpoints[3].first = -0.5;
		object.objpoints[3].second = 0.5;
	}

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
	for (Object& object : objects) {
		object.Update(elapsed);
	}
	std::pair<float, float> penetration1;
	std::pair<float, float> penetration2;
	std::pair<float, float> penetration3;

	std::vector<std::pair<float, float>> e1Points;
	std::vector<std::pair<float, float>> e2Points;
	std::vector<std::pair<float, float>> e3Points;

	for (int i = 0; i < 4; i++) {
		objects[0].CalcNorms();
		Vector3 point = objects[0].edges[i] * objects[0].model;
		e1Points.push_back(std::make_pair(point.y, point.x));
	}

	for (int i = 0; i < 4; i++) {
		objects[1].CalcNorms();
		Vector3 point =  objects[1].edges[i] * objects[1].model;
		e2Points.push_back(std::make_pair(point.y, point.x));
	}
	for (int i = 0; i < 4; i++) {
		objects[2].CalcNorms();
		Vector3 point = objects[2].edges[i] * objects[2].model;
		e3Points.push_back(std::make_pair(point.y, point.x));
	}
	bool hit1 = CheckSATCollision(e1Points, e2Points, penetration1);
	bool hit2 = CheckSATCollision(e2Points, e3Points, penetration2);
	bool hit3 = CheckSATCollision(e1Points, e3Points, penetration3);

	if (hit1) {
		objects[0].position.x += (penetration1.first* 0.5);
		objects[0].position.y += (penetration1.second* 0.5);

		objects[1].position.x -= (penetration1.first* 0.5);
		objects[1].position.y -= (penetration1.second* 0.5);
	}
	 if (hit2) {
		objects[1].position.x += (penetration2.first* 0.5);
		objects[1].position.y += (penetration2.second* 0.5);

		objects[2].position.x -= (penetration2.first* 0.5);
		objects[2].position.y -= (penetration2.second* 0.5);
	}
	 if (hit3) {
		objects[0].position.x += (penetration3.first * 0.5);
		objects[0].position.y += (penetration3.second * 0.5);

		objects[2].position.x -= (penetration3.first * 0.5);
		objects[2].position.y -= (penetration3.second * 0.5);
	}

	
}

void Render(ShaderProgram *program) {
	glClear(GL_COLOR_BUFFER_BIT);
	//glClearColor(255 / 255, 150 / 255, 100 / 255, 1.0f);
	for (Object& object : objects) {
		object.Draw(program);
	}
}
