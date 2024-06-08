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
    Vec2 screen_to_world(Vec2 pos) const;
};

Vec2 Cam::WorldToScreen(Vec2 pos) const {
    Vec2 temp = {pos.x - m_Position.x, m_Position.y - pos.y};
    Vec2 normalized = temp / m_Bounds + 0.5;
        
    return Vec2 {
        normalized.x * GetScreenWidth(),
        normalized.y * GetScreenHeight()
    };
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
        std::cout << (elapsed - map[current_note].time).count() << "\n";

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

void render_map(const std::vector<Note>& map, const Cam& cam) {
    for(int i = 0; i < map.size(); i++) {
        const Note& note = map[i];
        Vec2 circle_pos = cam.WorldToScreen({(float)note.time.count(), 0});
        Color color = (note.type == NoteType::don) ? RED : BLUE;

        DrawCircle(circle_pos.x, circle_pos.y, 20, color);
    }
}

class Editor {
public:
    void Update(std::chrono::duration<double> delta_time);
    void Init();
private:
    Cam cam = {{0,0}, {2,3}};

    std::vector<int> selected;

    std::vector<Note> map;
    std::chrono::duration<double> offset;
    float bpm = 253;

    bool paused = true;

    Music music;
};

void Editor::Init() {
    music = LoadMusicStream("kinoko.mp3");
    SetMusicVolume(music, 0.2f);

    PlayMusicStream(music);
    offset = std::chrono::duration<double>(0.994);

    PauseMusicStream(music);

    SeekMusicStream(music, 0.0f);

    map.push_back({std::chrono::duration<double>(1), NoteType::don});
    selected.push_back(0);
}

void Editor::Update(std::chrono::duration<double> delta_time) {
    UpdateMusicStream(music);
    BeginDrawing();
    
    ClearBackground(BLACK);

    if (IsMouseButtonPressed(0)) {
        Vec2 cursor_pos = cam.screen_to_world({(float)GetMouseX(), (float)GetMouseY()});
        std::cout << std::format("{} {}\n", cursor_pos.x, cursor_pos.y);

        map.push_back({
            std::chrono::duration<double>(cursor_pos.x),
            NoteType::don
        });
    }

    for (int i = 0; i < 500; i++) {
        float x = offset.count() + i * 60 / bpm / 4;

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
        } else {
            PauseMusicStream(music);
        }

        paused = !paused;
    }

    if (IsKeyPressed(KEY_A) && GetMusicTimePlayed(music) != 0.0f) {
        SeekMusicStream(music, 1.0f);
        std::cout << GetMusicTimePlayed(music) << '\n';
    }

    float played = GetMusicTimePlayed(music);
    cam.m_Position.x = played;

    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        float seek = std::clamp(played - wheel * 0.4f, 0.0f, GetMusicTimeLength(music));
        // std::cout << GetMusicTimeLength(music) << '\n';
        SeekMusicStream(music, seek);
        std::cout << seek << '\t' << GetMusicTimePlayed(music) << '\n';
    }

    for (int& i : selected) {
        Vec2 circle_pos = cam.WorldToScreen({(float)map[i].time.count(), 0});
        DrawCircle(circle_pos.x, circle_pos.y, 21, WHITE);
    }

    render_map(map, cam);


    DrawText(std::to_string(cam.m_Position.x).data(), 0, 0, 24, WHITE);
    DrawText((std::to_string(((float) std::chrono::duration_cast<std::chrono::microseconds>(delta_time).count()) / 1000) + "ms").data(), 100, 100, 24, WHITE);
    EndDrawing();
}


int main() {
    InitWindow(800, 600, "taiko");
    
    auto last_frame = std::chrono::high_resolution_clock::now();
    
    InitAudioDevice();
    
    
    // for (const auto& entry : std::filesystem::directory_iterator(".")) {
    //     auto file_name = entry.path().filename();
    //     std::cout << file_name << "\n";
    // }
    
    // Music music = LoadMusicStream("mesmerizer.mp3");
    // PlayMusicStream(music);




    float pos = 400;



    auto start = std::chrono::high_resolution_clock::now();

    GameState game_state;

    Editor editor;

    editor.Init();

    using namespace std::chrono;
    
    while (!WindowShouldClose()) {
        auto now = high_resolution_clock::now();
        duration<double> delta_time = now - last_frame;
        editor.Update(delta_time);


        last_frame = now;
    }
    
    CloseWindow();
}
