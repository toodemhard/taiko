#include "ui.h"
#include "raylib.h"
#include <iostream>

void UI::begin_group(Style style) {
    groups.push_back(Group{ {},  style });
    in_group = true;
}

//Element UI::get_element(ElementHandle handle) {
//    switch (handle.type) {
//        case 
//    }
//}

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

	Vec2 start = {((float)GetScreenWidth() - total_width) * group.style.anchor.x, ((float)GetScreenHeight() - total_height) * group.style.anchor.y};

	for (auto& e: group.children) {
        switch (e.type) {
        case ElementType::slider:
        {
            Slider& slider = sliders[e.index];
			slider.position = {start.x, start.y};

			const Vec2& p1 = slider.position;
			Vec2 p2 = slider.position + slider.scale;

			if (mouse_zero && mouse_pos.x > p1.x && mouse_pos.y > p1.y && mouse_pos.x < p2.x && mouse_pos.y < p2.y) {
                slider.callback((mouse_pos.x - start.x) / slider.scale.x);
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

			if (mouse_zero && mouse_pos.x > p1.x && mouse_pos.y > p1.y && mouse_pos.x < p2.x && mouse_pos.y < p2.y) {
                button.callback();
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

void UI::input() {
    mouse_zero = IsMouseButtonPressed(0);
    mouse_pos = {(float)GetMouseX(), (float)GetMouseY()};
}

void UI::rect(const char* text) {
    rects.push_back(Rect{ Vec2{0, 0}, Vec2{(float)MeasureText(text, font_size), (float)font_size}, text});

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
    buttons.push_back( {Vec2{0, 0}, Vec2{(float)MeasureText(text, font_size), (float)font_size}, text, on_click});

    if (in_group) {
        groups[groups.size() - 1].children.push_back({ ElementType::button, buttons.size() - 1 });
    }
}

void UI::draw() {
    for (auto& e : rects) {
        DrawRectangle(e.position.x, e.position.y, e.scale.x, e.scale.y, GRAY);
        DrawText(e.text, e.position.x, e.position.y, font_size, WHITE);
    }

    for (auto& e : sliders) {
        DrawRectangle(e.position.x, e.position.y, e.scale.x, e.scale.y, GRAY);
        float x = e.position.x + e.scale.x * e.fraction;
        DrawLine(x, e.position.y, x, e.position.y + e.scale.y, YELLOW);
    }

    rects.clear();
    sliders.clear();
    groups.clear();
}
