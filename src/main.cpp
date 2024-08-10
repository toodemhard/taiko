#include <tracy/Tracy.hpp>

#include <iostream>
#include <algorithm>
#include <functional>
#include <format>
#include <iostream>
#include <fstream>
#include <string>
#include <codecvt>
#include <chrono>
#include <filesystem>
#include <array>
#include <future>
#include <queue>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>

#include "vec.h"
#include "ui.h"
#include "audio.h"
#include "font.h"
#include "asset_loader.h"
#include "color.h"

using namespace std::chrono_literals;

const int window_width = 1280;
const int window_height = 960;

const std::chrono::duration<double> hit_range = 100ms;
const std::chrono::duration<double> perfect_range = 30ms;

const float circle_radius = 0.1f;
const float circle_outer_radius = 0.11f;


Assets assets;

//struct Globals {
//    Input input;
//    Audio audio;
//};

enum class EventType {
    EditorToTest,
    TestToEditor,
    ToEditor,
};

class EventQueue {
public:
    bool pop_event(EventType* event);
    void push_event(EventType event);
private:
    std::queue<EventType> events{};
};

void EventQueue::push_event(EventType event) {
    events.push(event);
}

bool EventQueue::pop_event(EventType* event) {
    if (events.empty()) {
        return false;
    }

    *event = events.front();
    events.pop();

    return true;
}

EventQueue g_event_queue;


enum NoteFlagBits : uint8_t {
    don_or_kat = 1 << 0,
    normal_or_big = 1 << 1,
};

using NoteFlags = uint8_t;

class Cam {
public:
    Vec2 position;
    Vec2 bounds;
    
    Vec2 world_to_screen(const Vec2& point) const;
    Vec2 screen_to_world(const Vec2& point) const;
    Vec2 world_to_screen_scale(const Vec2& scale) const;
    float world_to_screen_scale(const float& length) const;
};

Vec2 Cam::world_to_screen(const Vec2& point) const {
    Vec2 relative_pos = {point.x - position.x, position.y - point.y};
    Vec2 normalized = relative_pos / bounds + Vec2::one() * 0.5f; // 0 to 1
        
    return Vec2 {
        normalized.x * window_width,
        normalized.y * window_height
    };
}
Vec2 Cam::world_to_screen_scale(const Vec2& scale) const {
    return { world_to_screen_scale(scale.x), world_to_screen_scale(scale.y) };
}

float Cam::world_to_screen_scale(const float& length) const {
    return length / bounds.y * window_height;

}

Vec2 Cam::screen_to_world(const Vec2& point) const {
    Vec2 normalized = { point.x / window_width, point.y / window_height };
    Vec2 relative_pos = (normalized - Vec2::one() * 0.5f) * bounds;
    return { relative_pos.x + position.x, position.y - relative_pos.y };
}


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

class Map {
public:
    Map() = default;
    //Map(const Map& map) {
    //    std::cout << "copy\n";
    //}
    void insert_note(double time, NoteFlags flags);
    void remove_note(int i);

    template<class Archive>
    void save(Archive& ar) const {
        ar(times, flags_vec);
    }

    template<class Archive>
    void load(Archive& ar) {
        ar(times, flags_vec);

        selected = std::vector<bool>(times.size(), false);
    }
    
    std::string music_file;
    double bpm;
    double offset;

    std::vector<double> times;
    std::vector<NoteFlags> flags_vec;
    std::vector<bool> selected;
};

void Map::insert_note(double time, NoteFlags flags) {
    int i = std::upper_bound(times.begin(), times.end(), time) - times.begin();
    times.insert(times.begin() + i, time);
    flags_vec.insert(flags_vec.begin() + i, flags);
    selected.insert(selected.begin() + i, false);
}

void Map::remove_note(int i) {
    times.erase(times.begin() + i);
    flags_vec.erase(flags_vec.begin() + i);
    selected.erase(selected.begin() + i);
}


std::filesystem::path map_path(const char* file_name) {
    return "data/maps/" + std::string(file_name);
}

void save_map(const Map& map, const char* file_name) {
    std::ofstream fout(map_path(file_name).string().data());

    cereal::BinaryOutputArchive oarchive(fout);

    oarchive(map);
}

void load_map(Map* map) {
    std::ifstream fin("map.tnt");

    cereal::BinaryInputArchive iarchive(fin);

    iarchive(*map);
}

