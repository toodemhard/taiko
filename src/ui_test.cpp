#include <iostream>

#include "ui_test.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_scancode.h"
#include "constants.h"
#include "ui.h"


UI_Test::UI_Test(Input& input) : 
    m_input{ input } {}

float linear_interp(float v1, float v2, float t) {
    return v1 + (v2 - v1) * t;
}

void UI_Test::update(double delta_time) {
    m_ui.input(m_input);

    auto st = Style{};
    // // st.position = Position::Anchor{0, 0.5};
    // st.position = Position::Absolute{0, 500};
    // st.stack_direction = StackDirection::Vertical;
    //
    // m_ui.begin_group(st);
    // st = {};
    // m_ui.begin_group(st);
    // m_ui.text("resolution", {});
    // m_ui.drop_down_menu(m_selected_resolution_index, m_resolutions, m_resolution_dropdown, [&](int new_selected){
    //     m_selected_resolution_index = new_selected;
    // });
    // m_ui.end_group();
    // m_ui.text("asdlfkjh asdkfh djsafh dalhjsfk jl", {});
    //
    // m_ui.end_group();
    //
    //
    // m_ui.begin_group({.position{Position::Anchor{0.5,0.5}}});
    // m_ui.text("kasfd", {});
    // st = {};
    // st.position = Position::Absolute{500, 500};
    // m_ui.text("fucked off", {});
    // m_ui.text("kasfd", {});
    //
    // m_ui.end_group();


    auto inactive_st = Style{};
    inactive_st.text_color = color::white;
    // inactive_st.text_color = RGBA{160, 160, 160, 255};


    auto anim_st = AnimStyle{
        color::red,
        RGBA{255, 255, 255, 255}
    };

    m_ui.begin_group({.position = Position::Anchor{0.5, 0.5}});
    // m_ui.begin_group_button_anim(&m_button, inactive_st, anim_st, [](ClickInfo info){});
    // m_ui.text("ASkjahsdjkhflkashjfJKHASashdkjHDASKLDJHS", {.text_color = Inherit{}});
    // m_ui.end_group();

    m_ui.button_anim("BUTTON", &m_other_button, inactive_st, anim_st, [](){});

    m_ui.end_group();
    // m_ui.button("THING", {}, [&](ClickInfo info) {
    //     
    //     });
    // m_ui.button("NOTHING", {}, [&](ClickInfo info) {
    //
    //         });


    m_ui.end_frame();
}

void UI_Test::render(SDL_Renderer* renderer) {
    m_ui.draw(renderer);
}
