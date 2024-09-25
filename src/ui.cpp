#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <optional>
#include <tracy/Tracy.hpp>

#include "SDL3/SDL_mouse.h"
#include "font.h"
#include "ui.h"
#include <format>
#include <variant>

#include "dev_macros.h"

template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};

const char* StringCache::add(std::string&& string) {
    strings.push_back(std::move(string));
    return strings[strings.size() - 1].data();
}

void StringCache::clear() {
    strings.clear();
}

UI::UI(int _screen_width, int _screen_height)
    : m_screen_width{_screen_width}, m_screen_height{_screen_height} {}

RectID UI::begin_row(const Style& style) {
    ZoneScoped;

    m_rects.push_back(Rect{ {}, {}, });
    m_draw_rects.push_back(DrawRect{ style.background_color, style.border_color });

    m_rows.push_back(Row{(int)m_rects.size() - 1, style});

    m_command_tree.push_back(Command::begin_row);

    m_row_stack.push_back(m_rows.size() - 1);

    return m_rects.size() - 1;
}

RectID UI::begin_row_button(const Style& style, OnClick&& on_click) {
    ZoneScoped;

    if (!on_click) {
        DEV_PANIC("empty on_click std::function passed\n");
    }

    auto rect_index = this->begin_row(style);

    m_on_click_callbacks.push_back(std::move(on_click));
    m_click_rects.push_back({ rect_index, (int)m_on_click_callbacks.size() - 1 });

    return rect_index;

}

uint8_t lerp(uint8_t a, uint8_t b, float t) {
    return a + (b - a) * t;
}

RGBA lerp_rgba(RGBA a, RGBA b, float t) {
    return {
        lerp(a.r, b.r, t),
        lerp(a.g, b.g, t),
        lerp(a.b, b.b, t),
        lerp(a.a, b.a, t),
    };
}

void UI::text_headless(const char* text, const Style& style) {
    auto& parent_group = m_rows[m_row_stack.back()];
    RGBA text_color = std::visit(
        overloaded{
            [](RGBA color) { return color; },
            [=](Inherit scale) { return std::get<RGBA>(parent_group.style.text_color); },
        },
        style.text_color
    );

    auto width = font_width(text, style.font_size);
    auto height = font_height(text, style.font_size);

    m_command_tree.push_back(Command::text);
    m_texts.push_back(Text{
        {},
        Vec2{width, height},
        text,
        style.font_size,
        text_color
    });

    add_parent_scale(m_rows[m_row_stack.back()], Vec2{width, height});
}

RectID UI::button_anim(const char* text, AnimState* anim_state, const Style& style, const AnimStyle& anim_style, OnClick&& on_click) {
    ZoneScoped;

    auto rect_index = this->begin_row_button_anim(anim_state, style, anim_style, std::move(on_click));

    auto& parent_group = m_rows[m_row_stack.back()];

    text_headless(text, style);

    this->end_row();

    return rect_index;
}

RectID UI::begin_row_button_anim(AnimState* anim_state, Style style, const AnimStyle& anim_style, OnClick&& on_click) {
    ZoneScoped;

    float new_mix;
    if (anim_style.duration == 0) {
        if (anim_state->target_hover) {
            anim_state->mix = 1;
        } else {
            anim_state->mix = 0;
        }
    } else {
        if (anim_state->target_hover) {
            new_mix = anim_state->mix + 0.002f;
        } else {
            new_mix = anim_state->mix - 0.002f;
        }
        anim_state->mix = std::min(std::max(0.0f, new_mix), 1.0f);
    }

    anim_state->target_hover = false;

    style.text_color = lerp_rgba(
        *std::get_if<RGBA>(&style.text_color), anim_style.alt_text_color, anim_state->mix
    );
    style.background_color = lerp_rgba(style.background_color, anim_style.alt_background_color, anim_state->mix);

    
    auto rect_id = this->begin_row_button(style, std::move(on_click));

    m_hover_rects.push_back(HoverRect{rect_id, *anim_state});

    return rect_id;
}

