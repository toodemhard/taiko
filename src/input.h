#pragma once

#include <SDL3/SDL.h>
#include <array>
#include "vec.h"

#include <optional>
#include <string>

class Input {
public:
    void begin_frame();
    void end_frame();

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
    std::array<Uint8, SDL_NUM_SCANCODES> keyboard_repeat{};

private:
    std::array<Uint8, SDL_NUM_SCANCODES> last_keyboard{};
    const Uint8* current_keyboard = SDL_GetKeyboardState(NULL);

    SDL_Keymod mod_state;

    SDL_MouseButtonFlags last_mouse{};
    SDL_MouseButtonFlags current_mouse{};
};
