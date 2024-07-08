#pragma once

#include <vector>
#include <functional>
#include <optional>

#include "vec.h"
#include "input.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

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
    UI(const Input& _input, int _screen_width, int _screen_height);
    void button(const char* text, std::function<void()> on_click);
    void slider(float fraction, std::function<void(float)> on_input);
    void rect(const char* text);

    void draw(SDL_Renderer* renderer);

    void begin_group(const Style& style);
    void end_group();

    bool clicked = false;

private:
    const Input& input;
    TTF_Font* font;

    int screen_width;
    int screen_height;
    const int font_size = 36;
    std::vector<Button> buttons;
    std::vector<Rect> rects;
    std::vector<Slider> sliders;
    std::vector<Group> groups;

    bool in_group = false;
};