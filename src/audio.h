#pragma once

#include <chrono>
#include <SDL3_mixer/SDL_mixer.h>

static int effect_volume{(int)(MIX_MAX_VOLUME * 0.3)};

enum class AudioState {
    initial,
    playing,
    paused,
    stopped,
};

// need to wrap audio cuz changing lib a lot
// keep track of time also
class Audio {
public:
    Audio();
    int load_music(const char* file_path);
    void resume();
    void pause();
    void stop();
    void set_position(double position);
    double get_position();
    bool paused();
    void play(int loops);
    void fade_in(int loops, int ms);

    Mix_Music* m_music = nullptr;
    double elapsed_at_last_time = 0;

    int m_loops{};

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> last_time;
};
