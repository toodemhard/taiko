#pragma once
#include "ui.h"
#include "input.h"
#include <SDL3/SDL.h>

class ui_test {
public:
    ui_test(Input& input, SDL_Renderer* renderer, int width, int height);
    void update();

private:
    Input& m_input;
    SDL_Renderer* m_renderer;

    UI m_ui;
};
