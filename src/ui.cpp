#include <tracy/Tracy.hpp>

#include "font.h"
#include "ui.h"
#include <format>
#include <iostream>

#include "debug_macros.h"


UI::UI(int _screen_width, int _screen_height) : screen_width{ _screen_width }, screen_height{ _screen_height } {}

Rect& UI::element_rect(ElementHandle e) {
    switch (e.type) {
    case ElementType::slider:
        return sliders[e.index].rect;
    case ElementType::button:
        return rects[buttons[e.index].rect_index];
    case ElementType::rect:
        return rects[e.index];
    case ElementType::group:
        return rects[groups[e.index].rect_index];
    case ElementType::text_field:
        return rects[text_fields[e.index].rect_index];
    default:
        DEBUG_PANIC("didn't add rect getter for an element type");
    };
}

void UI::begin_group(const Style& style) {
    ZoneScoped;
    groups.push_back(Group{ internal_rect(nullptr, style), style });

    if (group_stack.size() > 0) {
        auto& parent_group = groups[group_stack.back()];
        parent_group.children.push_back({ ElementType::group, (int) groups.size() - 1 });
    }

    group_stack.push_back(groups.size() - 1);
}

void UI::begin_group_v2(const Style& style, const Properties& properties) {
    ZoneScoped;


    groups.push_back(Group{ internal_rect(nullptr, style), style, {}, properties.on_click });

    if (group_stack.size() > 0) {
        auto& parent_group = groups[group_stack.back()];
        parent_group.children.push_back({ ElementType::group, (int) groups.size() - 1 });
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

    auto length_axis = [&]() -> float(*)(const Rect&) {
        switch (group.style.stack_direction) {
        case StackDirection::Horizontal:
            return [](const Rect& rect) -> float {
                return rect.scale.x;
                };
        break;
        case StackDirection::Vertical:
            return [](const Rect& rect) -> float {
                return rect.scale.y;
                };
        break;
        }
    }();

    auto max_orthoganal_length = [&]() -> void(*)(float&, const Rect&) {
        switch (group.style.stack_direction) {
        case StackDirection::Horizontal:
            return [](float& max_height, const Rect& next_rect){
                if (next_rect.scale.y > max_height) {
                    max_height = next_rect.scale.y;
                }
                };
        break;
        case StackDirection::Vertical:
            return [](float& max_width, const Rect& next_rect){
                if (next_rect.scale.x > max_width) {
                    max_width = next_rect.scale.x;
                }
                };
        break;
        }
    }();

    float max_length = 0;
	float total_length = group.style.gap * (group.children.size() - 1);
	for (auto& e: group.children) {
        Rect& rect = element_rect(e);
        total_length += length_axis(rect);

        max_orthoganal_length(max_length, rect);
	}

    float total_width;
    float total_height;

    switch (group.style.stack_direction) {
    case StackDirection::Horizontal:
        total_width = total_length;
        total_height = max_length;
        break;
    case StackDirection::Vertical:
        total_width = max_length;
        total_height = total_length;
        break;
    }

    float x_padding = group.style.padding.left + group.style.padding.right;
    float y_padding = group.style.padding.top + group.style.padding.bottom;


    auto& group_rect = rects[group.rect_index];
    group_rect.scale = group_rect.scale + Vec2{ total_width, total_height };

    if (group_stack.empty()) {
        Vec2 start_pos = { (screen_width - (total_width + x_padding)) * group.style.anchor.x, (screen_height - (total_height + y_padding)) * group.style.anchor.y };
        rects[group.rect_index].position = start_pos;
        visit_group(group, start_pos);
    }
}

void UI::visit_group(Group& group, Vec2 start_pos) {
    ZoneScoped;

    if (group.click_property != nullptr) {
        click_rects.push_back({ start_pos, rects[group.rect_index].scale, group.click_property });
    }

    start_pos = start_pos + Vec2{ group.style.padding.left, group.style.padding.top };
    for (auto& e : group.children) {
        Rect& rect = element_rect(e);


        if (e.type == ElementType::group) {
            visit_group(groups[e.index], start_pos);
        }

        rect.position = start_pos;

        if (group.style.stack_direction == StackDirection::Horizontal) {
            start_pos.x += rect.scale.x + group.style.gap;
        }
        else {
            start_pos.y += rect.scale.y + group.style.gap;
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

    for (auto& button : buttons) {
        const auto& rect = rects[button.rect_index];
        const Vec2& p1 = rect.position;
        const Vec2 p2 = rect.position + rect.scale;

        if (input.mouse_down(SDL_BUTTON_LEFT) && input.mouse_pos.x > p1.x && input.mouse_pos.y > p1.y && input.mouse_pos.x < p2.x && input.mouse_pos.y < p2.y) {
            button.on_click();
            clicked = true;
        }
    }

    for (auto& e : click_rects) {
        const Vec2& p1 = e.position;
        const Vec2 p2 = e.position + e.scale;
        if (input.mouse_down(SDL_BUTTON_LEFT) && input.mouse_pos.x > p1.x && input.mouse_pos.y > p1.y && input.mouse_pos.x < p2.x && input.mouse_pos.y < p2.y) {
            e.on_click({});
        }
    }
    click_rects.clear();

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
                    }
                    else {
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

    for (auto& slider : sliders) {
        const auto& rect = slider.rect;
        const Vec2& p1 = rect.position;
        const Vec2 p2 = rect.position + rect.scale;
        if (input.mouse_down(SDL_BUTTON_LEFT) && input.mouse_pos.x > p1.x && input.mouse_pos.y > p1.y && input.mouse_pos.x < p2.x && input.mouse_pos.y < p2.y) {
            slider.callback((input.mouse_pos.x - p1.x) / rect.scale.x);
            clicked = true;
        }
    }
}

void UI::text_field(TextFieldState* state, Style style) {
    ZoneScoped;

    if (state->focused) {
        style.border_color = { 255, 255, 255, 255 };
    }

    text_fields.push_back({
        state,
        internal_rect(state->text.data(), style),
        });

    if (group_stack.size() > 0) {
        auto& group = groups[group_stack.back()];
        group.children.push_back({ ElementType::text_field, (int) text_fields.size() - 1 });
    }
}

void UI::rect(const char* text, const Style& style) {
    ZoneScoped;

    auto index = internal_rect(text, style);

    if (group_stack.size() > 0) {
        auto& group = groups[group_stack.back()];
        group.children.push_back({ ElementType::rect,  (int) rects.size() - 1 });
    }
}

int UI::internal_rect(const char* text, const Style& style) {
    ZoneScoped;

    auto& padding = style.padding;
    int width = std::max(style.min_width, font_width(text, style.font_size)) + padding.left + padding.right;
    int height = std::max(style.min_height, font_height(text, style.font_size)) + padding.top + padding.bottom;

    rects.push_back(Rect{
        Vec2{0, 0},
        Vec2{(float)width, (float)height},
        text,
        style.font_size,
        style.text_color,
        style.background_color,
        style.border_color,
    });

    return (int)rects.size() - 1;
}


void UI::slider(float fraction, std::function<void(float)> on_input) {
    sliders.push_back({ Rect{{}, {500, 36}}, fraction, on_input });

    //if (in_group) {
    //    groups[groups.size() - 1].children.push_back({ ElementType::slider, sliders.size() - 1 });
    //} } void UI::text_field(std::string& text, Style style) { 

}

void UI::button(const char* text, Style style, std::function<void()> on_click) {
    ZoneScoped;

    buttons.push_back(Button{ internal_rect(text, style), on_click});

    if (group_stack.size() > 0) {
        auto& group = groups[group_stack.back()];
        group.children.push_back({ ElementType::button,  (int) buttons.size() - 1 });
    }
}

SDL_FPoint vec2_to_sdl_fpoint(const Vec2& vec) {
    return { vec.x, vec.y };
}

void draw_wire_box(SDL_Renderer* renderer, const Rect& rect) {
    SDL_FPoint points[5];
    const Vec2& pos = rect.position;
    const Vec2& scale = rect.scale;
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
        auto frect = SDL_FRect{ rect.position.x, rect.position.y, rect.scale.x, rect.scale.y };

        {
            ZoneNamedN(asdf, "draw rect", true);

            auto& bg_color = rect.background_color;
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
            SDL_RenderFillRect(renderer, &frect);
        }

        draw_text(renderer, rect.text, rect.font_size, rect.position, rect.text_color);

        {
            ZoneNamedN(askdjh, "draw border", true);
            auto& color = rect.border_color;
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            draw_wire_box(renderer, rect);
        }
    }

    rects.clear();
    sliders.clear();
    buttons.clear();
    groups.clear();
    text_fields.clear();

    clicked = false;
}
