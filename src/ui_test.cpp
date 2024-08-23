#include <iostream>

#include "ui_test.h"

ui_test::ui_test(Input& input, SDL_Renderer* renderer, int width, int height) : 
    m_input{ input }, m_renderer{ renderer }, m_ui{ width, height } {}

void ui_test::update() {
    auto st = Style{};
    st.anchor = { 0.5, 0.5 };
    st.border_color = { 255, 255, 255, 255 };

    m_ui.begin_group_v2(st, {
        [](ClickInfo click_info) {
            std::cerr << "selected!\n";
        }
    }); 

    m_ui.rect("Item", {});

    m_ui.button("x", { .border_color{255,0,0,255} }, []() {
        std::cerr << "closed!\n";
    });

    m_ui.end_group_v2();

    m_ui.input(m_input);

    m_ui.draw(m_renderer);
}
