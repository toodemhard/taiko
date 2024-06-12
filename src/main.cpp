#include <algorithm>
#include <format>
#include <iostream>
#include <string>
#include <codecvt>
#include <chrono>
#include <filesystem>
#include "raylib.h"

using namespace std::chrono_literals;

const int window_width = 800;
const int window_height = 600;

const std::chrono::duration<double> hit_range = 300ms;

enum class NoteType {
    kat,
    don,
};

struct Note {
    std::chrono::duration<double> time;
    NoteType type;
};

struct Vec2 {
    float x;
    float y;

    Vec2 operator+(const Vec2& other) {
        return Vec2{x + other.x, y + other.y};
    }

    Vec2 operator+(const float& other) {
        return Vec2{x + other, y + other};
    }
    
    Vec2 operator-(const Vec2& other) {
        return Vec2{x - other.x, y - other.y};
    }

    Vec2 operator*(const float& other) {
        return Vec2{x * other, y * other};
    }

    Vec2 operator/(const float& other) {
        return Vec2{x / other, y / other};
    }

    Vec2 operator/(const Vec2& other) {
        return Vec2{x / other.x, y / other.y};
    }

    Vec2 operator*(const Vec2& other) {
        return Vec2{x * other.x, y * other.y};
    }
};

class Cam {
public:
    Vec2 m_Position;
    Vec2 m_Bounds;
    
    Vec2 WorldToScreen(Vec2 pos) const;
    float world_to_screen(float length) const;
    Vec2 screen_to_world(Vec2 pos) const;
};

Vec2 Cam::WorldToScreen(Vec2 pos) const {
    Vec2 temp = {pos.x - m_Position.x, m_Position.y - pos.y};
    Vec2 normalized = temp / m_Bounds + 0.5;
        
    return Vec2 {
        normalized.x * (float)GetScreenWidth(),
        normalized.y * (float)GetScreenHeight()
    };
}

float Cam::world_to_screen(float length) const {
    return length / m_Bounds.y * GetScreenHeight();

}

Vec2 Cam::screen_to_world(Vec2 pos) const {
    Vec2 normalized = {pos.x / GetScreenWidth(), pos.y / GetScreenHeight()};
    Vec2 offset = (normalized - Vec2{0.5, 0.5}) * m_Bounds;
    return {offset.x + m_Position.x, m_Position.y - offset.y};
}


class GameState {
public:
    GameState();
    void Update(std::chrono::duration<double> delta_time);
private:
    Sound don_sound = LoadSound("don.wav");
    Sound kat_sound = LoadSound("kat.wav");
    
    Cam cam = {{0,0}, {4,3}};

    std::vector<Vec2> circles;
    std::vector<Color> colors;

    std::vector<int> stuff {0,1,2,3,4,5};

    std::vector<Note> map{};
    int current_note = 0;

    std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::high_resolution_clock::now();
};

GameState::GameState() {
    for (int i = 0; i < 100; i++) {
        map.push_back({
            0.0681s * i,
            NoteType::don
        });
    }

    for (int i = 0; i < map.size(); i++) {
        auto& note = map[i];
        Vec2 pos = {1 * (float)note.time.count(), 0};
        circles.push_back(pos);


        if (note.type == NoteType::don) {
            colors.push_back(RED);
        } else {
            colors.push_back(BLUE);
        }

    }
}

void GameState::Update(std::chrono::duration<double> delta_time) {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = now - start;

    std::vector<NoteType> pressed;

    if (IsKeyPressed(KEY_X)) {
        PlaySound(don_sound);

        pressed.push_back(NoteType::don);
    }

    if (IsKeyPressed(KEY_PERIOD)) {
        PlaySound(don_sound);

        pressed.push_back(NoteType::don);
    }

    if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SLASH)) {
        PlaySound(kat_sound);
    }

    for (auto& note : pressed) {
        if ((elapsed - map[current_note].time) / hit_range + 0.5 < 1) {
            if (map[current_note].type == note) {
                current_note++;
            }
        }

    }

    if ((elapsed > map[current_note].time)) {
        PlaySound(don_sound);
        current_note++;
    }
    
    cam.m_Position.x += 1 * delta_time.count();

    for (int i = current_note; i < circles.size(); i++) {
        Vec2 screen_pos = cam.WorldToScreen(circles[i]);
        DrawCircle(screen_pos.x, screen_pos.y, 20, colors[i]);
    }
    

    BeginDrawing();
    
    ClearBackground(BLACK);
    DrawText((std::to_string(((float) std::chrono::duration_cast<std::chrono::microseconds>(delta_time).count()) / 1000) + "ms").data(), 100, 100, 24, WHITE);
    EndDrawing();

}

void render_map(const std::vector<Note>& map, const Cam& cam, int current_note) {
    if (map.size() == 0) {
        return;
    }

    int right = map.size() - 1;
    int left = 0;
    constexpr float circle_padding = 0.2f;
    float right_bound = cam.m_Position.x + cam.m_Bounds.x / 2 + circle_padding;
    float left_bound = cam.m_Position.x - (cam.m_Bounds.x / 2 + circle_padding);

    for (int i = current_note; i < map.size(); i++) {
        if (map[i].time.count() >= right_bound) {
            right = i - 1;
            break;
        }
    }
    for (int i = current_note; i >= 0; i--) {
        if (i >= map.size()) {
            continue;
        }
        if (map[i].time.count() <= left_bound) {
            left = i + 1;
            break;
        }
    }

    for(int i = right; i >= left; i--) {
        const Note& note = map[i];
        Vec2 circle_pos = cam.WorldToScreen({(float)note.time.count(), 0});
        Color color = (note.type == NoteType::don) ? RED : BLUE;

        DrawCircle(circle_pos.x, circle_pos.y, cam.world_to_screen(0.11), WHITE);
        DrawCircle(circle_pos.x, circle_pos.y, cam.world_to_screen(0.1), color);
    }
}

