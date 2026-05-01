#pragma once
#include "SDL3/SDL.h"
#include "SDL3_ttf/SDL_ttf.h"


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
	void DrawScores();

	Uint64 mTicksCount;
	SDL_Window* mWindow;
	SDL_Renderer* mRenderer;
	bool mIsRunning;

	Vector2 mPaddlePosP1;
	Vector2 mPaddlePosP2;
	int mPaddleDirP1;
	int mPaddleDirP2;

	int mP1Score;
	int mP2Score;

	Vector2 mBallPos;
	Vector2 mBallVel;
	bool mIsHit;
};