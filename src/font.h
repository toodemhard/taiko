#pragma once
#include <vector>

#include <SDL3/SDL.h>
#include "vec.h"
#include "color.h"

// void init_font(SDL_Renderer* renderer);
//
// void draw_text(SDL_Renderer* renderer, const char* text, float size, Vec2 position, RGBA color);
// void draw_text_cutoff(SDL_Renderer* renderer, const char* text, float font_size, Vec2 position, RGBA color, float max_width);
//
// Vec2 text_dimensions(const char* text, float font_size);
//
// float text_height(const char* text, float font_size);
//
// void kms();


namespace Font2 {
    void init_fonts(SDL_Renderer* renderer);
    void draw_text(SDL_Renderer* renderer, const char* text, float font_size, Vec2 position, RGBA color, float max_width);
    Vec2 text_dimensions(const char* text, float font_size);
    float text_height(const char* text, float font_size);

    //
    // void save_font_atlas_image();
    // void save_icons();
}