Rect UI::query_rect(RectID id) {
    return m_rects[id];
}

void UI::add_parent_scale(Row& parent_row, Vec2 scale) {
    auto& rect = m_rects[parent_row.rect_index];

    auto length_axis = [&](const Vec2& scale) {
        switch (parent_row.style.stack_direction) {
        case StackDirection::Horizontal:
            return scale.x;
            break;
        case StackDirection::Vertical:
            return scale.y;
            break;
        }
    };

    auto orthogonal_length = [&](const Vec2& scale) {
        switch (parent_row.style.stack_direction) {
        case StackDirection::Horizontal:
            return scale.y;
            break;
        case StackDirection::Vertical:
            return scale.x;
            break;
        }
    };

    float incr_length = length_axis(scale);
    if (parent_row.children > 0) {
        incr_length += parent_row.style.gap;
    }
    float max_orthogonal_length = std::max(orthogonal_length(rect.scale), orthogonal_length(scale));

    switch (parent_row.style.stack_direction) {
    case StackDirection::Horizontal:
        rect.scale.x += incr_length;
        rect.scale.y = max_orthogonal_length;
        break;
    case StackDirection::Vertical:
        rect.scale.y += incr_length;
        rect.scale.x = max_orthogonal_length;
    }
    parent_row.children++;
}

void UI::end_row() {
    ZoneScoped;

    Row& row = m_rows[m_row_stack.back()];
    m_row_stack.pop_back();
    float total_width;
    float total_height;

    float x_padding = row.style.padding.left + row.style.padding.right;
    float y_padding = row.style.padding.top + row.style.padding.bottom;

    auto& rect = m_rects[row.rect_index];
    auto width = std::visit(
        overloaded{
            [&](Scale::FitContent scale) -> float { return rect.scale.x; },
            [=](Scale::Min scale) -> float { return std::max(rect.scale.x, scale.value); },
            [](Scale::Fixed scale) -> float { return scale.value; }
        }, row.style.width) + x_padding;

    auto height = std::visit(
        overloaded{
            [=](Scale::FitContent scale) -> float { return rect.scale.y; },
            [=](Scale::Min scale) -> float { return std::max(rect.scale.y, scale.value); },
            [](Scale::Fixed scale) -> float { return scale.value; }
        }, row.style.height) + y_padding;

    rect.scale = {width, height};

    if (m_row_stack.size() > 0 && std::get_if<Position::Relative>(&row.style.position) != nullptr) {
        add_parent_scale(m_rows[m_row_stack.back()], Vec2{width, height});
    }

    m_command_tree.push_back(Command::end_row);
}

struct RowStackFrame {
    int row_index;
    Vec2 next_position;
    StackDirection stack_direction;
};

bool rect_point_intersect(Vec2 point, Vec2 position, Vec2 scale) {
    const Vec2& p1 = position;
    const Vec2 p2 = position + scale;

    if (point.x >= p1.x && point.y >= p1.y && point.x <= p2.x && point.y <= p2.y) {
        return true;
    }

    return false;
}

