#pragma once

#include <SDL3/SDL.h>
#include <tracy/Tracy.hpp>

#include "vec.h"
#include "map.h"
#include "systems.h"
#include "ui.h"
#include "constants.h"

class Cam {
public:
    Vec2 position;
    Vec2 bounds;
    
    Vec2 world_to_screen(const Vec2& point) const;
    Vec2 screen_to_world(const Vec2& point) const;
    Vec2 world_to_screen_scale(const Vec2& scale) const;
    float world_to_screen_scale(const float& length) const;
};

//struct Particle {
//    Vec2 position;
//    Vec2 velocity;
//    float scale;
//    NoteType type;
//    std::chrono::duration<double> start;
//};

//void draw_particles(const Cam& cam, const std::vector<Particle>& particles, std::chrono::duration<double> now) {
//    float outer_radius = cam.world_to_screen_scale(circle_outer_radius);
//    float inner_radius = cam.world_to_screen_scale(circle_radius);
//    for (auto& p : particles) {
//        Vec2 pos = cam.world_to_screen(p.position);
//
//        Color color;
//
//        if (p.type == NoteType::don) {
//            color = RED;
//        } else {
//            color = BLUE;
//        }
//
//        uint8_t alpha = (p.start - now).count() / particle_duration * 255;
//        color.a = alpha;
//
//        //DrawCircle(pos.x, pos.y, outer_radius, Color{255,255,255,alpha});
//        DrawCircle(pos.x, pos.y, inner_radius, color);
//    }
//}

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

class Game {
public:
    Game(SDL_Renderer* _renderer, Input& _input, Audio& _audio, AssetLoader& _assets, EventQueue& _event_queue, Map map);
    void start();
    void update(std::chrono::duration<double> delta_time);
private:
    SDL_Renderer* renderer{};
    Input& input;
    Audio& audio;
    AssetLoader& assets;
    EventQueue& event_queue;

    Cam cam{{0,0}, {2,1.5f}};

    int current_note = 0;

    int score = 0;
    int combo = 0;

    UI ui{ constants::window_width, constants::window_height };

    std::vector<InputRecord> input_history;

    Map map{};

    //std::vector<Particle> particles;

    BigNoteHits current_big_note_status;

    bool auto_mode = true;

    bool initialized = false;
};


