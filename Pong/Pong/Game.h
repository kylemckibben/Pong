#pragma once
#include "SDL3/SDL.h"


struct Vector2
{
	float x;
	float y;
};

class Game
{
public:
	Game();

	bool Initialize();
	void RunLoop();
	void Shutdown();

private:
	void ProcessInput();
	void UpdateGame();
	void GenerateOutput();

	Uint64 mTicksCount;
	SDL_Window* mWindow;
	SDL_Renderer* mRenderer;
	bool mIsRunning;

	Vector2 mPaddlePos;
	int mPaddleDir;

	Vector2 mBallPos;
	Vector2 mBallVel;
};