#pragma once
#include "constants.h"
#include "input.h"
#include "ui.h"
#include <SDL3/SDL.h>

class UI_Test {
  public:
    UI_Test(Input& input);
    void update();
    void render(SDL_Renderer* renderer);

  private:
    Input& m_input;
    
    TextFieldState m_text_field{};

    UI m_ui{constants::window_width, constants::window_height};

    int m_selected{};

    std::vector<const char*> elements{"el 1", "el 2", "el 3", "el 4", "el 5", "el 6", "el 7", "el 8"};
};