void UI::input(Input& input) {
    ZoneScoped;

    m_input_called = true;

    if (input.mouse_up(SDL_BUTTON_LMASK)) {
        for (auto& callback : m_slider_on_release_callbacks) {
            callback();
        }
    }

    m_slider_on_release_callbacks.clear();

    for (auto& e : m_click_rects) {
        auto rect = m_rects[e.rect_index];
        if (input.mouse_down(SDL_BUTTON_LEFT) && rect_point_intersect(input.mouse_pos, rect.position, rect.scale)) {
            m_on_click_callbacks[e.on_click_index]();
        }
    }
    m_click_rects.clear();
    m_on_click_callbacks.clear();
    m_dropdown_on_select_callbacks.clear();

    for (auto& e : m_hover_rects) {
        auto rect = m_rects[e.rect_index];
        if (rect_point_intersect(input.mouse_pos, rect.position, rect.scale)) {
            e.anim_state.target_hover = true;
        }
    }
    m_hover_rects.clear();

    for (auto& e : m_slider_input_rects) {
        auto& rect = m_rects[e.rect_index];
        auto offset_position = input.mouse_pos - rect.position;
        auto new_fraction = std::min(std::max(0.0f, offset_position.x / rect.scale.x), 1.0f);
        m_slider_on_held_callbacks[e.on_held_index](new_fraction);
    }
    m_slider_input_rects.clear();
    m_slider_on_held_callbacks.clear();

    for (auto& callback : m_slider_user_callbacks) {
        callback();
    }
    m_slider_user_callbacks.clear();

    for (auto& e : m_text_field_inputs) {
        if (e.state->focused) {
            auto& text = e.state->text;
            if (input.key_down_repeat(SDL_SCANCODE_BACKSPACE) && text.length() > 0) {
                if (input.modifier(SDL_KMOD_LCTRL)) {
                    if (text.back() == ' ') {
                        int i = text.length() - 2;
                        while (i >= 0 && text.at(i) == ' ') {
                            i--;
                        }

                        text.erase(text.begin() + (i + 1), text.end());
                    } else {
                        int i = text.length() - 2;
                        while (i >= 0 && text.at(i) != ' ') {
                            i--;
                        }

                        text.erase(text.begin() + (i + 1), text.end());
                    }

                } else {
                    text.pop_back();
                }
            }

            if (input.input_text.has_value()) {
                e.state->text += input.input_text.value();
            }
        }

        if (input.mouse_down(SDL_BUTTON_LEFT)) {
            auto& rect = m_rects[e.rect_index];
            e.state->focused = rect_point_intersect(input.mouse_pos, rect.position, rect.scale);
        }
    }
    m_text_field_inputs.clear();
    m_rects.clear();
}

void UI::begin_frame(int width, int height) {
    if (!m_input_called) {
        DEV_PANIC("UI::input() not called");
    }
    m_input_called = false;

    this->begin_row({.width=Scale::Fixed{(float)width}, .height=Scale::Fixed{(float)height}});

    m_begin_frame_called = true;
}

void UI::end_frame() {
    if (!m_begin_frame_called) {
        DEV_PANIC("UI::begin_frame() not called");
    }
    m_begin_frame_called = false;

    if (m_row_stack.size() > 1) {
        DEV_PANIC("UI::begin_group() not closed")
    }
    m_end_frame_called = true;

    this->end_row();


    // add open dropdown on top of everything

    auto active_st = Style{};
    active_st.background_color = color::white;
    active_st.text_color = color::black;

    if (m_post_overlay.has_value()) {
        auto& overlay = m_post_overlay.value();

        auto& hook = m_rects[overlay.rect_hook_index];
        auto g_st = Style{};
        g_st.position = Position::Absolute{hook.position + Vec2{0, hook.scale.y}};
        g_st.width = Scale::Fixed{hook.scale.x};
        g_st.stack_direction = StackDirection::Vertical;
        g_st.background_color = color::red;
        this->begin_row(g_st);
        auto& options = *overlay.items;
        for (int i = 0; i < options.size(); i++) {
            auto st = (i == overlay.selected_index) ? active_st : Style{};

            this->button(
                options[i],
                st,
                [this, i, on_click_index = overlay.on_click_index]() {
                    m_dropdown_on_select_callbacks[on_click_index](i);
                }
            );
        }
        this->end_row();

        m_post_overlay = std::nullopt;
    }


    // position the whole tree from root

    auto incr = [](RowStackFrame& parent, Vec2 scale, float gap) {
        if (parent.stack_direction == StackDirection::Horizontal) {
            parent.next_position.x += scale.x + gap;
        } else if (parent.stack_direction == StackDirection::Vertical) {
            parent.next_position.y += scale.y + gap;
        }
    };

    int current_row_index = 0;
    int current_text_index = 0;

    std::vector<RowStackFrame> row_stack;
    for (auto command : m_command_tree) {
        switch (command) {
        case Command::begin_row: {
            auto& current_row = m_rows[current_row_index];
            auto& current_rect = m_rects[current_row.rect_index];

            RowStackFrame new_row{current_row_index};
            new_row.next_position = {current_row.style.padding.left, current_row.style.padding.top};
            new_row.stack_direction = current_row.style.stack_direction;

            if (row_stack.size() > 0) {
                auto& parent_row = m_rows[row_stack.back().row_index];
                auto parent_rect = m_rects[parent_row.rect_index];

                current_rect.position = std::visit(overloaded {
                    [&](Position::Anchor anchor) {
                        float x_padding = parent_row.style.padding.left + parent_row.style.padding.right;
                        float y_padding = parent_row.style.padding.top + parent_row.style.padding.bottom;
                        return Vec2{
                            parent_row.style.padding.left + parent_rect.position.x + (parent_rect.scale.x - current_rect.scale.x - x_padding) * anchor.position.x,
                            parent_row.style.padding.top + parent_rect.position.y + (parent_rect.scale.y - current_rect.scale.y - y_padding) * anchor.position.y,
                        };
                    },
                    [](Position::Absolute absolute) { return absolute.position; },
                    [&](Position::Relative relative) {
                        auto pos = row_stack.back().next_position;
                        if (current_row.style.alignment == Alignment::Right) {
                            if (parent_row.style.stack_direction == StackDirection::Vertical) {
                                pos.x = pos.x + parent_rect.scale.x - current_rect.scale.x;
                            }

                        }
                        incr(row_stack.back(), m_rects[current_row.rect_index].scale, parent_row.style.gap);
                        return pos; 
                    }
                }, current_row.style.position);

                new_row.next_position += current_rect.position;
            }

            row_stack.push_back(new_row);
            auto row = row_stack[row_stack.size() - 1];

            current_row_index++;
        }
        break;
        case Command::end_row:
            row_stack.pop_back();
        break;
        case Command::text:
            m_texts[current_text_index].position = row_stack.back().next_position;
            incr(row_stack.back(), m_texts[current_text_index].scale, 0);

            current_text_index++;
        break;
        }
    }

    m_command_tree.clear();
}

