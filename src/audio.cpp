#include <stdlib.h>
#include <SDL3/SDL.h>

//#define MINIMP3_IMPLEMENTATION
//#include "minimp3.h"
//#include "minimp3_ex.h"

#include <iostream>
#include <format>

#include "audio.h"

Audio::Audio() {
    Mix_OpenAudio(0, NULL);
}

/*
0 on success, 1 on error
*/
int Audio::load_music(const char* file_path) {
    if (m_music != nullptr) {
        this->stop();
    }

    auto result = Mix_LoadMUS(file_path);
    if (result == NULL) {
        return 1;
    }

    m_music = result;
    elapsed = 0;

    Mix_PlayMusic(m_music, 0);
    Mix_PauseMusic();

    return 0;
}

void Audio::stop() {
    Mix_FreeMusic(m_music);
    m_music = nullptr;
}

void Audio::resume() {
    Mix_ResumeMusic();
    last_time = std::chrono::high_resolution_clock::now();
}

void Audio::pause() {
    Mix_PauseMusic();
    elapsed += (std::chrono::high_resolution_clock::now() - last_time).count() / 1000000000.0;
}

double Audio::get_position() {
    float ret = elapsed;
    if (!paused()) {
        ret += (std::chrono::high_resolution_clock::now() - last_time).count() / 1000000000.0;
    }

    return ret;
}

void Audio::set_position(double position) {
    if (position < 0) {
        position = 0;
    }
    Mix_SetMusicPosition(position);
    elapsed = position;
    last_time = std::chrono::high_resolution_clock::now();
}

bool Audio::paused() {
    return (bool)Mix_PausedMusic();
}

#include <SDL3/SDL.h>

Player::Player(Input& _input, Audio& _audio) : input{ _input }, audio{ _audio } {
}

void Player::update(float delta_time) {
    if (input.key_down(SDL_SCANCODE_SPACE)) {
        if (audio.paused()) {
            audio.resume();
        }
        else {
            audio.pause();
        }

    }

    if (!audio.paused()) {
        std::cout << std::format("measured: {}s, lib: {}s\n", audio.get_position(), Mix_GetMusicPosition(audio.m_music));
    }
}
