#pragma once
#include <vector>

#include <SDL3/SDL.h>
#include "vec.h"
#include "color.h"

void init_font(SDL_Renderer* renderer);

void draw_text(SDL_Renderer* renderer, const char* text, float size, Vec2 position, RGBA color);
void draw_text_cutoff(SDL_Renderer* renderer, const char* text, float font_size, Vec2 position, RGBA color, float max_width);
std::vector<unsigned char> main_atlas();

Vec2 text_dimensions(const char* text, float font_size);

float text_height(const char* text, float font_size);

void kms();