void UI::text_field(TextFieldState* state, Style style) {
    ZoneScoped;

    if (state->focused) {
        style.border_color = {255, 255, 255, 255};
    }

    auto rect_id = this->text(state->text.data(), style);

    m_text_field_inputs.push_back({
        state,
        rect_id
    });
}

RectID UI::text(const char* text, const Style& style) {
    ZoneScoped;

    auto rect_id = this->begin_row(style);

    this->text_headless(text, style);

    this->end_row();

    return rect_id;
}

void UI::slider(Slider& state, SliderStyle style, float fraction, SliderCallbacks&& callbacks) {
    auto container = Style{};
    container.position = style.position;
    container.width = Scale::Fixed{style.width};
    container.height = Scale::Fixed{style.height};
    container.border_color = color::white;

    if (state.held) {
        m_slider_on_release_callbacks.push_back([&, on_release = std::move(callbacks.on_release)]() {
            state.held = false;
            if (on_release.has_value()) {
                m_slider_user_callbacks.push_back(std::move(on_release.value()));
            }
        });
    }


    auto group = Row{};
    group.style = container;

    m_on_click_callbacks.push_back([&, onclick = std::move(callbacks.on_click)]() {
        state.held = true;
        if (onclick.has_value()) {
            m_slider_user_callbacks.push_back(std::move(onclick.value()));
        }
    });

    auto rect_id = this->begin_row(container);

    m_click_rects.push_back({rect_id, (int)m_on_click_callbacks.size() - 1});
    if (state.held) {
        m_slider_on_held_callbacks.push_back(std::move(callbacks.on_input));
        m_slider_input_rects.push_back({rect_id, (int)m_slider_on_held_callbacks.size() - 1});
    }

    auto filling = Style{};
    filling.background_color = style.fg_color;
    filling.width = Scale::Fixed{style.width * fraction};
    filling.height = Scale::Fixed{style.height};
    this->begin_row(filling);
    this->end_row();

    this->end_row();

    if (state.held) {
        auto rect = m_rects[rect_id];
    }
}

