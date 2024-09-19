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
    : screen_width{_screen_width}, screen_height{_screen_height} {}

Vec2& UI::element_scale(ElementHandle e) {
    switch (e.type) {
    case ElementType::group:
        return m_rects[m_groups[e.index].rect_index].scale;
    case ElementType::text:
        return m_texts[e.index].scale;
    };
}

Vec2& UI::element_position(ElementHandle e) {
    switch (e.type) {
    case ElementType::group:
        return m_rects[m_groups[e.index].rect_index].position;
    case ElementType::text:
        return m_texts[e.index].position;
    };
}

void UI::begin_group(const Style& style) {
    ZoneScoped;

    m_groups.push_back(Group{(int)m_rects.size() - 1, style});

    if (group_stack.size() > 0) {
        auto& parent_group = m_groups[group_stack.back()];
        parent_group.children.push_back({ElementType::group, (int)m_groups.size() - 1});
    }

    group_stack.push_back(m_groups.size() - 1);
}

void UI::begin_group_any(const Group& group) {
    m_groups.push_back(group);

    if (group_stack.size() > 0) {
        auto& parent_group = m_groups[group_stack.back()];
        parent_group.children.push_back({ElementType::group, (int)m_groups.size() - 1});
    }

    group_stack.push_back(m_groups.size() - 1);
}

