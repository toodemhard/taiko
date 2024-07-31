#include <tracy/Tracy.hpp>

#include "font.h"
#include "ui.h"
#include <format>
#include <iostream>

UI::UI(int _screen_width, int _screen_height) : screen_width{ _screen_width }, screen_height{ _screen_height } {}

void UI::begin_group(const Style& style) {
    groups.push_back(Group{ {},  style });
    in_group = true;
}

void UI::end_group() {
    in_group = false;

    Group& group = groups[groups.size() - 1];

	float total_width = 0;
	for (auto& e: group.children) {
        Rect& rect = [&]() -> Rect& {
            switch (e.type) {
            case ElementType::slider:
                return sliders[e.index].rect;
                break;
            case ElementType::button:
                return buttons[e.index].rect;
                break;
            case ElementType::rect:
                return rects[e.index].rect;
                break;
            }
        }();

        total_width += rect.scale.x;
	}

    float total_height = font_size;

	Vec2 start = {(screen_width - total_width) * group.style.anchor.x, (screen_height - total_height) * group.style.anchor.y};

	for (auto& e: group.children) {
        Rect& rect = [&]() -> Rect& {
            switch (e.type) {
            case ElementType::slider:
                return sliders[e.index].rect;
                break;
            case ElementType::button:
                return buttons[e.index].rect;
                break;
            case ElementType::rect:
                return rects[e.index].rect;
                break;
            }
        }();

        rect.position = { start.x, start.y };
        start.x += rect.scale.x + 50;
	}
}

void UI::input(Input& input) {
    for (auto& button : buttons) {
        const auto& rect = button.rect;
        const Vec2& p1 = rect.position;
        const Vec2 p2 = rect.position + rect.scale;

        if (input.mouse_down(SDL_BUTTON_LEFT) && input.mouse_pos.x > p1.x && input.mouse_pos.y > p1.y && input.mouse_pos.x < p2.x && input.mouse_pos.y < p2.y) {
            button.on_click();
            clicked = true;
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

void UI::rect(const char* text) {
    int width = font_width(text, 36);

    int height = font_height(36);

    rects.push_back(Rector{ Rect{Vec2{0, 0}, Vec2{(float)width, (float)height}}, text });

    if (in_group) {
        groups[groups.size() - 1].children.push_back(ElementHandle{ ElementType::rect, rects.size() - 1 });
    }
}


void UI::slider(float fraction, std::function<void(float)> on_input) {
    sliders.push_back({ Rect{{}, {500, 36}}, fraction, on_input });

    if (in_group) {
        groups[groups.size() - 1].children.push_back({ ElementType::slider, sliders.size() - 1 });
    }
}

void UI::button(const char* text, std::function<void()> on_click) {
    int width = font_width(text, 36);

    int height = font_height(36);
    buttons.push_back( Button{ Vec2{0, 0}, Vec2{(float)width, (float)height}, text, on_click});

    if (in_group) {
        groups[groups.size() - 1].children.push_back({ ElementType::button, buttons.size() - 1 });
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

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderLines(renderer, points, 5);
}

void UI::draw(SDL_Renderer* renderer) {
    ZoneScoped;

    for (auto& e : rects) {
        draw_text(renderer, e.text, e.rect.position);
        draw_wire_box(renderer, e.rect);
    }

    for (auto& e : buttons) {
        draw_text(renderer, e.text, e.rect.position);
        draw_wire_box(renderer, e.rect);
    }

    //for (auto& e : sliders) {
    //    DrawRectangle(e.position.x, e.position.y, e.scale.x, e.scale.y, GRAY);
    //    float x = e.position.x + e.scale.x * e.fraction;
    //    DrawLine(x, e.position.y, x, e.position.y + e.scale.y, YELLOW);
    //}

    rects.clear();
    sliders.clear();
    buttons.clear();
    groups.clear();

    clicked = false;
}
