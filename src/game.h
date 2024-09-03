#pragma once

#include <SDL3/SDL.h>
#include <tracy/Tracy.hpp>

#include "vec.h"
#include "map.h"
#include "systems.h"
#include "ui.h"
#include "constants.h"

#include "assets.h"

class Cam {
public:
    Vec2 position;
    Vec2 bounds;
    
    Vec2 world_to_screen(const Vec2& point) const;
    Vec2 screen_to_world(const Vec2& point) const;
    Vec2 world_to_screen_scale(const Vec2& scale) const;
    float world_to_screen_scale(const float& length) const;
};

enum hit_effect {
    none,
    ok,
    perfect,
};

enum DrumInputFlagBits : uint8_t {
    don_kat = 1 << 0,
    left_right = 1 << 1,
};

enum DrumInput : uint8_t {
    don_left = DrumInputFlagBits::don_kat | DrumInputFlagBits::left_right,
    don_right = DrumInputFlagBits::don_kat | 0,
    kat_left = 0 | DrumInputFlagBits::left_right,
    kat_right = 0,
};

struct InputRecord {
    DrumInput type;
    double time;
};

struct BigNoteHits {
    bool left = false;
    bool right = false;
};

namespace game {

struct InitConfig {
    bool auto_mode;
    bool test_mode;
};

}

struct InFlightNotes {
    std::vector<NoteFlags> flags;
    std::vector<double> times;
};

class Game {
public:
    Game(Systems systems, game::InitConfig config, Map map);
    void start();
    void update(std::chrono::duration<double> delta_time);
private:
    SDL_Renderer* renderer{};
    Input& input;
    Audio& audio;
    AssetLoader& assets;
    EventQueue& event_queue;

    Cam cam{{0,0}, {1.5f, 1.5f}};

    float accuracy_fraction = 1;

    int perfect_count{};
    int ok_count{};
    int miss_count{};

    bool m_test_mode{ false };
    bool m_auto_mode{ false };

    int current_note_index = 0;

    int score = 0;
    int combo = 0;

    UI ui{ constants::window_width, constants::window_height };

    std::vector<InputRecord> input_history;

    Map m_map{};
    std::vector<bool> note_alive_list{};

    hit_effect m_current_hit_effect{};
    float hit_effect_time_point_seconds{};

    InFlightNotes in_flight_notes;
    
    bool m_paused{};
    bool m_end_screen{};

    BigNoteHits current_big_note_status;

    bool initialized = false;

    void draw_map();
};
