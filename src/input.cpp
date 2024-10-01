#include <tracy/Tracy.hpp>
#include <cstring>
#include "input.h"
#include "SDL3/SDL_scancode.h"

namespace Input {

void Input::begin_frame() {
    ZoneScoped;

    std::fill(keyboard_repeat.begin(), keyboard_repeat.end(), false);
    current_mouse = SDL_GetMouseState(&mouse_pos.x, &mouse_pos.y);
    mod_state = SDL_GetModState();
}

void Input::end_frame() {
    ZoneScoped;

    std::memcpy(last_keyboard.data(), current_keyboard, SDL_NUM_SCANCODES);

    m_key_this_frame = {};

    last_mouse = current_mouse;
}

void Input::init_keybinds(std::array<Keybind, ActionID::count> keybinds) {
    for (auto kb : keybinds) {
        keybindings[kb.action_id] = kb.scancode;
    }
}

bool Input::action_down(int action_id) {
    return this->key_down(keybindings[action_id]);
}

bool Input::modifier(const SDL_Keymod modifiers) const {
    return (modifiers & mod_state);
}

bool Input::mouse_down(const SDL_MouseButtonFlags& button_mask) const {
    return (current_mouse & button_mask) && !(last_mouse & button_mask);
}

bool Input::mouse_held(const SDL_MouseButtonFlags& button_mask) const {
    return (current_mouse & button_mask);
}

bool Input::mouse_up(const SDL_MouseButtonFlags& button_mask) const {
    return !(current_mouse & button_mask) && (last_mouse & button_mask);
}

bool Input::key_down(const SDL_Scancode& scan_code) const {
    return current_keyboard[scan_code] && !last_keyboard[scan_code];
}

bool Input::key_held(const SDL_Scancode& scan_code) const {
    return current_keyboard[scan_code];
}

bool Input::key_up(const SDL_Scancode& scan_code) const {
    return !current_keyboard[scan_code] && last_keyboard[scan_code];
}

bool Input::key_down_repeat(const SDL_Scancode& scan_code) const {
    return keyboard_repeat[scan_code];
}

}
