# Pong (a tribute)

This is a Pong clone I am building to begin learning SDL. Pong is a familiar concept, and seems a tried and true "hello, world" as I venture into game programming.  

## Background
This started with the first chapter in "Game Programming in C++: Creating 3D Games" by Sanjay Madhav; the edition of the book I have uses SDL2 but I am using SDL3.
The initial project from the book has 3 walls and just a single player, with additional assignments to add a second player and to add multiple balls. 
I am adding a second player, but not multiple balls, since I am instead choosing to stay true to the original with my clone.

## Current state
2 player support with scoring, game ends on 11 points.
Ball has a range in the Y direction of velocity for the initial serve, and increases velocity upon the first hit after a serve.
There is no menu, to quit you either hit 'esc', click the 'x' on the window, alt+f4, etc.

## Upcoming features
- Menu to start the game
- Release with an executable
- Single player vs NPC support