void draw_map(SDL_Renderer* renderer, const Map& map, const Cam& cam, int current_note) {
    ZoneScoped;

    int right = map.times.size() - 1;
    int left = 0;
    constexpr float circle_padding = 0.2f;
    float right_bound = cam.position.x + cam.bounds.x / 2 + circle_padding;
    float left_bound = cam.position.x - (cam.bounds.x / 2 + circle_padding);

    for (int i = current_note; i < map.times.size(); i++) {
        if (map.times[i] >= right_bound) {
            right = i - 1;
            break;
        }
    }
    for (int i = current_note; i >= 0; i--) {
        if (i >= map.times.size()) {
            continue;
        }
        if (map.times[i] <= left_bound) {
            left = i + 1;
            break;
        }
    }

    for (int i = right; i >= left; i--) {
        Vec2 center_pos = cam.world_to_screen({ (float)map.times[i], 0 });

        float scale = (map.flags_vec[i] & NoteFlagBits::normal_or_big) ? 0.9f : 1.4f;
        Image circle_image = (map.flags_vec[i] & NoteFlagBits::don_or_kat) ? assets.get_image("don_circle") : assets.get_image("kat_circle");
        Image circle_overlay = assets.get_image("circle_overlay");
        Image select_circle = assets.get_image("select_circle");

        Vec2 circle_pos = center_pos;
        circle_pos.x -= circle_image.width / 2 * scale;
        circle_pos.y -= circle_image.height / 2 * scale;


        {
            SDL_FRect rect = { circle_pos.x, circle_pos.y, circle_image.width * scale, circle_image.height * scale };
            SDL_RenderTexture(renderer, circle_image.texture, NULL, &rect);
            SDL_RenderTexture(renderer, circle_overlay.texture, NULL, &rect);
        }

    }
}

const Vec2 note_hitbox = { 0.25, 0.25 };

void draw_map_editor(SDL_Renderer* renderer, const Map& map, const Cam& cam, int current_note) {
    ZoneScoped;

    if (map.times.size() == 0) {
        return;
    }

    int right = map.times.size() - 1;
    int left = 0;
    constexpr float circle_padding = 0.2f;
    float right_bound = cam.position.x + cam.bounds.x / 2 + circle_padding;
    float left_bound = cam.position.x - (cam.bounds.x / 2 + circle_padding);

    for (int i = current_note; i < map.times.size(); i++) {
        if (map.times[i] >= right_bound) {
            right = i - 1;
            break;
        }
    }
    for (int i = current_note; i >= 0; i--) {
        if (i >= map.times.size()) {
            continue;
        }
        if (map.times[i] <= left_bound) {
            left = i + 1;
            break;
        }
    }

    for(int i = right; i >= left; i--) {
        Vec2 center_pos = cam.world_to_screen({(float)map.times[i], 0});

        float scale = (map.flags_vec[i] & NoteFlagBits::normal_or_big) ? 0.9f : 1.4f;
        Image circle_image = (map.flags_vec[i] & NoteFlagBits::don_or_kat) ? assets.get_image("don_circle") : assets.get_image("kat_circle");
        Image circle_overlay = assets.get_image("circle_overlay");
        Image select_circle = assets.get_image("select_circle");

        Vec2 circle_pos = center_pos;
        circle_pos.x -= circle_image.width / 2 * scale;
        circle_pos.y -= circle_image.height / 2 * scale;


        {
            SDL_FRect rect = { circle_pos.x, circle_pos.y, circle_image.width * scale, circle_image.height * scale };
            SDL_RenderTexture(renderer, circle_image.texture, NULL, &rect);
            SDL_RenderTexture(renderer, circle_overlay.texture, NULL, &rect);
        }

        Vec2 hitbox_bounds = cam.world_to_screen_scale(note_hitbox);
        Vec2 hitbox_pos = center_pos - (hitbox_bounds / 2.0f);
        //draw_wire_box(hitbox_pos, hitbox_bounds, RED);


        if (map.selected[i]) {
            SDL_FRect rect = {
                center_pos.x - select_circle.width / 2 * scale,
                center_pos.y - select_circle.height / 2 * scale,
                select_circle.width * scale,
                select_circle.height * scale,
            };
            SDL_RenderTexture(renderer, select_circle.texture, NULL, &rect);
        }
    }
}


//using InputFlags = uint8_t;
//
//enum InputFlagBits : InputFlags {
//    left = 1 << 0,
//    don = 1 << 1
//};

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
    Game() {};
    Game(Input* _input, Audio* _audio, SDL_Renderer* _renderer, Map _map);
    //Game& operator=(const Game&) = default;
    void start();
    void update(std::chrono::duration<double> delta_time);
private:
    Input* input_ptr = nullptr;
    Audio* audio_ptr = nullptr;
    SDL_Renderer* renderer = nullptr;
    Cam cam = {{0,0}, {2,1.5f}};

    int current_note = 0;

    int score = 0;
    int combo = 0;

    //std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::high_resolution_clock::now();

    UI ui{};

    std::vector<InputRecord> input_history;

    Map map{};

    //std::vector<Particle> particles;

    BigNoteHits current_big_note_status;

    bool auto_mode = true;

    bool initialized = false;
};

