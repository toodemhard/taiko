#include <iostream>

#include "ui_test.h"
#include "SDL3/SDL_scancode.h"
#include "constants.h"
#include "ui.h"

UI_Test::UI_Test(Input& input) : 
    m_input{ input } {}

void UI_Test::update() {
    if (m_input.key_down(SDL_SCANCODE_LEFT) && m_selected > 0) {
        m_selected--;
    }

    if (m_input.key_down(SDL_SCANCODE_RIGHT) && m_selected < elements.size() - 1) {
        m_selected++;
    }

    auto st = Style{};
    st.position = Position::Anchor{ 0.5, 0 };

    m_ui.begin_group(st);
        st = {};
        m_ui.text_rect({"Thing 1"}, st);
        m_ui.text_rect({"Thing 2"}, st);
        m_ui.text_rect({"Thing 3"}, st);
        m_ui.text_rect({"Thing 4"}, st);
    m_ui.end_group();
    

    st = Style{};

    float gap{0};
    float item_width{150};
    float item_height{400};
    auto start_pos = Vec2 { (constants::window_width - item_width) / 2.0f, (constants::window_height - item_height) / 2.0f};

    for(int i = 0; i < elements.size(); i++) {
        auto index_offset = m_selected - i;
        st = {};
        auto offset = index_offset * Vec2{ item_width + gap, 0};
        st.position = Position::Absolute{ start_pos - offset };
        st.border_color = color::white;
        st.width = Scale::Fixed{item_width};
        st.height = Scale::Fixed{item_height};

        if (m_selected == i) {
            st.background_color = color::red;
        }

        m_ui.begin_group(st);

        m_ui.text_rect({elements[i]}, {});

        m_ui.end_group();
    }

    m_ui.input(m_input);
}

void UI_Test::render(SDL_Renderer* renderer) {
    m_ui.draw(renderer);
}