enum class EditorMode {
    select,
    insert,
};

class App {
public:
    void Update(std::chrono::duration<double> delta_time);
    void Init();
private:
    Cam cam = {{0,0}, {2,3}};

    EditorMode mode;
    std::vector<int> selected;
    NoteType note_type;

    std::vector<Note> map;
    std::chrono::duration<double> offset;
    float bpm = 253;

    double quarter_interval = 60 / bpm / 4;
    double collision_range = quarter_interval / 2;

    bool paused = true;

    int current_note = -1;

    Music music;

    Sound don_sound = LoadSound("don.wav");
    Sound kat_sound = LoadSound("kat.wav");
};

void App::Init() {
    music = LoadMusicStream("kinoko.mp3");
    SetMusicVolume(music, 0.2f);

    PlayMusicStream(music);
    offset = std::chrono::duration<double>(0.994);

    PauseMusicStream(music);

    SeekMusicStream(music, 0.0f);
}

void add_note(std::vector<Note>& map, Note note) {
    auto cmp = [](Note l, Note r) { return l.time < r.time; };
    map.insert(std::upper_bound(map.begin(), map.end(), note, cmp), note);
}

void App::Update(std::chrono::duration<double> delta_time) {
    UpdateMusicStream(music);
    BeginDrawing();
    
    ClearBackground(BLACK);

    float elapsed = GetMusicTimePlayed(music);

    if (IsMouseButtonPressed(0)) {
        Vec2 cursor_pos = cam.screen_to_world({(float)GetMouseX(), (float)GetMouseY()});

        int i = std::round((cursor_pos.x - offset.count()) / quarter_interval);
        auto time = std::chrono::duration<double>(offset.count() + i * quarter_interval);

        bool collision = false;
        double diff;
        for (int i = 0; i < map.size(); i++) {
            diff = std::abs((map[i].time - time).count());
            if (diff < collision_range) {
                collision = true;
                break;
            }
        }

        if (!collision) {
            add_note(map, Note{time, note_type});

            if (time.count() < elapsed) {
                current_note++;
            }
        }
    }

    if (IsKeyPressed(KEY_R)) {
        if (note_type == NoteType::don) {
            note_type = NoteType::kat;
        } else {
            note_type = NoteType::don;
        }
    }

    for (int i = 0; i < 500; i++) {
        float x = offset.count() + i * quarter_interval;

        float height;
        Color color;
        if (i % 4 == 0) {
            height = 0.2;
            color = WHITE;
        } else {
            height = 0.1;
            color = RED;
        }
        
        Vec2 p1 = cam.WorldToScreen({x, 0});
        Vec2 p2 = cam.WorldToScreen({x, height});

        DrawLine(p1.x, p1.y, p2.x, p2.y, color);
    }

    Vec2 p1 = cam.WorldToScreen(cam.m_Position);
    Vec2 p2 = cam.WorldToScreen(cam.m_Position + Vec2{0,0.6});

    DrawLine(p1.x, p1.y, p2.x, p2.y, YELLOW);


    if (IsKeyPressed(KEY_SPACE)) {
        if (paused) {
            ResumeMusicStream(music);
            auto cmp = [](float current_time, Note& note) { return current_time < note.time.count(); };
            current_note = std::upper_bound(map.begin(), map.end(), elapsed, cmp) - map.begin();
            std::cout << current_note << '\n';
        } else {
            PauseMusicStream(music);
        }

        paused = !paused;
    }

    if (!paused && current_note < map.size() && elapsed >= map[current_note].time.count()) {
        if (map[current_note].type == NoteType::don) {
            PlaySound(don_sound);
        } else {
            PlaySound(kat_sound);
        }

        current_note++;
    }

    if (IsKeyPressed(KEY_A) && GetMusicTimePlayed(music) != 0.0f) {
        SeekMusicStream(music, 1.0f);
        std::cout << GetMusicTimePlayed(music) << '\n';
    }

    cam.m_Position.x = elapsed;

    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        float seek = std::clamp(elapsed - wheel * 0.4f, 0.0f, GetMusicTimeLength(music));
        SeekMusicStream(music, seek);
        elapsed = GetMusicTimePlayed(music);

        // if (map.size() > 0 && elapsed < map[current_note].time.count()) {
        //     for (int i = current_note - 1; i >= 0; i--) {
        //         if (elapsed > map[i].time.count()) {
        //             current_note = i + 1;
        //             break;
        //         }
        //     }
        // } else {

        // }
        // auto cmp = [](float current_time, Note& note) { return current_time < note.time.count(); };
        // current_note = std::upper_bound(map.begin(), map.end(), elapsed, cmp) - map.begin();
        std::cout << current_note << '\n';
    }

    render_map(map, cam, current_note);


    DrawText(std::to_string(cam.m_Position.x).data(), 0, 0, 24, WHITE);
    DrawText((std::to_string(((float) std::chrono::duration_cast<std::chrono::microseconds>(delta_time).count()) / 1000) + "ms").data(), 100, 100, 24, WHITE);
    EndDrawing();
}

void run() {
    InitWindow(1280, 960, "taiko");
    
    auto last_frame = std::chrono::high_resolution_clock::now();
    
    InitAudioDevice();
    
    float pos = 400;
    auto start = std::chrono::high_resolution_clock::now();

    GameState game_state;

    App app;

    app.Init();

    using namespace std::chrono;
    
    while (!WindowShouldClose()) {
        auto now = high_resolution_clock::now();
        duration<double> delta_time = now - last_frame;
        app.Update(delta_time);


        last_frame = now;
    }
    
    CloseWindow();
}

int main() {
    run();
}