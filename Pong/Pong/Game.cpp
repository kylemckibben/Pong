#include "Game.h"
#include <random>
#include <string>

// Window
const Uint16 windowWidth = 1024;
const Uint16 windowHeight = 768;

// Wall
const Uint16 thickness = 15;

// Score
const Uint8 winningScore = 11;

// Dividing line ("net")
const Uint16 numSegments = 30;
const float unit = (windowHeight - thickness) / numSegments;
const float segmentWidth = 10.0f;
const float segmentHeight = unit / 2.0f;
const float segmentX = (windowWidth / 2.0f) - (segmentWidth - 2.0f);
SDL_FRect segments[numSegments];

// Paddle
const Uint16 paddleHeight = windowHeight / 8;

// Velocity
// Serve has a range of possible values in the Y direction
const float serveVelocityX = -300.0f;
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dist(-100, 100);
const float serveVelocityY = dist(gen);

// Font
TTF_Font* font;

// Score display
SDL_Surface* p1ScoreSurface;
SDL_Surface* p2ScoreSurface;
SDL_Texture* p1Score;
SDL_Texture* p2Score;
SDL_FRect p1ScoreTextBox;
SDL_FRect p2ScoreTextBox;

// Winner display
SDL_Surface* winnerSurface;
SDL_Texture* winner;
SDL_FRect winnerTextBox;

Game::Game()
{
	mTicksCount = 0;
	mWindow = nullptr;
	mRenderer = nullptr;
	mIsRunning = true;
	mPaddlePosP1.x = 0.0f;
	mPaddlePosP1.y = windowHeight / 2.0f;
	mPaddleDirP1 = 0;
	mPaddlePosP2.x = windowWidth;
	mPaddlePosP2.y = windowHeight / 2.0f;
	mPaddleDirP2 = 0;
	mP1Score = 0;
	mP2Score = 0;
	mBallPos.x = windowWidth / 2.0f;
	mBallPos.y = windowHeight / 2.0f;
	mBallVel.x = serveVelocityX;
	mBallVel.y = serveVelocityY;
	mIsHit = false;
}

/** Initializes the SDL library with specified subsystems (currently just video),
*	followed by the game window and renderer, and finally the "net" segments.
*
*	Returns:
*	- (boolean) true on successful initialization, false on failure
*/
bool Game::Initialize()
{
	bool sdlResult = SDL_InitSubSystem(SDL_INIT_VIDEO);
	if (!sdlResult)
	{
		SDL_Log("Unable to initialize: %s", SDL_GetError());
		return false;
	}

	const char* windowTitle = "Pong";
	const Uint32 windowFlags = 0;

	mWindow = SDL_CreateWindow(windowTitle, windowWidth, windowHeight, windowFlags);
	if (!mWindow)
	{
		SDL_Log("Failed to create window: %s", SDL_GetError());
		return false;
	}

	mRenderer = SDL_CreateRenderer(mWindow, NULL);
	if (!mRenderer)
	{
		SDL_Log("Failed to create renderer: %s", SDL_GetError());
		return false;
	}

	float segmentY = 0.0f + thickness;
	for (int i = 0; i < numSegments; i++)
	{
		SDL_FRect segment{
			segmentX,
			segmentY,
			segmentWidth,
			segmentHeight
		};
		segments[i] = segment;
		segmentY += unit;
	}

	bool ttfResult = TTF_Init();
	if (!ttfResult)
	{
		SDL_Log("Unable to initialize TTF: %s", SDL_GetError());
		return false;
	}
	DrawScores();

	font = TTF_OpenFont("PressStart2P-Regular.ttf", 128);
	if (!font)
		SDL_Log("No font: %s", SDL_GetError());

	return true;
}

/** First main step of game loop. Processes the event queue and keyboard state. */
void Game::ProcessInput()
{
	SDL_Event event;

	// Loop through event queue and process accordingly
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			mIsRunning = false;
			break;
		}
	}

	// Handle keyboard inputs
	const bool* state = SDL_GetKeyboardState(NULL);

	if (state[SDL_SCANCODE_ESCAPE])
		mIsRunning = false;

	// P1 paddle inputs
	mPaddleDirP1 = 0; // reset paddle direction to avoid drift
	if (state[SDL_SCANCODE_W])
		mPaddleDirP1 -= 1;
	if (state[SDL_SCANCODE_S])
		mPaddleDirP1 += 1;

	// P2 paddle inputs
	mPaddleDirP2 = 0; // reset paddle direction to avoid drift
	if (state[SDL_SCANCODE_I])
		mPaddleDirP2 -= 1;
	if (state[SDL_SCANCODE_K])
		mPaddleDirP2 += 1;
}