Game::Game(Input* _input, Audio* _audio, SDL_Renderer* _renderer, Map _map)
    : input_ptr{ _input }, audio_ptr{ _audio }, renderer{ _renderer }, map{ _map }, ui{ window_width, window_height } {
}

const double input_indicator_duration = 0.1;

void Game::start() {
    audio_ptr->play();
}

void Game::update(std::chrono::duration<double> delta_time) {

    Input& input = *input_ptr;
    if (!initialized) {
        this->start();
        initialized = true;
    }

    double elapsed = audio_ptr->get_position();

    if (input.key_down(SDL_SCANCODE_ESCAPE)) {
        g_event_queue.push_event(EventType::TestToEditor);
        return;
    }

    std::vector<DrumInput> inputs{};

    if (auto_mode) {
        if (current_note < map.times.size()) {
            if (elapsed >= map.times[current_note]) {
                if (map.flags_vec[current_note] & NoteFlagBits::don_or_kat) {
                    input_history.push_back(InputRecord{ DrumInput::don_left, elapsed });
                    inputs.push_back(DrumInput::don_left);
                }
                else {
                    input_history.push_back(InputRecord{ DrumInput::kat_left, elapsed });
                    inputs.push_back(DrumInput::kat_left);
                }
            }
        }
    }

    if (input.key_down(SDL_SCANCODE_X)) {
        input_history.push_back(InputRecord{ DrumInput::don_left, elapsed });
        inputs.push_back(DrumInput::don_left);
    }

    if (input.key_down(SDL_SCANCODE_PERIOD)) {
        input_history.push_back(InputRecord{ DrumInput::don_right, elapsed });
        inputs.push_back(DrumInput::don_right);
    }

    if (input.key_down(SDL_SCANCODE_Z)) {
        input_history.push_back(InputRecord{ DrumInput::kat_left, elapsed });
        inputs.push_back(DrumInput::kat_left);
    }

    if (input.key_down(SDL_SCANCODE_SLASH)) {
        input_history.push_back(InputRecord{ DrumInput::kat_right, elapsed });
        inputs.push_back(DrumInput::kat_right);
    }

    for (const auto& input : inputs) {
        if ( input == DrumInput::don_left || input == DrumInput::don_right) {
            Mix_PlayChannel(-1, assets.get_sound("don"), 0);
        } else if (input == DrumInput::kat_left || input == DrumInput::kat_right) {
            Mix_PlayChannel(-1, assets.get_sound("kat"), 0);
        }
    }

    if (current_note < map.times.size()) {
        for (const auto& thing : inputs) {
            double hit_normalized = (elapsed - map.times[current_note]) / hit_range.count() + 0.5;
            if (hit_normalized >= 0 && hit_normalized < 1) {
                if (map.flags_vec[current_note] & NoteFlagBits::normal_or_big) {
                    auto actual_type = (uint8_t)(map.flags_vec[current_note] & NoteFlagBits::don_or_kat);
                    auto input_type = (uint8_t)(thing & DrumInputFlagBits::don_kat);
                    if (actual_type == input_type) {
                        score += 300;
                        combo++;
                        current_note++;
                    }
                    //particles.push_back(Particle{ Vec2{(float)elapsed.count(), 0}, {0,1},1, note.type, elapsed });
                } else {
                    

                }
            }

        }

        //if (elapsed > map[current_note].time - (hit_range / 2)) {
        //    //PlaySound(don_sound);
        //    particles.push_back(Particle{ Vec2{(float)elapsed.count(), 0}, {0,1}, 1, map[current_note].type, elapsed });
        //    current_note++;
        //}
    }

    cam.position.x = elapsed;

    //for (int i = particles.size() - 1; i >= 0; i--) {
    //    Particle& p = particles[i];
    //    p.position += p.velocity * delta_time.count();

    //    if (elapsed - p.start > 1s) {
    //        particles.erase(particles.begin() + i);
    //    }
    //}


    //Style style{};
    //style.anchor = { 1,0 };
    //ui.begin_group(style);
    //std::string score_text = std::to_string(score);
    //ui.rect(score_text.data());
    //ui.end_group();
    ui.begin_group(Style{ {1,0} }); 
        auto score_text = std::to_string(score);
        ui.rect(score_text.data(), {});
    ui.end_group();

    ui.begin_group(Style{ {0,1} }); 
        auto combo_text = std::format("{}x", combo);
        ui.rect(combo_text.data(), {});
    ui.end_group();

    ui.begin_group(Style{ {1,1} }); 
        auto frame_time = std::to_string(((float)std::chrono::duration_cast<std::chrono::microseconds>(delta_time).count()) / 1000) + " ms";
        ui.rect(frame_time.data(), {});
    ui.end_group();

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    float x = elapsed;
    Vec2 p1 = cam.world_to_screen({ x, 0.5f });
    Vec2 p2 = cam.world_to_screen({ x, -0.5f });

    SDL_SetRenderDrawColor(renderer, 255, 255, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);

    draw_map_editor(renderer, map, cam, current_note);

    auto inner_drum = assets.get_image("inner_drum");
    auto outer_drum = assets.get_image("outer_drum");
    Vec2 drum_pos = { 0, (window_height - inner_drum.height) / 2};

    auto left_rect = SDL_FRect{ drum_pos.x, drum_pos.y, (float)inner_drum.width, (float)inner_drum.height };
    auto right_rect = SDL_FRect{ drum_pos.x + inner_drum.width, drum_pos.y, (float)inner_drum.width, (float)inner_drum.height };

    for (int i = input_history.size() - 1; i >= 0; i--) {
        const InputRecord& input = input_history[i];
        if (elapsed - input.time > input_indicator_duration) {
            break;
        }

        switch (input.type) {
        case DrumInput::don_left:
            SDL_RenderTexture(renderer, inner_drum.texture, NULL, &left_rect);
            break;
        case DrumInput::don_right:
            SDL_RenderTextureRotated(renderer, inner_drum.texture, NULL, &right_rect, 0, NULL, SDL_FLIP_HORIZONTAL);
            break;
        case DrumInput::kat_left:
            SDL_RenderTextureRotated(renderer, outer_drum.texture, NULL, &left_rect, 0, NULL, SDL_FLIP_HORIZONTAL);
            break;
        case DrumInput::kat_right:
            SDL_RenderTexture(renderer, outer_drum.texture, NULL, &right_rect);
            break;
        }
    }


    Vec2 target = cam.world_to_screen(cam.position);
    //DrawCircle(target.x, target.y, 50, WHITE);
    //DrawText((std::to_string(delta_time.count() * 1000) + " ms").data(), 100, 100, 24, WHITE);
    //draw_particles(cam, particles, elapsed);

    ui.draw(renderer);

    SDL_RenderPresent(renderer);
}


