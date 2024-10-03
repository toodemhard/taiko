#include <iostream>

#include "ui_test.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_scancode.h"
#include "constants.h"
#include "deflate.h"
#include "font.h"
#include "ui.h"


UI_Test::UI_Test(SDL_Renderer* renderer, Input::Input& input) :
    m_renderer{ renderer }, m_input{ input } {}

void UI_Test::update(double delta_time) {
    Font2::draw_text(m_renderer, "Hujuni", 36, {500, 500}, color::white, 1000000);
    // m_ui.input(m_input);
    // m_ui.begin_frame(constants::window_width, constants::window_height);
    //
    //
    // m_ui.end_frame();
    //
    // m_ui.draw(m_renderer);
}
