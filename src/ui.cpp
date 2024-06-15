#include "ui.h"
#include "raylib.h"
#include <iostream>

void UI::begin_group() {
    groups.push_back(Group{});
    in_group = true;
}

void UI::end_group() {
    in_group = false;

    Group& group = groups[groups.size() - 1];

	float total_width = 50;
	for (auto& i: group.children) {
		total_width += rects[i].scale.x;
	}
    float total_height = font_size;

	Vec2 start = {((float)GetScreenWidth() - total_width) * group.style.anchor.x, ((float)GetScreenHeight() - total_height) * group.style.anchor.y};

	for (auto& i: group.children) {
		Rect& rect = rects[i];
		rect.position = {start.x, start.y};

        start.x += rect.scale.x + 50;

		const Vec2& p1 = rect.position;
		const Vec2& p2 = rect.position + rect.scale;

        if (rect.callback.has_value()) {
			if (mouse_zero && mouse_pos.x > p1.x && mouse_pos.y > p1.y && mouse_pos.x < p2.x && mouse_pos.y < p2.y) {
				rect.callback.value()();
			}
        }
	}
}

void UI::input() {
    mouse_zero = IsMouseButtonPressed(0);
    mouse_pos = {(float)GetMouseX(), (float)GetMouseY()};
}

void UI::rect(const char* text) {
    rects.push_back(Rect{ Vec2{0, 0}, Vec2{(float)MeasureText(text, font_size), (float)font_size}, text, {}});

    if (in_group) {
        groups[groups.size() - 1].children.push_back(rects.size() - 1);
    }
}

void UI::button(const char* text, std::function<void()> on_click) {
    float height = 0;
    if (rects.size() > 0) {
        height = rects[rects.size() - 1].position.y + 220;
    }

    rects.push_back(Rect{Vec2{0, height}, Vec2{(float)MeasureText(text, font_size), (float)font_size}, text, on_click});

    if (in_group) {
        groups[groups.size() - 1].children.push_back(rects.size() - 1);
    }
}

void UI::draw() {
    for (auto& r : rects) {
        DrawRectangle(r.position.x, r.position.y, r.scale.x, r.scale.y, GRAY);
        DrawText(r.text, r.position.x, r.position.y, font_size, WHITE);
    }

    rects.clear();
    groups.clear();
}