std::vector<int> note_box_intersection(const Map& map, Vec2 start_pos, Vec2 end_pos) {
    if (end_pos.x < start_pos.x) {
        float temp = start_pos.x;
        start_pos.x = end_pos.x;
        end_pos.x = temp;
    }
    
    if (end_pos.y > start_pos.y) {
        float temp = start_pos.y;
        start_pos.y = end_pos.y;
        end_pos.y = temp;
    }

    std::vector<int> hits;

    for (int i = 0; i < map.times.size(); i++) {
        const float& x = map.times[i];
        if (x > start_pos.x && x < end_pos.x && start_pos.y > 0 && end_pos.y < 0) {
            hits.push_back(i);
        }
    }
 
    return hits;
}

std::optional<int> note_point_intersection(const Map& map, const Vec2& point, const int& current_note) {
    Vec2 half_bounds = note_hitbox / 2;
    std::vector<int> hits;
    for (int i = 0; i < map.times.size(); i++) {
        Vec2 center = { static_cast<float>(map.times[i]), 0 };
        Vec2 top_left = { center.x - half_bounds.x, center.y + half_bounds.y };
        Vec2 bottom_right = { center.x + half_bounds.x, center.y - half_bounds.y };

        if (point.x > top_left.x && point.y < top_left.y && point.x < bottom_right.x && point.y > bottom_right.y) {
            hits.push_back(i);
        }

    }

    if (hits.size() == 0) {
        return {};
    }

    int min = hits[0];

    for (int i = 1; i < hits.size(); i++) {
        if (std::abs(map.times[hits[i]] - point.x) < std::abs(map.times[min] - point.x)) {
            min = hits[i];
        }
    }

    return min;
}

//SDL_FPoint vec2_to_sdl_fpoint(const Vec2& vec) {
//    return { vec.x, vec.y };
//}
//
//void draw_wire_box(SDL_Renderer* renderer, const Vec2& pos, const Vec2& scale) {
//    SDL_FPoint points[5];
//    points[0] = vec2_to_sdl_fpoint(pos);
//    points[1] = vec2_to_sdl_fpoint(pos + Vec2{1, 0} *scale);
//    points[2] = vec2_to_sdl_fpoint(pos + scale);
//    points[3] = vec2_to_sdl_fpoint(pos + Vec2{0, 1} *scale);
//    points[4] = points[0];
//    SDL_RenderLines(renderer, points, 5);
//}



