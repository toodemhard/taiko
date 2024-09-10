#include <algorithm>
#include <tracy/Tracy.hpp>

#include "font.h"
#include "ui.h"
#include <variant>

#include "debug_macros.h"

template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};

const char* StringCache::add(std::string&& string) {
    strings[count] = std::move(string);
    count++;
    return strings[count - 1].data();
}

UI::UI(int _screen_width, int _screen_height)
    : screen_width{_screen_width}, screen_height{_screen_height} {}

Vec2& UI::element_scale(ElementHandle e) {
    switch (e.type) {
    case ElementType::group:
        return rects[groups[e.index].rect_index].scale;
    case ElementType::text:
        return texts[e.index].scale;
    };
}

Vec2& UI::element_position(ElementHandle e) {
    switch (e.type) {
    case ElementType::group:
        return rects[groups[e.index].rect_index].position;
    case ElementType::text:
        return texts[e.index].position;
    };
}

void UI::begin_group(const Style& style) {
    ZoneScoped;

    groups.push_back(Group{(int)rects.size() - 1, style});

    if (group_stack.size() > 0) {
        auto& parent_group = groups[group_stack.back()];
        parent_group.children.push_back({ElementType::group, (int)groups.size() - 1});
    }

    group_stack.push_back(groups.size() - 1);
}

void UI::begin_group_button(const Style& style, OnClick&& on_click) {
    ZoneScoped;

    on_click_callbacks.push_back(std::move(on_click));
    groups.push_back(Group{(int)rects.size() - 1, style, on_click_callbacks.size() - 1});

    if (group_stack.size() > 0) {
        auto& parent_group = groups[group_stack.back()];
        parent_group.children.push_back({ElementType::group, (int)groups.size() - 1});
    }

    group_stack.push_back(groups.size() - 1);
}

void UI::end_group_v2() {
    end_group();
}

void UI::end_group() {
    ZoneScoped;

    Group& group = groups[group_stack.back()];
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

    auto width = std::visit(
        overloaded{
            [=](Scale::Auto scale) -> float { return total_width; },
            [=](Scale::Min scale) -> float { return std::max(total_width, scale.value); },
            [](Scale::Fixed scale) -> float { return scale.value; }
        },
        group.style.width
    ) + x_padding;

    auto height = std::visit(
        overloaded{
            [=](Scale::Auto scale) -> float { return total_height; },
            [=](Scale::Min scale) -> float { return std::max(total_height, scale.value); },
            [](Scale::Fixed scale) -> float { return scale.value; }
        },
        group.style.height
    ) + y_padding;

    rects.push_back(Rect{
        {},
        {width, height},
        {group.style.background_color},
        {group.style.border_color},
    });

    group.rect_index = (int)rects.size() - 1;

    if (group_stack.empty()) {

        Vec2 start_pos = std::visit(
            overloaded{
                [=, this](Position::Anchor anchor) {
                    return Vec2{
                        (screen_width - (total_width + x_padding)) * anchor.position.x,
                        (screen_height - (total_height + y_padding)) * anchor.position.y
                    };
                },
                [](Position::Absolute absolute) { return absolute.position; },
                [](Position::Relative relative) { return Vec2{}; }
            },
            group.style.position
        );

        rects[group.rect_index].position = start_pos;
        visit_group(group, start_pos);
    }
}

void UI::visit_group(Group& group, Vec2 start_pos) {
    ZoneScoped;

   auto rect = rects[group.rect_index];
   if (group.click_index.has_value()) {
       click_rects.push_back({start_pos, rect.scale, group.click_index.value() });
   }

   start_pos = start_pos + Vec2{group.style.padding.left, group.style.padding.top};
   for (const auto& e : group.children) {
       auto& position = element_position(e);
       auto& scale = element_scale(e);

       if (e.type == ElementType::group) {
           visit_group(groups[e.index], start_pos);
       }

       position = start_pos;

       if (group.style.stack_direction == StackDirection::Horizontal) {
           start_pos.x += scale.x + group.style.gap;
       } else {
           start_pos.y += scale.y + group.style.gap;
       }
   }
}