/** Second main step of game loop. Update game objects as functions
*	of delta time so the game runs at the ideal speed regardles of
*	frame rate.
*/
void Game::UpdateGame()
{
	// Wait until 16ms has elapsed since last frame.
	while (SDL_GetTicks() < mTicksCount + 16);

	// Difference in ticks from last frame (converted to seconds).
	float deltaTime = (SDL_GetTicks() - mTicksCount) / 1000.0f;

	// Clamp delta time value to prevent game from jumping too
	// far forward in a frame.
	if (deltaTime > 0.05f)
		deltaTime = 0.05f;

	// Update the paddle positions with guards in place to prevent going off screen
	// by checking that the center of the paddle remains half the paddle height in
	// bounds accounting for wall thickness.
	if (mPaddleDirP1 != 0)
	{
		mPaddlePosP1.y += mPaddleDirP1 * 300.0f * deltaTime;
		if (mPaddlePosP1.y < (paddleHeight / 2.0f + thickness))
			mPaddlePosP1.y = paddleHeight / 2.0f + thickness;
		else if (mPaddlePosP1.y > (windowHeight - paddleHeight / 2.0f - thickness))
			mPaddlePosP1.y = windowHeight - paddleHeight / 2.0f - thickness;
	}

	if (mPaddleDirP2 != 0)
	{
		mPaddlePosP2.y += mPaddleDirP2 * 300.0f * deltaTime;
		if (mPaddlePosP2.y < (paddleHeight / 2.0f + thickness))
			mPaddlePosP2.y = paddleHeight / 2.0f + thickness;
		else if (mPaddlePosP2.y > (windowHeight - paddleHeight / 2.0f - thickness))
			mPaddlePosP2.y = windowHeight - paddleHeight / 2.0f - thickness;
	}

	// Detect when the ball collides with other game objects and update the
	// direction of velocity.

	// top wall collision
	if (mBallPos.y <= thickness && mBallVel.y < 0.0f)
		mBallVel.y *= -1;

	// bottom wall collision
	if (mBallPos.y >= windowHeight - thickness && mBallVel.y > 0.0f)
		mBallVel.y *= -1;

	// Paddle collisions
	float diffP1 = fabsf(mBallPos.y - mPaddlePosP1.y);
	if (diffP1 <= paddleHeight / 2.0f && 
		(mBallPos.x <= 25.0f && mBallPos.x >= 20.0f) && 
		mBallVel.x < 0.0f
	)
	{
		if (mIsHit == false)
		{
			mBallVel.x *= 2.0f;
			mBallVel.y *= 2.0f;
			mIsHit = true;
		}
		mBallVel.x *= -1.0f;
	}

	float diffP2 = fabsf(mBallPos.y - mPaddlePosP2.y);
	if (diffP2 <= paddleHeight / 2.0f &&
		(mBallPos.x >= windowWidth - 25.0f && mBallPos.x <= windowWidth - 20.0f) &&
		mBallVel.x > 0.0f
		)
	{
		if (mIsHit == false)
		{
			mBallVel.x *= 2.0f;
			mBallVel.y *= 2.0f;
			mIsHit = true;
		}
		mBallVel.x *= -1.0f;
	}

	// Reset ball position if it goes off screen
	if (mBallPos.x < 0.0f)
	{
		mBallPos.x = windowWidth / 2.0f;
		mBallPos.y = windowHeight / 2.0f;
		mIsHit = false;
		mBallVel.x = serveVelocityX;
		mBallVel.y = dist(gen);
		mP2Score++;
	}

	if (mBallPos.x > windowWidth)
	{
		mBallPos.x = windowWidth / 2.0f;
		mBallPos.y = windowHeight / 2.0f;
		mIsHit = false;
		mBallVel.x = -serveVelocityX;
		mBallVel.y = dist(gen);
		mP1Score++;
	}


	// Update the ball position in terms of velocity.
	mBallPos.x += mBallVel.x * deltaTime;
	mBallPos.y += mBallVel.y * deltaTime;

	// Update ticks count for next frame.
	mTicksCount = SDL_GetTicks();
}