void set_draw_color(SDL_Renderer* renderer, Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

enum class EditorMode {
    select,
    insert,
};


class Editor {
public:
    Editor(Input& _input, Audio& _audio, SDL_Renderer* _renderer);
    ~Editor();
    void update(std::chrono::duration<double> delta_time);
    void init();
    Map map{};

    double last_pos{};
private:
    Input& input;
    Audio& audio;
    SDL_Renderer* renderer;

    Cam cam = {{0,0}, {2,1.5f}};

    EditorMode editor_mode = EditorMode::select;
    std::vector<int> selected;

    std::optional<Vec2> box_select_begin;

    NoteFlags insert_flags = NoteFlagBits::don_or_kat | NoteFlagBits::normal_or_big;
    //NoteType note_type;


    //std::vector<Note> map;
    double offset = 4.390f;
    float bpm = 190;


    double quarter_interval = 60 / bpm / 4;
    double collision_range = quarter_interval / 2;

    bool paused = true;

    int current_note = -1;

    UI ui{};

    //Mix_Music* music;

    TextFieldState name;

    bool creating_map = true;
};

Editor::Editor(Input& _input, Audio& _audio, SDL_Renderer* _renderer)
    : input{ _input }, audio{ _audio }, renderer{ _renderer }, ui{ window_width, window_height } {
}

Editor::~Editor() {
    //Mix_FreeMusic(music);
    //Mix_FreeChunk(don_sound);
    //Mix_FreeChunk(kat_sound);
}

auto last = std::chrono::high_resolution_clock::now();


void Editor::init() {
}

void Editor::update(std::chrono::duration<double> delta_time) {
    ZoneScoped;
    //double elapsed = Mix_GetMusicPosition(music);
    double elapsed = audio.get_position();
    last_pos = elapsed;


    //if (!paused) {
    //    elapsed = Mix_GetMusicPosition(music);
    //} else {
    //    elapsed = cam.position.x;
    //}
    Style style{};
    style.anchor = { 0.5, 0.5 };
    style.stack_direction = StackDirection::Vertical;


    if (creating_map) {
        ui.begin_group(style);

        ui.text_field(&name, { .border_color = {255, 255, 255, 0}, .min_width = 200 });

        ui.button("enter", {}, [&]() {
            creating_map = false;
            });

        ui.end_group();
    }

    style.anchor = { 0, 0.5 };
    style.stack_direction = StackDirection::Vertical;

    ui.begin_group(style); {
        const char* note_color_text = (insert_flags & NoteFlagBits::don_or_kat) ? "Don" : "Kat";

        const char* note_size_text = (insert_flags & NoteFlagBits::normal_or_big) ? "Normal" : "Big";

        const char* editor_mode_text;
        switch (editor_mode) {
        case EditorMode::select:
            editor_mode_text = "Select";
            break;
        case EditorMode::insert:
            editor_mode_text = "Insert";
            break;
        }

        ui.button(editor_mode_text, {}, [&]() {
            if (editor_mode == EditorMode::insert) {
                editor_mode = EditorMode::select;
            }
            else {
                editor_mode = EditorMode::insert;
            }
        });

        ui.button(note_size_text, {}, [&]() {
            insert_flags = insert_flags ^ NoteFlagBits::normal_or_big;
        });

        ui.button(note_color_text, {}, [&]() {
            insert_flags = insert_flags ^ NoteFlagBits::don_or_kat;
        });
    }
    ui.end_group();

    ui.begin_group(Style{ {0,1} });
        std::string time = std::to_string(cam.position.x) + " s";
        ui.rect(time.data(), {});
    //ui.slider(elapsed / GetMusicTimeLength(music), [&](float fraction) {
    //    SeekMusicStream(music, fraction * GetMusicTimeLength(music));
    //});
    ui.end_group();

    ui.begin_group(Style{ {1,1} }); 
        auto frame_time = std::to_string(((float)std::chrono::duration_cast<std::chrono::microseconds>(delta_time).count()) / 1000) + " ms";
        ui.rect(frame_time.data(), {});
    ui.end_group();

    ui.input(input);

    Vec2 cursor_pos = cam.screen_to_world(input.mouse_pos);

    std::optional<int> to_select = note_point_intersection(map, cursor_pos, current_note);
    if (input.mouse_down(SDL_BUTTON_RMASK) && to_select.has_value()) {
        if (map.selected[to_select.value()]) {
            for (int i = map.selected.size() - 1; i >= 0; i--) {
                if (map.selected[i]) {
                    map.remove_note(i);
                }
            }
        }
        else {
            map.remove_note(to_select.value());
        }
    }

    if (input.key_down(SDL_SCANCODE_S) && input.modifier(SDL_KMOD_LCTRL)) {
        //save_map(map);
    }

    if (input.key_down(SDL_SCANCODE_O) && input.modifier(SDL_KMOD_LCTRL)) {
        //load_map(&map);
    }

    if (input.key_down(SDL_SCANCODE_A) && input.modifier(SDL_KMOD_LCTRL)) {
        std::fill(map.selected.begin(), map.selected.end(), true);
    }

    float seek = 0;
    seek += input.wheel;

    if (input.key_down(SDL_SCANCODE_LEFT)) {
        if (input.modifier(SDL_KMOD_LCTRL)) {
            seek += 4;
        }
        else {
            seek += 1;
        }
    }

    if (input.key_down(SDL_SCANCODE_RIGHT)) {
        if (input.modifier(SDL_KMOD_LCTRL)) {
            seek -= 4;
        }
        else {
            seek -= 1;
        }
    }

    if (seek != 0) {
        int i = std::round((elapsed - offset) / quarter_interval);
        double seek_double = offset + (i - seek) * quarter_interval;
        audio.set_position(seek_double);
        elapsed = audio.get_position();
    }
    cam.position.x = elapsed;

    switch (editor_mode) {
    case EditorMode::select:
        if (!ui.clicked && input.mouse_down(SDL_BUTTON_LMASK)) {
            if (to_select.has_value()) {
                map.selected[to_select.value()] = !map.selected[to_select.value()];
            } else { 
                box_select_begin = cursor_pos;
            }
        }

        if (input.mouse_held(SDL_BUTTON_LMASK)) {
            if (box_select_begin.has_value()) {
                auto hits = note_box_intersection(map, box_select_begin.value(), cursor_pos);
                std::fill(map.selected.begin(), map.selected.end(), false);

                for (auto& i : hits) {
                    map.selected[i] = true;
                }
            }

        }

        if (input.mouse_up(SDL_BUTTON_LMASK)) {
            box_select_begin = {};
        }
        break;

    case EditorMode::insert:
        if (!ui.clicked && input.mouse_down(SDL_BUTTON_LMASK)) {
            int i = std::round((cursor_pos.x - offset) / quarter_interval);
            float time = offset + i * quarter_interval;

            bool collision = false;
            double diff;
            for (int i = 0; i < map.times.size(); i++) {
                diff = std::abs((map.times[i] - time));
                if (diff < collision_range) {
                    collision = true;
                    break;
                }
            }

            if (!collision) {
                map.insert_note(time, insert_flags);

                if (time < elapsed) {
                    current_note++;
                }
            }
        }
        break;
    }

    if (input.key_down(SDL_SCANCODE_F5)) {
        g_event_queue.push_event(EventType::EditorToTest);
    }

    if (input.key_down(SDL_SCANCODE_Q)) { 
        editor_mode = EditorMode::select;
    }

    if (input.key_down(SDL_SCANCODE_W)) { 
        editor_mode = EditorMode::insert;
    }

    if (input.key_down(SDL_SCANCODE_E)) {
        insert_flags = insert_flags ^ NoteFlagBits::normal_or_big;
    }

    if (input.key_down(SDL_SCANCODE_R)) {
        insert_flags = insert_flags ^ NoteFlagBits::don_or_kat;
    }

    if (input.key_down(SDL_SCANCODE_SPACE)) {
        if (audio.paused()) {
            audio.play();
            current_note = std::upper_bound(map.times.begin(), map.times.end(), elapsed) - map.times.begin();
        } else {
            audio.pause();
        }
    }

    if (!audio.paused() && current_note < map.times.size() && elapsed >= map.times[current_note]) {
        switch (map.flags_vec[current_note]) {
        case (NoteFlagBits::don_or_kat | NoteFlagBits::normal_or_big):
            Mix_PlayChannel(-1, assets.get_sound("don"), 0);
            break;
        case (0 | NoteFlagBits::normal_or_big):
            Mix_PlayChannel(-1, assets.get_sound("kat"), 0);
            break;
        case (NoteFlagBits::don_or_kat | 0):
            Mix_PlayChannel(-1, assets.get_sound("kat"), 0);
            break;
        case (0 | 0):
            Mix_PlayChannel(-1, assets.get_sound("kat"), 0);
            break;
        }

        current_note++;
    }



    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    float right_bound =cam.position.x + cam.bounds.x / 2;
    float left_bound = cam.position.x - cam.bounds.x / 2;

    int start = (left_bound - offset) / quarter_interval + 2;
    int end = (right_bound - offset) / quarter_interval - 1;

    for (int i = start; i < end; i++) {
        float x = offset + i * quarter_interval;

        if (x > right_bound) {
            break;
        }

        float height;
        Color color;
        if (i % 4 == 0) {
            height = 0.2;
            color = { 255, 255, 255 };
        } else {
            height = 0.1;
            color = { 255, 0, 0 };
        }
        
        Vec2 p1 = cam.world_to_screen({x, 0});
        Vec2 p2 = cam.world_to_screen({x, height});

        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, SDL_ALPHA_OPAQUE);
        SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
    }

    Vec2 p1 = cam.world_to_screen(cam.position);
    Vec2 p2 = cam.world_to_screen(cam.position + Vec2{0,0.6});

    SDL_SetRenderDrawColor(renderer, 255, 255, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);

    draw_map_editor(renderer, map, cam, current_note);

    if (box_select_begin.has_value()) {
        Vec2 start_pos = cam.world_to_screen(box_select_begin.value());
        Vec2 end_pos = cam.world_to_screen(cursor_pos);

        if (end_pos.x < start_pos.x) {
            float temp = start_pos.x;
            start_pos.x = end_pos.x;
            end_pos.x = temp;
        }
        
        if (end_pos.y < start_pos.y) {
            float temp = start_pos.y;
            start_pos.y = end_pos.y;
            end_pos.y = temp;

        }

        SDL_FRect rect = { start_pos.x, start_pos.y, end_pos.x - start_pos.x, end_pos.y - start_pos.y };

        set_draw_color(renderer, { 255, 255, 255, 40 });
        SDL_RenderRect(renderer, &rect);
    }

    ui.draw(renderer);


    {
        ZoneNamedN(jksfdgjh, "Render Present", true);
        SDL_RenderPresent(renderer);
    }
}

