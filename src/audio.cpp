#include <SDL3/SDL.h>
#include <chrono>
#include <stdlib.h>
#include <iostream>
#include <format>

#include "audio.h"
#include "SDL3_mixer/SDL_mixer.h"

Audio::Audio() {
    Mix_OpenAudio(0, NULL);
}

// return 0 on success, 1 on error
int Audio::load_music(const char* file_path) {
    if (m_music != nullptr) {
        this->stop();
    }

    auto result = Mix_LoadMUS(file_path);
    if (result == NULL) {
        return 1;
    }

    m_music = result;
    elapsed_at_last_time = 0;

    return 0;
}

void Audio::fade_in(int loops, int ms) {
    Mix_FadeInMusic(m_music, loops, ms);
    m_loops = loops;
    last_time = std::chrono::high_resolution_clock::now();
}

void Audio::play(int loops) {
    Mix_PlayMusic(m_music, loops);
    m_loops = loops;
    last_time = std::chrono::high_resolution_clock::now();
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
    if (!Mix_PlayingMusic()) {
        return;
    }

    Mix_PauseMusic();
    elapsed_at_last_time += std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - last_time).count();
}

double Audio::get_position() {
    if (m_music == nullptr) {
        return 0;
    }

    double current_elapsed = elapsed_at_last_time;
    if (!paused()) {
        current_elapsed = elapsed_at_last_time + std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - last_time).count();
        auto duration = Mix_MusicDuration(m_music);
        if (current_elapsed > duration) {
            if (m_loops > 0) {
                current_elapsed = std::fmod(current_elapsed, duration);

                last_time = std::chrono::high_resolution_clock::now();
                elapsed_at_last_time = current_elapsed;

                m_loops--;
            } else {
                return duration;
            }
        }
    }

    return current_elapsed;
}

void Audio::set_position(double position) {
    if (position < 0) {
        position = 0;
    }
    Mix_SetMusicPosition(position);
    elapsed_at_last_time = position;
    last_time = std::chrono::high_resolution_clock::now();
}

bool Audio::paused() {
    return (bool)Mix_PausedMusic();
}
