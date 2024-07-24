#pragma once

#include <SDL3/SDL.h>
#include "vec.h"

void init_font(SDL_Renderer* renderer);

void draw_text(SDL_Renderer* renderer, const char* text, Vec2 position);