//enum class View {
//    main,
//    map_select,
//};

struct MapEntry {
    std::string title;
    std::string artist;
    std::string difficulty_name;
};

enum class EntryMode {
    Play,
    Edit,
};

class MainMenu {
public:
    MainMenu(Input* _input, Audio* _audio, SDL_Renderer* _renderer);
    
    void update();

private:
    UI ui;
    Input* input_ptr;
    Audio* audio_ptr;
    SDL_Renderer* renderer;

    EntryMode entry_mode = EntryMode::Play;

    std::vector<MapEntry> map_list;

    TextFieldState search{.text = "ashkjfhkjh"};
};

MainMenu::MainMenu(Input* _input, Audio* _audio, SDL_Renderer* _renderer)
    : input_ptr{ _input }, audio_ptr{ _audio }, renderer{ _renderer }, ui{ window_width, window_height } {}

struct ButtonInfo {
    const char* text;
    Style style;
    std::function<void()> on_click;
};

void MainMenu::update() {
    Input& input = *input_ptr;
    Audio& audio = *audio_ptr;


    std::vector<ButtonInfo> option;



    Style inactive_style{};
    inactive_style.text_color = { 128, 128 ,128, 255 };

    Style active_style{};

    option.push_back({ "Play", inactive_style, [&]() {
        entry_mode = EntryMode::Play;
    }});

    option.push_back({ "Edit", inactive_style, [&]() {
        entry_mode = EntryMode::Edit;
    }});


    if (entry_mode == EntryMode::Play) {
        option[0].style = active_style;
    }
    else {
        option[1].style = active_style;
    }

    Style style{};

    ui.begin_group(style);

    for (auto& info : option) {
        ui.button(info.text, info.style, info.on_click);
    }

    ui.end_group();

    if (entry_mode == EntryMode::Edit) {
        Style style{};
        style.anchor = { 0.25f, 0.5f };
        ui.begin_group(style);

        auto callback = [](void* userdata, const char* const* filelist, int filter) {
            std::cout << std::format("{}\n", (int)filelist);
            std::cout << std::format("{}\n", (int)filelist[0]);

            if (filelist[0] != nullptr) {
                //std::cout << "jkhaskdhfklsjahdkf";
                //std::filesystem::create_directory("maps");
                g_event_queue.push_event(EventType::ToEditor);
            }
            else {
                std::cout << "vcbh";
            }
            };


        ui.button("New Map", {}, [&]() {
            SDL_ShowOpenFileDialog(callback, NULL, NULL, NULL, 0, NULL, 0);
        });

        ui.end_group();
    }


    //Style style = {};
    //style.anchor = { 0.5, 0.25 };

    //ui.begin_group(style);

    //ui.text_field(&search, {});

    //ui.end_group();


    //style = {};
    //style.anchor = { 0.5, 0.5 };
    //style.background_color = { 255, 0, 0, 255 };
    //style.gap = 36;


    //ui.begin_group(style);
    //
    //ui.rect("1 djkfhgkjh", {});

    //style = {};
    //style.background_color = { 0, 0, 255, 255 };
    //style.stack_direction = StackDirection::Vertical;
    //ui.begin_group(style);
    //ui.rect("2 aslkdjhf", {});
    //ui.rect("aslkdjhf", {});
    //ui.rect("aslkdjhf", {});
    //ui.rect("aslkdjhf", {});
    //ui.end_group();

    //ui.begin_group(style);
    //ui.rect("3 aslkdjhf", { .text_color = { 255, 0, 0, 255} });
    //ui.rect("aslkdjhf", {});
    //ui.rect("aslkdjhf", {});
    //ui.rect("aslkdjhf", {});
    //ui.end_group();

    //ui.begin_group(style);
    //ui.rect("4 aslkdjhf", {});
    //ui.rect("aslkdjhf", {});
    //ui.rect("aslkdjhf", {});
    //ui.rect("aslkdjhf", {});
    //ui.end_group();
    //        
    //ui.end_group();



    ui.input(input);


    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);


    ui.draw(renderer);
    SDL_RenderPresent(renderer);
}

