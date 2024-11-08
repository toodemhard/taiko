#include <iostream>

#include "ui_test.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_scancode.h"
#include "constants.h"
#include "deflate.h"
#include "font.h"
#include "ui.h"


UI_Test::UI_Test(MemoryAllocators& memory, SDL_Renderer* renderer, Input::Input& input) :
    m_memory(memory), m_renderer{ renderer }, m_input{ input } {}

void UI_Test::update(double delta_time) {
    // Font2::draw_text(m_renderer, "Hujuni", 36, {500, 500}, color::white, 1000000);

    UI ui(m_memory.ui_allocator);

    ui.begin_frame(constants::window_width, constants::window_height);

    ui.text("asdf", {});


    ui.end_frame(m_input);

    ui.draw(m_renderer);
}