bool rect_point_intersect(const Rect& rect, const Vec2& point) {
    const Vec2& p1 = rect.position;
    const Vec2 p2 = rect.position + rect.scale;

    if (point.x > p1.x && point.y > p1.y && point.x < p2.x && point.y < p2.y) {
        return true;
    }

    return false;
}

void UI::input(Input& input) {
    ZoneScoped;

    for (auto& e : click_rects) {
        const Vec2& p1 = e.position;
        const Vec2 p2 = e.position + e.scale;
        if (input.mouse_down(SDL_BUTTON_LEFT) && input.mouse_pos.x > p1.x &&
            input.mouse_pos.y > p1.y && input.mouse_pos.x < p2.x && input.mouse_pos.y < p2.y) {
            on_click_callbacks[e.on_click_index]({});
        }
    }
    click_rects.clear();
    on_click_callbacks.clear();

    for (auto& e : text_fields) {
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
            auto& rect = rects[e.rect_index];
            e.state->focused = rect_point_intersect(rect, input.mouse_pos);
        }
    }
}

void UI::text_field(TextFieldState* state, Style style) {
    ZoneScoped;

    // if (state->focused) {
    //     style.border_color = {255, 255, 255, 255};
    // }
    //
    // text_fields.push_back({
    //     state,
    //     internal_rect(state->text.data(), style),
    // });
    //
    // if (group_stack.size() > 0) {
    //     auto& group = groups[group_stack.back()];
    //     group.children.push_back({ElementType::text_field, (int)text_fields.size() - 1});
    // }
}

void UI::text_rect(const char* text, const Style& style) {
    ZoneScoped;

    this->begin_group(style);

    auto& parent_group = groups[group_stack.back()];
    texts.push_back(Text{
        {},
        Vec2{
            font_width(text, style.font_size),
            font_height(text, style.font_size)
        },
        text,
        style.font_size,
        style.text_color
    });

    parent_group.children.push_back({ElementType::text, (int)texts.size() - 1});

    this->end_group();
}

void UI::slider(float fraction, std::function<void(float)> on_input) {
    // sliders.push_back({Rect{{}, {500, 36}}, fraction, on_input});
    //
    // if (in_group) {
    //     groups[groups.size() - 1].children.push_back({ ElementType::slider, sliders.size() -
    //     1
    //     });
    // } } void UI::text_field(std::string& text, Style style) {
}

void UI::button(const char* text, Style style, OnClick&& on_click) {
    ZoneScoped;

    this->begin_group_button(style, std::move(on_click));

    auto& parent_group = groups[group_stack.back()];
    texts.push_back(Text{
        {},
        Vec2{
            font_width(text, style.font_size),
            font_height(text, style.font_size)
        },
        text,
        style.font_size,
        style.text_color
    });

    parent_group.children.push_back({ElementType::text, (int)texts.size() - 1});

    this->end_group();
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

void UI::draw(SDL_Renderer* renderer) {
    ZoneScoped;

    for (auto& rect : rects) {
        auto frect = SDL_FRect{rect.position.x, rect.position.y, rect.scale.x, rect.scale.y};

        {
            ZoneNamedN(asdf, "draw rect", true);

            auto& bg_color = rect.background_color;
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
            SDL_RenderFillRect(renderer, &frect);
        }

        // SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        // draw_wire_box(renderer, { text_pos.x, text_pos.y, font_width(rect.text,
        // rect.font_size), font_height(rect.text, rect.font_size) });

        {
            ZoneNamedN(askdjh, "draw border", true);
            auto& color = rect.border_color;
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            draw_wire_box(renderer, {rect.position.x, rect.position.y, rect.scale.x, rect.scale.y});
        }
    }

    for (auto& text : texts) {
        draw_text(renderer, text.text, text.font_size, text.position, text.text_color);
    }

    rects.clear();
    groups.clear();
    texts.clear();

    sliders.clear();
    text_fields.clear();
    buttons.clear();

    clicked = false;
}
