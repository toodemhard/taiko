#include <tracy/Tracy.hpp>

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


#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>

#include "vec.h"
#include "ui.h"
#include "audio.h"
#include "font.h"

using namespace std::chrono_literals;

const int window_width = 1280;
const int window_height = 960;

const std::chrono::duration<double> hit_range = 100ms;
const std::chrono::duration<double> perfect_range = 30ms;

const float circle_radius = 0.1f;
const float circle_outer_radius = 0.11f;

enum NoteFlagBits : uint8_t {
    don_or_kat = 1 << 0, // true = don | false = kat
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


//void draw_map(const std::vector<Note>& map, const Cam& cam, int current_note) {
//    if (map.size() == 0) {
//        return;
//    }
//
//    int right = map.size() - 1;
//    constexpr float circle_padding = 0.2f;
//    float right_bound = cam.position.x + cam.bounds.x / 2 + circle_padding;
//
//    for (int i = current_note; i < map.size(); i++) {
//        if (map[i].time.count() >= right_bound) {
//            right = i - 1;
//            break;
//        }
//    }
//
//    for(int i = right; i >= current_note; i--) {
//        const Note& note = map[i];
//        Vec2 circle_pos = cam.world_to_screen({(float)note.time.count(), 0});
//        Color color = (note.type == NoteType::don) ? RED : BLUE;
//
//        DrawCircle(circle_pos.x, circle_pos.y, cam.world_to_screen_scale(circle_outer_radius), WHITE);
//        DrawCircle(circle_pos.x, circle_pos.y, cam.world_to_screen_scale(circle_radius), color);
//    }
//}
//
//const float particle_duration = 1.0f;
//
//struct Particle {
//    Vec2 position;
//    Vec2 velocity;
//    float scale;
//    NoteType type;
//    std::chrono::duration<double> start;
//};
//
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

//enum class Input {
//    don_left,
//    don_right,
//    kat_left,
//    kat_right,
//};
//
//struct InputRecord {
//    Input type;
//    float time;
//};

class Map {
public:
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


void save_map(const Map& map) {
    std::ofstream fout("map.tnt");

    cereal::BinaryOutputArchive oarchive(fout);

    oarchive(map);
}

void load_map(Map* map) {
    std::ifstream fin("map.tnt");

    cereal::BinaryInputArchive iarchive(fin);

    iarchive(*map);
}

//class Game {
//public:
//    Game();
//    void run();
//    void update(std::chrono::duration<double> delta_time);
//private:
//    Sound don_sound = LoadSound("don.wav");
//    Sound kat_sound = LoadSound("kat.wav");
//    
//    Cam cam = {{0,0}, {4,3}};
//
//    std::vector<Note> map{};
//    int current_note = 0;
//
//    std::vector<Particle> particles;
//
//    int score = 0;
//
//    std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::high_resolution_clock::now();
//
//    std::vector<InputRecord> inputs;
//};
//
//Game::Game() {
//    for (int i = 0; i < 100; i++) {
//        map.push_back({
//            (0.2371s / 2) * i + 0.4743s,
//            NoteType::don
//        });
//    }
//}
//
//void Game::run() {
//
//}
//
//const float input_indicator_duration = 0.1f;
//
//void Game::update(std::chrono::duration<double> delta_time) {
//    auto now = std::chrono::high_resolution_clock::now();
//    std::chrono::duration<double> elapsed = now - start;
//
//    //bool inputs[4] = { false, false, false, false };
//    //std::array<bool, 4> inputs = { false, false, false, false };
//    std::vector<NoteType> pressed;
//
//
//    if (IsKeyPressed(KEY_X)) {
//        PlaySound(don_sound);
//        inputs.push_back(InputRecord{ Input::don_left, (float) elapsed.count() });
//
//        pressed.push_back(NoteType::don);
//    }
//
//    if (IsKeyPressed(KEY_PERIOD)) {
//        PlaySound(don_sound);
//        inputs.push_back(InputRecord{ Input::don_right, (float)elapsed.count() });
//
//        pressed.push_back(NoteType::don);
//    }
//
//    if (IsKeyPressed(KEY_Z)) {
//        inputs.push_back(InputRecord{ Input::kat_left, (float)elapsed.count() });
//        PlaySound(kat_sound);
//
//        pressed.push_back(NoteType::kat);
//    }
//
//    if (IsKeyPressed(KEY_SLASH)) {
//        inputs.push_back(InputRecord{ Input::kat_right, (float)elapsed.count() });
//        PlaySound(kat_sound);
//
//        pressed.push_back(NoteType::kat);
//    }
//
//    if (current_note < map.size()) {
//        for (auto& note : pressed) {
//            double hit_normalized = (elapsed - map[current_note].time) / hit_range + 0.5;
//            if (hit_normalized >= 0 && hit_normalized < 1) {
//                if (map[current_note].type == note) {
//                    score += 300;
//                    //particles.push_back(Particle{ Vec2{(float)elapsed.count(), 0}, {0,1},1, note.type, elapsed });
//                    current_note++;
//                }
//            }
//
//        }
//
//        if (elapsed > map[current_note].time - (hit_range / 2)) {
//            //PlaySound(don_sound);
//            particles.push_back(Particle{ Vec2{(float)elapsed.count(), 0}, {0,1}, 1, map[current_note].type, elapsed });
//            current_note++;
//        }
//    }
//
//    cam.position.x = elapsed.count();
//
//    for (int i = particles.size() - 1; i >= 0; i--) {
//        Particle& p = particles[i];
//        p.position += p.velocity * delta_time.count();
//
//        if (elapsed - p.start > 1s) {
//            particles.erase(particles.begin() + i);
//        }
//    }
//
//    UI ui;
//
//    Style style{};
//    style.anchor = { 1,0 };
//    ui.begin_group(style);
//    std::string score_text = std::to_string(score);
//    ui.rect(score_text.data());
//    ui.end_group();
//
//
//
//    BeginDrawing();
//
//    ClearBackground(BLANK);
//    DrawRectangle(0, 0, 1280, 960, BLACK);
//
//    float x = elapsed.count();
//    Vec2 p1 = cam.world_to_screen({ x, 0.5f });
//    Vec2 p2 = cam.world_to_screen({ x, -0.5f });
//    DrawLine(p1.x, p1.y, p2.x, p2.y, YELLOW);
//
//    ui.draw();
//
//    Vector2 drum_pos = { 0, (window_height - textures.inner_drum.height) / 2};
//    Vector2 right_pos = drum_pos;
//    right_pos.x += textures.inner_drum.width;
//    Rectangle rect{ 0, 0, textures.inner_drum.width, textures.inner_drum.height };
//    Rectangle flipped_rect = rect;
//    flipped_rect.width *= -1;
//
//    for (int i = inputs.size() - 1; i >= 0; i--) {
//        const InputRecord& input = inputs[i];
//        if (elapsed.count() - input.time > input_indicator_duration) {
//            break;
//        }
//
//        switch (input.type) {
//        case Input::don_left:
//            DrawTextureRec(textures.inner_drum, rect, drum_pos, WHITE);
//            break;
//        case Input::don_right:
//            DrawTextureRec(textures.inner_drum, flipped_rect, right_pos, WHITE);
//            break;
//        case Input::kat_left:
//            DrawTextureRec(textures.outer_drum, flipped_rect, drum_pos, WHITE);
//            break;
//        case Input::kat_right:
//            DrawTextureRec(textures.outer_drum, rect, right_pos, WHITE);
//            break;
//        }
//    }
//
//
//    Vec2 target = cam.world_to_screen(cam.position);
//    DrawCircle(target.x, target.y, 50, WHITE);
//    DrawText((std::to_string(delta_time.count() * 1000) + " ms").data(), 100, 100, 24, WHITE);
//    draw_map(map, cam, current_note);
//
//    draw_particles(cam, particles, elapsed);
//
//    EndDrawing();
//
//}

const Vec2 note_hitbox = { 0.25, 0.25 };

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

SDL_FPoint vec2_to_sdl_fpoint(const Vec2& vec) {
    return { vec.x, vec.y };
}

void draw_wire_box(SDL_Renderer* renderer, const Vec2& pos, const Vec2& scale) {
    SDL_FPoint points[5];
    points[0] = vec2_to_sdl_fpoint(pos);
    points[1] = vec2_to_sdl_fpoint(pos + Vec2{1, 0} *scale);
    points[2] = vec2_to_sdl_fpoint(pos + scale);
    points[3] = vec2_to_sdl_fpoint(pos + Vec2{0, 1} *scale);
    points[4] = points[0];
    SDL_RenderLines(renderer, points, 5);
}

struct Image {
    SDL_Texture* texture;
    int w, h;
};

struct Color {
    uint8_t r, g, b, a;
};

Image kat_circle;
Image don_circle;
Image circle_overlay;
Image select_circle;

Image load_asset(SDL_Renderer* renderer, const char* path) {
    SDL_Surface* surface = IMG_Load(path);

    Image image;
    image.w = surface->w;
    image.h = surface->h;
    image.texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_DestroySurface(surface);

    return image;
}

Image load_asset_tint(SDL_Renderer* renderer, const char* path, const Color& color) {
    SDL_Surface* surface = IMG_Load(path);

    SDL_SetSurfaceColorMod(surface, color.r, color.g, color.b);

    Image image;
    image.w = surface->w;
    image.h = surface->h;
    image.texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_DestroySurface(surface);

    return image;
}

//struct Assets {
//    std::unordered_map<const char*, Image> stuff;
//};

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
        Image& circle_image = (map.flags_vec[i] & NoteFlagBits::don_or_kat) ? don_circle : kat_circle;