void UI::drop_down_menu(
    int selected_opt_index,
    std::vector<const char*>&& options,
    DropDown& state,
    std::function<void(int)> on_input
) {
    m_options = std::move(options);
    drop_down_menu(selected_opt_index, m_options, state, on_input);
}

void UI::drop_down_menu(
    int selected_opt_index,
    std::vector<const char*>& options,
    DropDown& state,
    std::function<void(int)> on_input
) {

    auto st = Style{};
    st.position = Position::Anchor{0.5, 0};
    st.stack_direction = StackDirection::Vertical;

    m_on_click_callbacks.push_back([&]() { state.clicked_last_frame = true; });

    auto rect_index = this->begin_row(st);
    m_click_rects.push_back({rect_index, (int)m_on_click_callbacks.size() - 1 });
    auto thing = (state.menu_dropped)
                     ? strings.add(std::format("{} \\/", options[selected_opt_index]))
                     : strings.add(std::format("{} <", options[selected_opt_index]));
    auto ref = this->button(thing, {}, [&]() {
        state.menu_dropped = !state.menu_dropped;
        state.clicked_last_frame = true;
    });

    m_click_rects.push_back({rect_index, (int)m_on_click_callbacks.size() - 1});

    m_dropdown_on_select_callbacks.push_back(std::move(on_input));

    if (state.menu_dropped) {
        m_post_overlay = DropDownOverlay{
            ref,
            (int)m_dropdown_on_select_callbacks.size() - 1,
            selected_opt_index,
            &options,
        };
    }

    this->end_row();
}

RectID UI::button(const char* text, Style style, OnClick&& on_click) {
    ZoneScoped;

    auto rect_index = this->begin_row_button(style, std::move(on_click));

    auto& parent_group = m_rows[m_row_stack.back()];

    RGBA text_color = std::visit(
        overloaded{
            [](RGBA color) { return color; },
            [=](Inherit scale) { return std::get<RGBA>(parent_group.style.text_color); },
        },
        style.text_color
    );

    text_headless(text, style);

    this->end_row();

    return rect_index;
}

SDL_FPoint vec2_to_sdl_fpoint(const Vec2& vec) {
    return {vec.x, vec.y};
}

void draw_wire_box(SDL_Renderer* renderer, const SDL_FRect& rect) {
    SDL_FPoint points[5];
    const Vec2& pos = {rect.x, rect.y};
    const Vec2& scale = {rect.w, rect.h};
    points[0] = vec2_to_sdl_fpoint(pos);
    points[1] = vec2_to_sdl_fpoint(pos + Vec2{1, 0} * scale);
    points[2] = vec2_to_sdl_fpoint(pos + scale);
    points[3] = vec2_to_sdl_fpoint(pos + Vec2{0, 1} * scale);
    points[4] = points[0];

    SDL_RenderLines(renderer, points, 5);
}

#define FUNCTION_NAME(func) #func
void UI::draw(SDL_Renderer* renderer) {
    ZoneScoped;

    if (!m_end_frame_called) {
        DEV_PANIC("UI::end_frame() not called");
    }
    m_end_frame_called = false;

    for (int i = m_draw_rects.size() - 1; i >= 0; i--) {
        auto draw_rect = m_draw_rects[i];
        auto& rect = m_rects[i];
        auto frect = SDL_FRect{rect.position.x, rect.position.y, rect.scale.x, rect.scale.y};

        {
            ZoneNamedN(asdf, "draw rect", true);

            auto& bg_color = draw_rect.background_color;
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
            SDL_RenderFillRect(renderer, &frect);
        }

        {
            ZoneNamedN(askdjh, "draw border", true);
            auto& color = draw_rect.border_color;
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            draw_wire_box(renderer, {rect.position.x, rect.position.y, rect.scale.x, rect.scale.y});
        }
    }

    for (auto& text : m_texts) {
        draw_text(renderer, text.text, text.font_size, text.position, text.text_color);
    }

    m_draw_rects.clear();
    m_rows.clear();
    m_texts.clear();

    strings.clear();

    clicked = false;
}
