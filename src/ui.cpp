#include <tracy/Tracy.hpp>

#include "ui.h"
#include <format>
#include <iostream>

UI::UI(const Input& _input, int _screen_width, int _screen_height)
    : input{ _input }, screen_width{ _screen_width }, screen_height{ _screen_height } {

    font = TTF_OpenFont("data/JetBrainsMonoNLNerdFont-Regular.ttf", 36);
}

void draw_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, Vec2 position) {
    ZoneScoped;

    SDL_Surface* surface;
    {
        ZoneNamedN(var, "Render to surface", true);
        surface = TTF_RenderText_Solid(font, text, SDL_Color{ 255, 255, 255, 255});

    }

    if (surface == NULL) {
        return;
    }

    SDL_Texture* texture;

    {
        ZoneNamedN(var2, "Create texture", true);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
    }

    {
        ZoneNamedN(var2, "Render texture", true);
        auto rect = SDL_FRect{ position.x, position.y, (float)surface->w, (float)surface->h };
        SDL_RenderTexture(renderer, texture, NULL, &rect);
    }

    {
        ZoneNamedN(var2, "Cleanup", true);
        SDL_DestroyTexture(texture);
        SDL_DestroySurface(surface);
    }
}

void UI::begin_group(const Style& style) {
    groups.push_back(Group{ {},  style });
    in_group = true;
}

void UI::end_group() {
    in_group = false;

    Group& group = groups[groups.size() - 1];

	float total_width = 50;
	for (auto& e: group.children) {
        switch (e.type) {
        case ElementType::slider:
            total_width += sliders[e.index].scale.x;
            break;
        case ElementType::button:
            total_width += buttons[e.index].scale.x;
            break;
        case ElementType::rect:
            total_width += rects[e.index].scale.x;
            break;

        }
	}

    float total_height = font_size;

	Vec2 start = {(screen_width - total_width) * group.style.anchor.x, (screen_height - total_height) * group.style.anchor.y};

	for (auto& e: group.children) {
        switch (e.type) {
        case ElementType::slider:
        {
            Slider& slider = sliders[e.index];
			slider.position = {start.x, start.y};

			const Vec2& p1 = slider.position;
			Vec2 p2 = slider.position + slider.scale;

			if (input.mouse_down(SDL_BUTTON_LEFT) && input.mouse_pos.x > p1.x && input.mouse_pos.y > p1.y && input.mouse_pos.x < p2.x && input.mouse_pos.y < p2.y) {
                slider.callback((input.mouse_pos.x - start.x) / slider.scale.x);
                clicked = true;
			}

			start.x += slider.scale.x + 50;

            break;
        }
        case ElementType::button:
        {
            Button& button = buttons[e.index];
			button.position = {start.x, start.y};

			const Vec2& p1 = button.position;
			Vec2 p2 = button.position + button.scale;

			if (input.mouse_down(SDL_BUTTON_LEFT) && input.mouse_pos.x > p1.x && input.mouse_pos.y > p1.y && input.mouse_pos.x < p2.x && input.mouse_pos.y < p2.y) {
                button.callback();
                clicked = true;
			}

			start.x += button.scale.x + 50;

            break;
        }
        case ElementType::rect:
        {
			Rect& rect = rects[e.index];
			rect.position = {start.x, start.y};
			start.x += rect.scale.x + 50;

            break;
        }
        }
	}
}

void UI::rect(const char* text) {
    int width;
    TTF_MeasureUTF8(font, text, 9999, &width, NULL);

    int height = TTF_FontHeight(font);
    rects.push_back(Rect{ Vec2{0, 0}, Vec2{(float)width, (float)height}, text});

    if (in_group) {
        groups[groups.size() - 1].children.push_back(ElementHandle{ ElementType::rect, rects.size() - 1 });
    }
}


void UI::slider(float fraction, std::function<void(float)> on_input) {
    sliders.push_back({ {}, {500, 36}, fraction, on_input });

    if (in_group) {
        groups[groups.size() - 1].children.push_back({ ElementType::slider, sliders.size() - 1 });
    }
}

void UI::button(const char* text, std::function<void()> on_click) {
    int width;
    TTF_MeasureUTF8(font, text, 9999, &width, NULL);

    int height = TTF_FontHeight(font);
    buttons.push_back( Button{ Vec2{0, 0}, Vec2{(float)width, (float)height}, text, on_click});

    if (in_group) {
        groups[groups.size() - 1].children.push_back({ ElementType::button, buttons.size() - 1 });
    }
}

void UI::draw(SDL_Renderer* renderer) {
    ZoneScoped;

    for (auto& e : rects) {
        draw_text(renderer, font, e.text, e.position);
    }

    for (auto& e : buttons) {
        draw_text(renderer, font, e.text, e.position);
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
