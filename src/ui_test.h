#pragma once
#include "constants.h"
#include "input.h"
#include "memory.h"
#include "ui.h"
#include <SDL3/SDL.h>

class UI_Test {
  public:
    UI_Test(MemoryAllocators& memory, SDL_Renderer* renderer, Input::Input& input);
    void update(double delta_time);

  private:
    SDL_Renderer* m_renderer;
    MemoryAllocators& m_memory;
    Input::Input& m_input;
    
    TextFieldState m_text_field{};

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

    Slider m_slider{};
    float kys{};
};
