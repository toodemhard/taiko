#pragma once

#include <SDL3/SDL.h>
#include "vec.h"
#include "color.h"

void init_font(SDL_Renderer* renderer);

void draw_text(SDL_Renderer* renderer, const char* text, float size, Vec2 position, RGBA color);

float font_width(const char* text, float font_size);

float font_height(const char* text, float font_size);