        Vec2 circle_pos = center_pos;
        circle_pos.x -= circle_image.w / 2 * scale;
        circle_pos.y -= circle_image.h / 2 * scale;

        {
            SDL_FRect rect = { circle_pos.x, circle_pos.y, circle_image.w * scale, circle_image.h * scale };
            SDL_RenderTexture(renderer, circle_image.texture, NULL, &rect);
            SDL_RenderTexture(renderer, circle_overlay.texture, NULL, &rect);
        }

        Vec2 hitbox_bounds = cam.world_to_screen_scale(note_hitbox);
        Vec2 hitbox_pos = center_pos - (hitbox_bounds / 2.0f);
        //draw_wire_box(hitbox_pos, hitbox_bounds, RED);

        if (map.selected[i]) {
            SDL_FRect rect = {
                center_pos.x - select_circle.w / 2 * scale,
                center_pos.y - select_circle.h / 2 * scale,
                select_circle.w * scale,
                select_circle.h * scale,
            };
            SDL_RenderTexture(renderer, select_circle.texture, NULL, &rect);
        }
    }
}

void set_draw_color(SDL_Renderer* renderer, Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

enum class EditorMode {
    select,
    insert,
};

class Editor {
public:
    Editor(const Input& _input, Audio& _audio, SDL_Renderer* _renderer);
    ~Editor();
    void update(std::chrono::duration<double> delta_time);
    void init();
private:
    const Input& input;
    Audio& audio;
    SDL_Renderer* renderer;

    Cam cam = {{0,0}, {2,1.5f}};

    EditorMode editor_mode = EditorMode::select;
    std::vector<int> selected;

    std::optional<Vec2> box_select_begin;

    NoteFlags insert_flags = NoteFlagBits::don_or_kat | NoteFlagBits::normal_or_big;
    //NoteType note_type;


    Map map{};
    //std::vector<Note> map;
    double offset = 4.390f;
    float bpm = 190;

    double quarter_interval = 60 / bpm / 4;
    double collision_range = quarter_interval / 2;

    bool paused = true;

    int current_note = -1;

    UI ui;

    //Mix_Music* music;
    Mix_Chunk* don_sound = Mix_LoadWAV("data/don.wav");
    Mix_Chunk* kat_sound = Mix_LoadWAV("data/kat.wav");
    Mix_Chunk* big_don_sound = Mix_LoadWAV("data/big-don.wav");
    Mix_Chunk* big_kat_sound = Mix_LoadWAV("data/big-kat.wav");
};

Editor::Editor(const Input& _input, Audio& _audio, SDL_Renderer* _renderer)
    : input{ _input }, audio{ _audio }, renderer{ _renderer }, ui{ input, renderer, window_width, window_height } {}

Editor::~Editor() {
    //Mix_FreeMusic(music);
    //Mix_FreeChunk(don_sound);
    //Mix_FreeChunk(kat_sound);
}

auto last = std::chrono::high_resolution_clock::now();


void Editor::init() {
    //music = Mix_LoadMUS("data/audio.mp3");

    //if (music == NULL) {
    //    std::cout << SDL_GetError() << '\n';
    //}

    //Mix_PlayMusic(music, 0);

    //auto callback = [&](void* udata, Uint8* stream, int len) {
    //    std::cout << std::format("{}\n", len);

    //    };
//typedef void (SDLCALL *Mix_MixCallback)(void *udata, Uint8 *stream, int len);

    //Mix_SetPostMix(callback, NULL);

    //Mix_VolumeMusic(MIX_MAX_VOLUME / 4);
    //Mix_PauseMusic();
}

void Editor::update(std::chrono::duration<double> delta_time) {
    ZoneScoped;
    //double elapsed = Mix_GetMusicPosition(music);
    double elapsed = audio.get_position();


    //if (!paused) {
    //    elapsed = Mix_GetMusicPosition(music);
    //} else {
    //    elapsed = cam.position.x;
    //}

    ui.begin_group(Style{ {0,0.5} }); {
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

        ui.button(editor_mode_text, [&]() {
            if (editor_mode == EditorMode::insert) {
                editor_mode = EditorMode::select;
            }
            else {
                editor_mode = EditorMode::insert;
            }
        });

        ui.button(note_size_text, [&]() {
            insert_flags = insert_flags ^ NoteFlagBits::normal_or_big;
        });

        ui.button(note_color_text, [&]() {
            insert_flags = insert_flags ^ NoteFlagBits::don_or_kat;
        });
    }
    ui.end_group();

    ui.begin_group(Style{ {0,1} });
        std::string time = std::to_string(cam.position.x) + " s";
        ui.rect(time.data());
    //ui.slider(elapsed / GetMusicTimeLength(music), [&](float fraction) {
    //    SeekMusicStream(music, fraction * GetMusicTimeLength(music));
    //});
    ui.end_group();

    ui.begin_group(Style{ {1,1} }); 
        auto frame_time = std::to_string(((float)std::chrono::duration_cast<std::chrono::microseconds>(delta_time).count()) / 1000) + " ms";
        ui.rect(frame_time.data());
    ui.end_group();

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
        save_map(map);
    }

    if (input.key_down(SDL_SCANCODE_O) && input.modifier(SDL_KMOD_LCTRL)) {
        load_map(&map);
    }

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


    //if (IsKeyPressed(KEY_F5)) {
    //    Game game{};
    //    game.run();
    //}

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
            Mix_PlayChannel(-1, don_sound, 0);
            break;
        case (0 | NoteFlagBits::normal_or_big):
            Mix_PlayChannel(-1, kat_sound, 0);
            break;
        case (NoteFlagBits::don_or_kat | 0):
            Mix_PlayChannel(-1, big_don_sound, 0);
            Mix_PlayChannel(-1, don_sound, 0);
            break;
        case (0 | 0):
            Mix_PlayChannel(-1, big_kat_sound, 0);
            Mix_PlayChannel(-1, kat_sound, 0);
            break;
        }

        current_note++;
    }


    const float& wheel = input.wheel;
    if (wheel != 0) {
        int i = std::round((elapsed - offset) / quarter_interval);
        double seek = offset + (i - wheel) * quarter_interval;
        audio.set_position(seek);
        elapsed = audio.get_position();
    }
    cam.position.x = elapsed;

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
//
//class MainMenu {
//public:
//    void update(std::function<void()> callback);
//
//private:
//    UI ui;
//    View current_view = View::main;
//};
//
//void MainMenu::update(std::function<void()> callback) {
//    if (IsKeyPressed(KEY_ONE)) {
//        callback();
//        std::cout << "fskadfh\n";
//        return;
//    }
//
//    ui.input();
//
//
//    switch (current_view) {
//        case View::main:
//            ui.begin_group({});
//            ui.button("Play", [&]() {
//                current_view = View::map_select;
//                std::cout << "kfsahdfhj\n";
//            });
//            ui.rect("Settings");
//            ui.rect("Exit");
//
//            ui.end_group();
//            break;
//        case View::map_select:
//            ui.rect("askjldhf");
//            if (IsKeyPressed(KEY_ESCAPE)) {
//                current_view = View::main;
//            }
//            break;
//    }
//
//    BeginDrawing();
//    ClearBackground(BLACK);
//    DrawText("editor", 400, 300, 24, WHITE);
//
//    ui.draw();
//    EndDrawing();
//}


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


    //int length;
    //if (TTF_MeasureUTF8(font, text, 9999, &length, NULL) < 0) {
    //    std::cout << std::format("{}\n", SDL_GetError());
    //    return 1;
    //}

    //MainMenu menu;

    //
    //Game game;

    kat_circle = load_asset_tint(renderer, "data/circle.png", { 60, 219, 226, 255 });
    don_circle = load_asset_tint(renderer, "data/circle.png", { 252, 78, 60, 255 });
    circle_overlay = load_asset(renderer, "data/circle-overlay.png");
    select_circle = load_asset(renderer, "data/circle-select.png");

    SDL_Surface* circle_surface = IMG_Load("data/circle.png");

    SDL_SetSurfaceColorMod(circle_surface, 255, 0, 0);

    SDL_Texture* don_circle = SDL_CreateTextureFromSurface(renderer, circle_surface);

    SDL_SetSurfaceColorMod(circle_surface, 0, 0, 255);
    
    SDL_Texture* kat_circle = SDL_CreateTextureFromSurface(renderer, circle_surface);

    Audio audio{};
    

    Context context = Context::Editor;

    auto to_editor = [&]() {
        context = Context::Editor;
    };

    Input input{};

    Editor editor{input, audio, renderer};
    editor.init();

    Mix_MasterVolume(MIX_MAX_VOLUME * 0.25);
    Mix_VolumeMusic(MIX_MAX_VOLUME * 0.1);

    auto last_frame = std::chrono::high_resolution_clock::now();

    Player player{ input, audio };

    init_font(renderer);

    //auto future = std::async(std::launch::async, fake_loop);

    while (1) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> delta_time = now - last_frame;

        if (delta_time < 1ms) {
            continue;
        }

        ZoneNamedN(var, "update", true);

        last_frame = now;

        float wheel{};
        bool moved{};

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
                    wheel += event.wheel.y;
                }

                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                    int asdf = 0;
                }
                
                if (event.type == SDL_EVENT_KEY_DOWN) {
                }
            }
        }

        if (quit) {
            break;
        }

        input.begin_frame(wheel);

        //input.mouse_down(SDL_BUTTON_LEFT);

        
        //SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        //SDL_RenderClear(renderer);


        //SDL_RenderPresent(renderer);
        //player.update(delta_time.count());

        editor.update(delta_time);

        input.end_frame();

        FrameMark;
    }


    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    
    //while (!WindowShouldClose()) {
    //    auto now = high_resolution_clock::now();
    //    duration<double> delta_time = now - last_frame;

    //    switch (context) {
    //    case Context::Menu:
    //        menu.update(to_editor);
    //        break;
    //    case Context::Editor:
    //        editor.update(delta_time);
    //        break;
    //    case Context::Game:
    //        game.update(delta_time);
    //        break;
    //    }

    //    last_frame = now;
    //}
    //
    //CloseWindow();
}

int main() {
    return run();
}
