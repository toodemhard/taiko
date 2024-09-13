#pragma once

#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "color.h"
#include "input.h"
#include "vec.h"

#include <SDL3/SDL.h>

namespace color {
constexpr RGBA white{255, 255, 255, 255};
constexpr RGBA black{0, 0, 0, 255};
constexpr RGBA red{255, 0, 0, 255};
constexpr RGBA grey{180, 180, 180, 255};
constexpr RGBA none{0, 0, 0, 0};
} // namespace color

constexpr int string_array_size{10};

class StringCache {
  public:
    const char* add(std::string&& string);
    void clear();

  private:
    std::vector<std::string> strings;
};

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

namespace Position {

// fraction of the screen
struct Anchor {
    Vec2 position;
};

struct Absolute {
    Vec2 position;
};

struct Relative {};

using Variant = std::variant<Relative, Anchor, Absolute>;
} // namespace Position

namespace Scale {

struct Fixed {
    float value;
};

struct Min {
    float value;
};

// scale to the content
struct FitContent {};

using Variant = std::variant<FitContent, Min, Fixed>;

} // namespace Scale

struct Style {
    Position::Variant position;
    RGBA background_color;
    RGBA border_color = color::red;

    Padding padding;

    Scale::Variant width;
    Scale::Variant height;
    bool overlap;

    // group styling
    StackDirection stack_direction = StackDirection::Horizontal;
    float gap;

    // text style
    float font_size = 36;
    RGBA text_color = color::white;
};

struct Rect {
    Vec2 position;
    Vec2 scale;
    RGBA background_color;
    RGBA border_color;
};

struct Text {
    Vec2 position;
    Vec2 scale;
    const char* text;
    float font_size;
    RGBA text_color;
};

struct Button {
    int rect_index;
    std::function<void()> on_click;
};

enum class ElementType {
    group,
    text,
};

struct ElementHandle {
    ElementType type;
    int index;
};

struct ClickInfo {
    Vec2 offset_pos;
    Vec2 scale;
};

using OnClick = std::function<void(ClickInfo)>;

struct Group {
    int rect_index;
    Style style;
    std::optional<uint16_t> on_click_index;
    std::optional<uint16_t> on_held_index;
    std::vector<ElementHandle> children;
};

struct ClickRect {
    Vec2 position;
    Vec2 scale;

    uint16_t on_click_index;
};

struct HeldRect {
    Vec2 position;
    Vec2 scale;

    int on_held_index;
};

struct TextFieldState {
    std::string text;
    bool focused;
};

struct TextField {
    TextFieldState* state;
    int rect_index;
};

struct Slider {
    bool held;
};

struct SliderStyle {
    Position::Variant position;
    float width;
    float height;
    RGBA bg_color;
    RGBA fg_color;
};

struct SliderCallbacks {
    std::function<void(float)> on_input;
    std::optional<std::function<void()>> on_click;
    std::optional<std::function<void()>> on_release;
};

struct DropDown {
    bool menu_dropped;
    bool clicked_last_frame;
};

using RectID = int;

struct DropDownOverlay {
    int rect_hook_index;
    int on_click_index;
    int selected_index;
    std::vector<const char*>* items;
};

class UI {
  public:
    UI() = default;
    UI(int _screen_width, int _screen_height);

    void text_field(TextFieldState* state, Style style);
    RectID button(const char* text, Style style, OnClick&& on_click);
    void slider(Slider& state, SliderStyle style, float fraction, SliderCallbacks&& callbacks);
    void drop_down_menu(
        int selected_opt_index,
        std::vector<const char*>&& options,
        DropDown& state,
        std::function<void(int)> on_input
    );
    void drop_down_menu(
        int selected_opt_index,
        std::vector<const char*>& options,
        DropDown& state,
        std::function<void(int)> on_input
    );

    void text(const char* text, const Style& style);

    void begin_group(const Style& style);
    RectID end_group();
    Rect query_rect(RectID id);

    void begin_group_button(const Style& style, OnClick&& on_click);

    void visit_group(Group& group, Vec2 start_pos);

    void input(Input& input);

    void begin_frame();

    void end_frame();

    void draw(SDL_Renderer* renderer);

    bool clicked = false;

    StringCache strings{};

  private:
    int screen_width = 0;
    int screen_height = 0;

    std::vector<ClickRect> click_rects;
    std::vector<HeldRect> slider_input_rects;

    std::vector<Rect> rects;

    std::vector<Group> groups;
    std::vector<Text> texts;

    std::vector<TextField> text_fields;

    std::vector<std::function<void()>> on_release_callbacks;
    std::vector<std::function<void()>> user_callbacks;
    std::vector<OnClick> on_click_callbacks;
    std::vector<std::function<void(float)>> slider_on_input_callbacks;
    std::vector<std::function<void(int)>> on_select_callbacks;

    std::vector<const char*> m_options;
    std::optional<DropDownOverlay> post_overlay;
    std::vector<DropDown*> dropdown_clickoff_callbacks;

    std::vector<int> group_stack;

    void begin_group_any(const Group& group);
    Vec2& element_scale(ElementHandle e);
    Vec2& element_position(ElementHandle e);
};
