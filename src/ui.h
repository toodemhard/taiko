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
};

struct Rector {
    Rect rect;
    const char* text;
};

struct Button {
    Rect rect;
    const char* text;
    std::function<void()> on_click;
};

struct Slider {
    Rect rect;
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
    UI() = default;
    UI(int _screen_width, int _screen_height);

    void button(const char* text, std::function<void()> on_click);
    void slider(float fraction, std::function<void(float)> on_input);
    void rect(const char* text);
    void begin_group(const Style& style);
    void end_group();

    void input(Input& input);

    void draw(SDL_Renderer* renderer);

    bool clicked = false;

private:
    TTF_Font* font;

    int screen_width;
    int screen_height;
    int font_size = 36;
    std::vector<Button> buttons;
    std::vector<Rector> rects;
    std::vector<Slider> sliders;
    std::vector<Group> groups;

    bool in_group = false;
};