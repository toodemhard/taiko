#pragma once

#include <vector>
#include <stack>
#include <functional>
#include <optional>
#include <string>

#include "vec.h"
#include "input.h"
#include "color.h"

#include <SDL3/SDL.h>

enum class StackDirection {
    Horizontal,
    Vertical,
};

struct Padding {
    float left;
    float right;
    float top;
    float bottom;
};

struct Style {
    Vec2 anchor;
    float font_size = 36;
    Color text_color = { 255, 255, 255, 255 };
    Color background_color = { 0, 0, 0, 0 };
    Color border_color;

    Padding padding;

    float min_width;
    float min_height;

    // group styling
    StackDirection stack_direction = StackDirection::Horizontal;
    float gap;
};

struct GroupStyle {
    Vec2 anchor;

    Color background_color = { 0, 0, 0, 0 };
    Color border_color;

    Padding padding;

    StackDirection stack_direction = StackDirection::Horizontal;
    float gap;
};

struct Rect {
    Vec2 position;
    Vec2 scale;
    const char* text;
    float font_size = 36;
    Color text_color = { 255, 255, 255, 255 };
    Color background_color = { 0, 0, 0, 0 };
    Color border_color;
};

struct Button {
    int rect_index;
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
    group,
    text_field,
};

struct ElementHandle {
    ElementType type;
    int index;
};

struct ClickInfo {
    Vec2 offset_pos;
};

using OnClick = std::function<void(ClickInfo)>;

struct Group {
    int rect_index;
    Style style;
    std::vector<ElementHandle> children;
    OnClick click_property;
};

struct ClickRect {
    Vec2 position;
    Vec2 scale;

    OnClick on_click;
};

struct Properties {
    OnClick on_click;
};

struct TextFieldState {
    std::string text;
    bool focused;
};

struct TextField {
    TextFieldState* state;
    int rect_index;
};

class UI {
public:
    UI() = default;
    UI(int _screen_width, int _screen_height);

    void text_field(TextFieldState* state, Style style);
    void button(const char* text, Style style, std::function<void()> on_click);
    void slider(float fraction, std::function<void(float)> on_input);
    void rect(const char* text, const Style& style);
    void begin_group(const Style& style);
    void end_group();

    void begin_group_v2(const Style& style, const Properties& properties);
    void end_group_v2();

    void visit_group(Group& group, Vec2 start_pos);

    void input(Input& input);

    void draw(SDL_Renderer* renderer);

    bool clicked = false;

private:
    int screen_width = 0;
    int screen_height = 0;


    //properties
    std::vector<ClickRect> click_rects;
    std::vector<Rect> rects;

    std::vector<Button> buttons;
    std::vector<Slider> sliders;
    std::vector<Group> groups;
    std::vector<TextField> text_fields;

    std::vector<int> group_stack;

    int internal_rect(const char* text, const Style& style);
    Rect& element_rect(ElementHandle e);
};