enum class Context {
    Menu,
    Editor,
    Game,
};

void fake_loop() {
    while (1) {
        ZoneScoped;
        int j;
        for (int i = 0; i < 10000000; i++) {
            j = i;
        }

        std::cout << "other frame\n";
        FrameMarkNamed("other thread");
    }
}

int run() {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        SDL_Log("SDL_Init failed (%s)", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        std::cout << std::format("{}\n", TTF_GetError());
        return 1;
    }

    SDL_Window* window;
    SDL_Renderer* renderer;

    SDL_CreateWindowAndRenderer("", window_width, window_height, 0, &window, &renderer);
    //SDL_SetWindowFullscreen(window, true);

    auto start = std::chrono::high_resolution_clock::now();

    Input input{};
    Audio audio{};
    

    Context context = Context::Menu;


    auto editor = Editor{input, audio, renderer};


    Game game;

    Map map{};
    MainMenu menu{&input, &audio, renderer};

    editor.init();

    Mix_MasterVolume(MIX_MAX_VOLUME * 0.25);
    Mix_VolumeMusic(MIX_MAX_VOLUME * 0.1);

    auto last_frame = std::chrono::high_resolution_clock::now();

    init_font(renderer);

    std::vector<SoundLoadInfo> sound_list = {
        {"don.wav", "don"},
        {"kat.wav", "kat"},
    };

    std::vector<ImageLoadInfo> image_list = {
        {"circle-overlay.png", "circle_overlay"},
        {"circle-select.png", "select_circle"},
        {"circle.png", "kat_circle", Color{ 60, 219, 226, 255 }},
        {"circle.png", "don_circle", Color{ 252, 78, 60, 255 }},
        {"drum-inner.png", "inner_drum"},
        {"drum-outer.png", "outer_drum"},
    };

    assets.init(renderer, image_list, sound_list);

    //auto future = std::async(std::launch::async, fake_loop);

    SDL_StartTextInput();

    while (1) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> delta_time = now - last_frame;

        if (delta_time < 1ms) {
            continue;
        }

        ZoneNamedN(var, "update", true);

        last_frame = now;

        float total_wheel{};
        bool moved{};

        input.begin_frame();
        input.input_text = std::nullopt;

        SDL_Event event;
        bool quit = false;
        {
            ZoneNamedN(var, "poll input", true);
            while (SDL_PollEvent(&event)) {
                ZoneNamedN(var, "event", true);
                if (event.type == SDL_EVENT_QUIT) {
                    quit = true;
                    break;
                }

                if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                    total_wheel += event.wheel.y;
                }


                if (event.type == SDL_EVENT_KEY_DOWN) {
                    input.keyboard_repeat[event.key.scancode] = true;
                }


                if (event.type == SDL_EVENT_TEXT_INPUT) {
                    input.input_text = std::string(event.text.text);
                }
            }
        }
        if (quit) {
            break;
        }

        input.wheel = total_wheel;



        EventType switch_event;
        while (g_event_queue.pop_event(&switch_event)) {
            switch (switch_event) {
            case EventType::EditorToTest:
                game = { &input, &audio, renderer, editor.map};
                context = Context::Game;
                break;
            case EventType::TestToEditor:
                game = {};
                audio.set_position(editor.last_pos);
                audio.pause();
                context = Context::Editor;
                break;
            case EventType::ToEditor:
                context = Context::Editor;
            }

        }

        switch (context) {
        case Context::Menu:
            menu.update();
            break;
        case Context::Editor:
            editor.update(delta_time);
            break;
        case Context::Game:
            game.update(delta_time);
            break;
        }

        input.end_frame();

        FrameMark;
    }


    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
}

int main() {
    return run();
}
