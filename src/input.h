#pragma once

#include "SDL3/SDL_scancode.h"
#include "vec.h"
#include <SDL3/SDL.h>
#include <array>

#include <optional>
#include <string>
#include <vector>

namespace Input {
#define KEYBINDINGS(_)                                                                                                 \
    _(kat_left, SDL_SCANCODE_Z)                                                                                        \
    _(don_left, SDL_SCANCODE_X)                                                                                        \
    _(don_right, SDL_SCANCODE_PERIOD)                                                                                  \
    _(kat_right, SDL_SCANCODE_SLASH)

#define ACTION(a, b) a,

enum ActionID {
    KEYBINDINGS(ACTION)
    count,
};

struct Keybind {
    uint32_t action_id;
    SDL_Scancode scancode;
};

#define KEYBIND(a, b) Keybind{ActionID::a, b},
constexpr std::array<Keybind, ActionID::count> default_keybindings = {KEYBINDINGS(KEYBIND)};

#define AS_STRING(a, b) #a,
constexpr std::array<const char*, ActionID::count> action_names = {KEYBINDINGS(AS_STRING)};

class Input {
  public:
    void begin_frame();
    void end_frame();

    void init_keybinds(std::array<Keybind, ActionID::count> keybinds);
    bool action_down(int action_id);

    bool key_down(const SDL_Scancode& scan_code) const;
    bool key_held(const SDL_Scancode& scan_code) const;
    bool key_up(const SDL_Scancode& scan_code) const;
    bool key_down_repeat(const SDL_Scancode& scan_code) const;

    bool modifier(const SDL_Keymod modifiers) const;

    bool mouse_down(const SDL_MouseButtonFlags& button_mask) const;
    bool mouse_held(const SDL_MouseButtonFlags& button_mask) const;
    bool mouse_up(const SDL_MouseButtonFlags& button_mask) const;

    Vec2 mouse_pos{};
    float wheel{};
    std::optional<std::string> input_text;
    std::array<bool, SDL_SCANCODE_COUNT> keyboard_repeat{};

    std::array<SDL_Scancode, ActionID::count> keybindings;

    std::optional<SDL_Scancode> m_key_this_frame;

  private:
    std::array<bool, SDL_SCANCODE_COUNT> last_keyboard{};
    const bool* current_keyboard = SDL_GetKeyboardState(NULL);


    SDL_Keymod mod_state;

    SDL_MouseButtonFlags last_mouse{};
    SDL_MouseButtonFlags current_mouse{};
};
}