/** Final main step in game loop. Utilizes a double buffer to output
*	to the screen by making changes to the renderer and then pushing
*	- calling SDL_RenderPresent - those changes to the screen. This
*	allows generating new output to happen on the same renderer while
*	the most recent complete state is actually displayed, operating
*	as a front and back buffer.
*/
void Game::GenerateOutput()
{
	/* Clear back buffer */

	// Set draw color to black
	SDL_SetRenderDrawColor(
		mRenderer,
		0,	// r
		0,	// g
		0,	// b
		0	// a
	);

	SDL_RenderClear(mRenderer);

	/* Draw game scene */

	// Set draw color to white
	SDL_SetRenderDrawColor(
		mRenderer,
		255,	// r
		255,	// g
		255,	// b
		255		// a
	);

	// Create top wall and render
	SDL_FRect wall{
		0,		// top left x
		0,		// top left y
		windowWidth,
		thickness
	};
	SDL_RenderFillRect(mRenderer, &wall);

	// Update position for bottom wall and render
	wall.y = windowHeight - thickness;
	SDL_RenderFillRect(mRenderer, &wall);

	// Render net
	SDL_RenderFillRects(mRenderer, segments, numSegments);

	// Create ball and render
	SDL_FRect ball{
		mBallPos.x - thickness / 2,
		mBallPos.y - thickness / 2,
		thickness,
		thickness
	};
	SDL_RenderFillRect(mRenderer, &ball);

	// Create paddles and render
	SDL_FRect paddleP1{
		mPaddlePosP1.x + thickness / 2,
		mPaddlePosP1.y - paddleHeight / 2,
		thickness,
		paddleHeight
	};
	SDL_RenderFillRect(mRenderer, &paddleP1);

	SDL_FRect paddleP2{
		mPaddlePosP2.x - thickness * 1.5,
		mPaddlePosP2.y - paddleHeight / 2,
		thickness,
		paddleHeight
	};
	SDL_RenderFillRect(mRenderer, &paddleP2);

	// Draw Scores
	DrawScores();

	// Display winner
	if (mP1Score == winningScore || mP2Score == winningScore) {
		if (mP1Score == winningScore)
			DisplayWinner(windowWidth / 6.0f, windowHeight - (windowHeight / 3.0f));
		else
			DisplayWinner(windowWidth - (windowWidth / 3.0f), windowHeight - (windowHeight / 3.0f));
		mBallVel.x = 0;
		mBallVel.y = 0;
		SDL_SetRenderDrawColor(
			mRenderer,
			0,	// r
			0,	// g
			0,	// b
			0	// a
		);
		SDL_RenderFillRect(mRenderer, &ball);
	}

	/* Swap the buffers */

	SDL_RenderPresent(mRenderer);
}

/** Main game loop that calls each frame step function. */
void Game::RunLoop()
{
	while (mIsRunning)
	{
		ProcessInput();
		UpdateGame();
		GenerateOutput();
	}
}

/** Close SDL after destroying the initialized. */
void Game::Shutdown()
{
	SDL_DestroySurface(p1ScoreSurface);
	SDL_DestroySurface(p2ScoreSurface);
	SDL_DestroySurface(winnerSurface);
	SDL_DestroyTexture(p1Score);
	SDL_DestroyTexture(p2Score);
	SDL_DestroyTexture(winner);
	SDL_DestroyRenderer(mRenderer);
	SDL_DestroyWindow(mWindow);
	TTF_Quit();
	SDL_Quit();
}

/** Handle drawing player scores to screen */
void Game::DrawScores()
{
	SDL_Color white = { 255, 255, 255, 255 };

	// p1 score
	std::string p1ScoreString = std::to_string(mP1Score);
	p1ScoreSurface = TTF_RenderText_Solid(
		font, 
		p1ScoreString.c_str(), 
		p1ScoreString.length(), 
		white);
	p1Score = SDL_CreateTextureFromSurface(mRenderer, p1ScoreSurface);
	p1ScoreTextBox = {
		windowWidth / 4.0f,
		30.0f,
		100.0f,
		100.0f
	};

	// p2 score
	std::string p2ScoreString = std::to_string(mP2Score);
	p2ScoreSurface = TTF_RenderText_Solid(
		font, 
		p2ScoreString.c_str(), 
		p2ScoreString.length(), 
		white);
	p2Score = SDL_CreateTextureFromSurface(mRenderer, p2ScoreSurface);
	p2ScoreTextBox = {
		windowWidth - (windowWidth / 4.0f),
		30.0f,
		100.0f,
		100.0f
	};

	SDL_RenderTexture(mRenderer, p1Score, NULL, &p1ScoreTextBox);
	SDL_RenderTexture(mRenderer, p2Score, NULL, &p2ScoreTextBox);
	SDL_DestroySurface(p1ScoreSurface);
	SDL_DestroySurface(p2ScoreSurface);
	SDL_DestroyTexture(p1Score);
	SDL_DestroyTexture(p2Score);
}

/** Display winning text at a location determined by the player that won. */
void Game::DisplayWinner(float x, float y)
{
	SDL_Color white = { 255, 255, 255, 255 };

	std::string winText = "winner!";
	winnerSurface = TTF_RenderText_Solid(
		font, 
		winText.c_str(), 
		winText.length(), 
		white);
	if (!winnerSurface)
		SDL_Log("No surface: %s", SDL_GetError());

	winner = SDL_CreateTextureFromSurface(mRenderer, winnerSurface);
	if (!winner)
		SDL_Log("No texture: %s", SDL_GetError());

	winnerTextBox = {
		x,
		y,
		256.0f,
		100.0f
	};
	SDL_RenderTexture(mRenderer, winner, NULL, &winnerTextBox);
	SDL_DestroySurface(winnerSurface);
	SDL_DestroyTexture(winner);
}
