#include <tracy/Tracy.hpp>
#include "input.h"

void Input::begin_frame(float& _wheel) {
    ZoneScoped;

    current_mouse = SDL_GetMouseState(NULL, NULL);
    wheel = _wheel;
    SDL_GetMouseState(&mouse_pos.x, &mouse_pos.y);
}

void Input::end_frame() {
    ZoneScoped;

    std::memcpy(last_keyboard.data(), current_keyboard, SDL_NUM_SCANCODES);

    last_mouse = current_mouse;
}

bool Input::mouse_down(const SDL_MouseButtonFlags& button) const {
    return (current_mouse & button) && !(last_mouse & button);
}

bool Input::mouse_held(const SDL_MouseButtonFlags& button) const {
    return (current_mouse & button);
}

bool Input::mouse_up(const SDL_MouseButtonFlags& button) const {
    return !(current_mouse & button) && (last_mouse & button);
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
