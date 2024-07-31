#pragma once

#include <SDL3/SDL.h>
#include "vec.h"

void init_font(SDL_Renderer* renderer);

void draw_text(SDL_Renderer* renderer, const char* text, Vec2 position);

float font_width(const char* text, float font_size);

float font_height(float font_size);
