#pragma once
#include "constants.h"
#include "input.h"
#include "ui.h"
#include <SDL3/SDL.h>

class UI_Test {
  public:
    UI_Test(Input& input);
    void update(double delta_time);
    void render(SDL_Renderer* renderer);

  private:
    Input& m_input;
    
    TextFieldState m_text_field{};

    UI m_ui{constants::window_width, constants::window_height};

    int m_selected{};
    float m_current_pos{};

    AnimState m_button{};
    AnimState m_other_button{};

    DropDown idk{};
    std::vector<const char*> m_options {"opt1", "opt2", "opt3"};

    std::vector<const char*> elements{"el 1", "el 2", "el 3", "el 4", "el 5", "el 6", "el 7", "el 8"};

    std::vector<const char*> m_resolutions{"1920x1080", "1024x768"};
    DropDown m_resolution_dropdown{};
    int m_selected_resolution_index{};
};