void UI::begin_group_button(const Style& style, OnClick&& on_click) {
    ZoneScoped;

    on_click_callbacks.push_back(std::move(on_click));
    auto group = Group{};
    group.style = style;
    group.on_click_index = on_click_callbacks.size() - 1;

    this->begin_group_any(group);
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
    // RGBA text_color = std::visit(
    //     overloaded{
    //         [](RGBA color) { return color; },
    //         [=](Inherit scale) { return std::get<RGBA>(parent_group.style.text_color); },
    //     },
    //     style.text_color
    // );

RectID UI::button_anim(const char* text, AnimState* anim_state, const Style& style, const AnimStyle& anim_style, OnClick&& on_click) {
    ZoneScoped;

    this->begin_group_button_anim(anim_state, style, anim_style, std::move(on_click));

    auto& parent_group = m_groups[group_stack.back()];

    RGBA text_color = *std::get_if<RGBA>(&parent_group.style.text_color);

    m_texts.push_back(Text{
        {},
        Vec2{font_width(text, style.font_size), font_height(text, style.font_size)},
        text,
        style.font_size,
        text_color
    });

    parent_group.children.push_back({ElementType::text, (int)m_texts.size() - 1});

    return this->end_group();
}

void UI::begin_group_button_anim(AnimState* anim_state, Style style, const AnimStyle& anim_style, OnClick&& on_click) {
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

    on_click_callbacks.push_back(std::move(on_click));

    auto group = Group{};
    group.style = style;
    group.on_click_index = on_click_callbacks.size() - 1;

    group.anim_state = anim_state;

    this->begin_group_any(group);
}

Rect UI::query_rect(RectID id) {
    return m_rects[id];
}

RectID UI::end_group() {
    ZoneScoped;

    Group& group = m_groups[group_stack.back()];
    group_stack.pop_back();

    auto length_axis = [](StackDirection stack_direction) -> float (*)(const Vec2&) {
        switch (stack_direction) {
        case StackDirection::Horizontal:
            return [](const Vec2& scale) -> float { return scale.x; };
            break;
        case StackDirection::Vertical:
            return [](const Vec2& scale) -> float { return scale.y; };
            break;
        }
    }(group.style.stack_direction);

    auto orthogonal_length = [](StackDirection stack_direction) -> float (*)(const Vec2&) {
        switch (stack_direction) {
        case StackDirection::Horizontal:
            return [](const Vec2& scale) -> float { return scale.y; };
            break;
        case StackDirection::Vertical:
            return [](const Vec2& scale) -> float { return scale.x; };
            break;
        }
    }(group.style.stack_direction);

    float max_orthogonal_length = 0;
    float total_length = group.style.gap * (group.children.size() - 1);
    for (auto& e : group.children) {
        auto& scale = element_scale(e);
        total_length += length_axis(scale);
        max_orthogonal_length = std::max(max_orthogonal_length, orthogonal_length(scale));
    }

    float total_width;
    float total_height;

    switch (group.style.stack_direction) {
    case StackDirection::Horizontal:
        total_width = total_length;
        total_height = max_orthogonal_length;
        break;
    case StackDirection::Vertical:
        total_width = max_orthogonal_length;
        total_height = total_length;
        break;
    }

    float x_padding = group.style.padding.left + group.style.padding.right;
    float y_padding = group.style.padding.top + group.style.padding.bottom;

    auto width =
        std::visit(
            overloaded{
                [=](Scale::FitContent scale) -> float { return total_width; },
                [=](Scale::Min scale) -> float { return std::max(total_width, scale.value); },
                [](Scale::Fixed scale) -> float { return scale.value; }
            },
            group.style.width
        ) +
        x_padding;

    auto height =
        std::visit(
            overloaded{
                [=](Scale::FitContent scale) -> float { return total_height; },
                [=](Scale::Min scale) -> float { return std::max(total_height, scale.value); },
                [](Scale::Fixed scale) -> float { return scale.value; }
            },
            group.style.height
        ) +
        y_padding;

    m_rects.push_back(Rect{
        {},
        {width, height},
    });
    m_draw_rects.push_back(DrawRect{
        (int)m_rects.size() - 1,
        {group.style.background_color},
        {group.style.border_color},
    });

    group.rect_index = (int)m_rects.size() - 1;

    if (group_stack.empty()) {

        Vec2 start_pos = std::visit(
            overloaded{
                [=, this](Position::Anchor anchor) {
                    return Vec2{
                        (screen_width - (width + x_padding)) * anchor.position.x,
                        (screen_height - (height + y_padding)) * anchor.position.y
                    };
                },
                [](Position::Absolute absolute) { return absolute.position; },
                [](Position::Relative relative) { return Vec2{}; }
            },
            group.style.position
        );

        m_rects[group.rect_index].position = start_pos;
        visit_group(group, start_pos);
    }

    return group.rect_index;
}

void UI::visit_group(Group& group, Vec2 start_pos) {
    ZoneScoped;

    auto rect = m_rects[group.rect_index];

    if (group.anim_state != nullptr) {
        m_hover_rects.push_back({start_pos, rect.scale, *group.anim_state});
    }

    if (group.on_click_index.has_value()) {
        m_click_rects.push_back({start_pos, rect.scale, group.on_click_index.value()});
    }

    if (group.silder_on_held_index.has_value()) {
        m_slider_input_rects.push_back({start_pos, rect.scale, group.silder_on_held_index.value()});
    }

    start_pos = start_pos + Vec2{group.style.padding.left, group.style.padding.top};
    for (const auto& e : group.children) {
        auto& position = element_position(e);
        auto& scale = element_scale(e);

        if (e.type == ElementType::group) {
            visit_group(m_groups[e.index], start_pos);
        }

        position = start_pos;

        if (group.style.stack_direction == StackDirection::Horizontal) {
            start_pos.x += scale.x + group.style.gap;
        } else {
            start_pos.y += scale.y + group.style.gap;
        }
    }
}

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

    if (input.mouse_up(SDL_BUTTON_LMASK)) {
        for (auto& callback : on_release_callbacks) {
            callback();
        }
    }

    on_release_callbacks.clear();

    for (auto& e : m_click_rects) {
        if (input.mouse_down(SDL_BUTTON_LEFT) &&
            rect_point_intersect(input.mouse_pos, e.position, e.scale)) {
            auto info = ClickInfo{input.mouse_pos - e.position, e.scale};

            on_click_callbacks[e.on_click_index]();
        }
    }
    m_click_rects.clear();
    on_click_callbacks.clear();
    dropdown_on_select_callbacks.clear();

    for (auto& e : m_hover_rects) {
        if (rect_point_intersect(input.mouse_pos, e.position, e.scale)) {
            e.anim_state.target_hover = true;
        }
    }
    m_hover_rects.clear();

    for (auto& e : m_slider_input_rects) {
        auto offset_position = input.mouse_pos - e.position;
        auto new_fraction = std::min(std::max(0.0f, offset_position.x / e.scale.x), 1.0f);
        m_slider_on_held_callbacks[e.on_held_index](new_fraction);
    }
    m_slider_input_rects.clear();
    m_slider_on_held_callbacks.clear();

    for (auto& callback : user_callbacks) {
        callback();
    }
    user_callbacks.clear();


    // for (auto& state : dropdown_clickoff_callbacks) {
    //     if (input.mouse_down(SDL_BUTTON_LMASK) && !state->clicked_last_frame) {
    //         state->menu_dropped = false;
    //     }
    //
    //     state->clicked_last_frame = false;
    //
    // }

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

void UI::end_frame() {
    if (group_stack.size() > 0) {
        DEV_PANIC("UI::begin_group() not closed")
    }
    m_end_frame_called = true;

    auto active_st = Style{};
    active_st.background_color = color::white;
    active_st.text_color = color::black;

    if (post_overlay.has_value()) {
        auto& overlay = post_overlay.value();

        auto& hook = m_rects[overlay.rect_hook_index];
        auto g_st = Style{};
        g_st.position = Position::Absolute{hook.position + Vec2{0, hook.scale.y}};
        g_st.width = Scale::Fixed{hook.scale.x};
        g_st.stack_direction = StackDirection::Vertical;
        g_st.background_color = color::red;
        this->begin_group(g_st);
        auto& options = *overlay.items;
        for (int i = 0; i < options.size(); i++) {
            auto st = (i == overlay.selected_index) ? active_st : Style{};

            this->button(
                options[i],
                st,
                [this, i, on_click_index = overlay.on_click_index]() {
                    dropdown_on_select_callbacks[on_click_index](i);
                }
            );
        }
        this->end_group();

        post_overlay = std::nullopt;
    }
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
    //
    // if (group_stack.size() > 0) {
    //     auto& group = groups[group_stack.back()];
    //     group.children.push_back({ElementType::text_field, (int)text_fields.size() - 1});
    // }
}

RectID UI::text(const char* text, const Style& style) {
    ZoneScoped;

    auto& parent_group = m_groups[group_stack.back()];
    RGBA text_color = std::visit(
        overloaded{
            [](RGBA color) { return color; },
            [=](Inherit scale) { return std::get<RGBA>(parent_group.style.text_color); },
        },
        style.text_color
    );

    this->begin_group(style);
    auto& text_group = m_groups[group_stack.back()];

    m_texts.push_back(Text{
        {},
        Vec2{font_width(text, style.font_size), font_height(text, style.font_size)},
        text,
        style.font_size,
        text_color
    });

    text_group.children.push_back({ElementType::text, (int)m_texts.size() - 1});

    return this->end_group();
}

void UI::slider(Slider& state, SliderStyle style, float fraction, SliderCallbacks&& callbacks) {
    auto container = Style{};
    container.position = style.position;
    container.width = Scale::Fixed{style.width};
    container.height = Scale::Fixed{style.height};
    container.border_color = color::white;

    if (state.held) {
        on_release_callbacks.push_back([&, on_release = std::move(callbacks.on_release)]() {
            state.held = false;
            if (on_release.has_value()) {
                user_callbacks.push_back(std::move(on_release.value()));
            }
        });
    }


    auto group = Group{};
    group.style = container;

    on_click_callbacks.push_back([&, onclick = std::move(callbacks.on_click)]() {
        state.held = true;
        if (onclick.has_value()) {
            user_callbacks.push_back(std::move(onclick.value()));
        }
    });

    group.on_click_index = on_click_callbacks.size() - 1;

    if (state.held) {
        m_slider_on_held_callbacks.push_back(std::move(callbacks.on_input));
        group.silder_on_held_index = m_slider_on_held_callbacks.size() - 1;
    }

    this->begin_group_any(group);

    auto filling = Style{};
    filling.background_color = style.fg_color;
    filling.width = Scale::Fixed{style.width * fraction};
    filling.height = Scale::Fixed{style.height};
    this->begin_group(filling);

    this->end_group();

    auto rect_id = this->end_group();
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

    auto group = Group{};
    on_click_callbacks.push_back([&]() { state.clicked_last_frame = true; });
    group.on_click_index = on_click_callbacks.size() - 1;
    group.style = st;

    this->begin_group_any(group);
    auto thing = (state.menu_dropped)
                     ? strings.add(std::format("{} \\/", options[selected_opt_index]))
                     : strings.add(std::format("{} <", options[selected_opt_index]));
    auto ref = this->button(thing, {}, [&]() {
        state.menu_dropped = !state.menu_dropped;
        state.clicked_last_frame = true;
    });

    dropdown_on_select_callbacks.push_back(std::move(on_input));

    if (state.menu_dropped) {
        post_overlay = DropDownOverlay{
            ref,
            (int)dropdown_on_select_callbacks.size() - 1,
            selected_opt_index,
            &options,
        };
    }

    this->end_group();
}

RectID UI::button(const char* text, Style style, OnClick&& on_click) {
    ZoneScoped;

    this->begin_group_button(style, std::move(on_click));

    auto& parent_group = m_groups[group_stack.back()];

    RGBA text_color = std::visit(
        overloaded{
            [](RGBA color) { return color; },
            [=](Inherit scale) { return std::get<RGBA>(parent_group.style.text_color); },
        },
        style.text_color
    );

    m_texts.push_back(Text{
        {},
        Vec2{font_width(text, style.font_size), font_height(text, style.font_size)},
        text,
        style.font_size,
        text_color
    });

    parent_group.children.push_back({ElementType::text, (int)m_texts.size() - 1});

    return this->end_group();
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
        auto& rect = m_rects[draw_rect.rect_index];
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
    m_groups.clear();
    m_texts.clear();

    strings.clear();

    clicked = false;
}
