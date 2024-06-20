#include <vector>
#include <functional>
#include <optional>
#include "vec.h"

#pragma once


class Element {
    virtual Vec2 position() = 0;
};

struct Style {
    Vec2 anchor;
};

struct Rect {
    Vec2 position;
    Vec2 scale;
    const char* text;
};

struct Button {
    Vec2 position;
    Vec2 scale;
    const char* text;
    std::function<void()> callback;
};

struct Slider {
    Vec2 position;
    Vec2 scale;
    float fraction;
    std::function<void(float)> callback;
};

enum class ElementType {
    button,
    slider,
    rect,
};

struct ElementHandle {
    ElementType type;
    size_t index;
};

struct Group {
    std::vector<ElementHandle> children;
    Style style =  {};
};


class UI {
public:
    void button(const char* text, std::function<void()> on_click);
    void slider(float fraction, std::function<void(float)> on_input);
    void rect(const char* text);

    void input();
    void draw();

    void begin_group(Style style);
    void end_group();

    bool clicked;

private:
    const int font_size = 36;
    std::vector<Button> buttons;
    std::vector<Rect> rects;
    std::vector<Slider> sliders;
    std::vector<Group> groups;

    bool mouse_zero;
    Vec2 mouse_pos;

    bool in_group = false;
};