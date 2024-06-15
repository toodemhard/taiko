#include <vector>
#include <functional>
#include <optional>
#include "vec.h"

#pragma once

struct Style {
    Vec2 anchor;
};

struct Rect {
    Vec2 position;
    Vec2 scale;
    const char* text;
    std::optional<std::function<void()>> callback;
};

struct Group {
    std::vector<int> children;
    Style style = { Vec2{1.0f,1.0f} };
};

class UI {
public:
    void button(const char* text, std::function<void()> on_click);
    void rect(const char* text);

    void input();
    void draw();

    void begin_group();
    void end_group();

private:
	const int font_size = 36;
    std::vector<Rect> rects;
    std::vector<Group> groups;

    bool mouse_zero;
    Vec2 mouse_pos;

    bool in_group = false;
};