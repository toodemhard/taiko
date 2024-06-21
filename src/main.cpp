#include <algorithm>
#include <functional>
#include <format>
#include <iostream>
#include <string>
#include <codecvt>
#include <chrono>
#include <filesystem>

#include "vec.h"
#include "ui.h"
#include "raylib.h"

using namespace std::chrono_literals;

const int window_width = 1280;
const int window_height = 960;

const std::chrono::duration<double> hit_range = 100ms;
const std::chrono::duration<double> perfect_range = 30ms;

const float circle_radius = 0.1f;
const float circle_outer_radius = 0.11f;

struct Textures {
	Texture2D circle;
	Texture2D circle_overlay;
	Texture2D big_circle;
	Texture2D big_circle_overlay;

    Texture2D select_circle;

	Texture2D inner_drum;
	Texture2D outer_drum;
};

Textures textures;


enum NoteFlagBits : uint8_t {
    don_or_kat = 1 << 0, // true = don | false = kat
    normal_or_big = 1 << 1,
};

using NoteFlags = uint8_t;

enum class NoteType : uint8_t {
    don = 0,
    kat = 1,
};

struct Note {
    std::chrono::duration<double> time;
    NoteType type;
};

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
        normalized.x * (float)GetScreenWidth(),
        normalized.y * (float)GetScreenHeight()
    };
}
Vec2 Cam::world_to_screen_scale(const Vec2& scale) const {
    return { world_to_screen_scale(scale.x), world_to_screen_scale(scale.y) };
}

float Cam::world_to_screen_scale(const float& length) const {
    return length / bounds.y * GetScreenHeight();

}

Vec2 Cam::screen_to_world(const Vec2& point) const {
    Vec2 normalized = { point.x / GetScreenWidth(), point.y / GetScreenHeight() };
    Vec2 relative_pos = (normalized - Vec2::one() * 0.5f) * bounds;
    return { relative_pos.x + position.x, position.y - relative_pos.y };
}

void draw_map(const std::vector<Note>& map, const Cam& cam, int current_note) {
    if (map.size() == 0) {
        return;
    }

    int right = map.size() - 1;
    constexpr float circle_padding = 0.2f;
    float right_bound = cam.position.x + cam.bounds.x / 2 + circle_padding;

    for (int i = current_note; i < map.size(); i++) {
        if (map[i].time.count() >= right_bound) {
            right = i - 1;
            break;
        }
    }

    for(int i = right; i >= current_note; i--) {
        const Note& note = map[i];
        Vec2 circle_pos = cam.world_to_screen({(float)note.time.count(), 0});
        Color color = (note.type == NoteType::don) ? RED : BLUE;

        DrawCircle(circle_pos.x, circle_pos.y, cam.world_to_screen_scale(circle_outer_radius), WHITE);
        DrawCircle(circle_pos.x, circle_pos.y, cam.world_to_screen_scale(circle_radius), color);
    }
}

const float particle_duration = 1.0f;

struct Particle {
    Vec2 position;
    Vec2 velocity;
    float scale;
    NoteType type;
    std::chrono::duration<double> start;
};

void draw_particles(const Cam& cam, const std::vector<Particle>& particles, std::chrono::duration<double> now) {
    float outer_radius = cam.world_to_screen_scale(circle_outer_radius);
    float inner_radius = cam.world_to_screen_scale(circle_radius);
    for (auto& p : particles) {
        Vec2 pos = cam.world_to_screen(p.position);

        Color color;

        if (p.type == NoteType::don) {
            color = RED;
        } else {
            color = BLUE;
        }

        uint8_t alpha = (p.start - now).count() / particle_duration * 255;
        color.a = alpha;

        //DrawCircle(pos.x, pos.y, outer_radius, Color{255,255,255,alpha});
        DrawCircle(pos.x, pos.y, inner_radius, color);
    }
}

enum class Input {
    don_left,
    don_right,
    kat_left,
    kat_right,
};

struct InputRecord {
    Input type;
    float time;
};

class Map {
public:
    void insert_note(float time, NoteFlags flags);
    
    std::vector<float> times;
    std::vector<NoteFlags> flags_vec;
    std::vector<bool> selected;
};

void Map::insert_note(float time, NoteFlags flags) {
    int i = std::upper_bound(times.begin(), times.end(), time) - times.begin();
    times.insert(times.begin() + i, time);
    flags_vec.insert(flags_vec.begin() + i, flags);
    selected.insert(selected.begin() + i, false);
}



class Game {
public:
    Game();
    void run();
    void update(std::chrono::duration<double> delta_time);
private:
    Sound don_sound = LoadSound("don.wav");
    Sound kat_sound = LoadSound("kat.wav");
    
    Cam cam = {{0,0}, {4,3}};

    std::vector<Note> map{};
    int current_note = 0;

    std::vector<Particle> particles;

    int score = 0;

    std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::high_resolution_clock::now();

    std::vector<InputRecord> inputs;
};

Game::Game() {
    for (int i = 0; i < 100; i++) {
        map.push_back({
            (0.2371s / 2) * i + 0.4743s,
            NoteType::don
        });
    }
}

void Game::run() {

}

const float input_indicator_duration = 0.1f;

void Game::update(std::chrono::duration<double> delta_time) {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = now - start;

    //bool inputs[4] = { false, false, false, false };
    //std::array<bool, 4> inputs = { false, false, false, false };
    std::vector<NoteType> pressed;


    if (IsKeyPressed(KEY_X)) {
        PlaySound(don_sound);
        inputs.push_back(InputRecord{ Input::don_left, (float) elapsed.count() });

        pressed.push_back(NoteType::don);
    }

    if (IsKeyPressed(KEY_PERIOD)) {
        PlaySound(don_sound);
        inputs.push_back(InputRecord{ Input::don_right, (float)elapsed.count() });

        pressed.push_back(NoteType::don);
    }

    if (IsKeyPressed(KEY_Z)) {
        inputs.push_back(InputRecord{ Input::kat_left, (float)elapsed.count() });
        PlaySound(kat_sound);

        pressed.push_back(NoteType::kat);
    }

    if (IsKeyPressed(KEY_SLASH)) {
        inputs.push_back(InputRecord{ Input::kat_right, (float)elapsed.count() });
        PlaySound(kat_sound);

        pressed.push_back(NoteType::kat);
    }

    if (current_note < map.size()) {
        for (auto& note : pressed) {
            double hit_normalized = (elapsed - map[current_note].time) / hit_range + 0.5;
            if (hit_normalized >= 0 && hit_normalized < 1) {
                if (map[current_note].type == note) {
                    score += 300;
                    //particles.push_back(Particle{ Vec2{(float)elapsed.count(), 0}, {0,1},1, note.type, elapsed });
                    current_note++;
                }
            }

        }

        if (elapsed > map[current_note].time - (hit_range / 2)) {
            //PlaySound(don_sound);
            particles.push_back(Particle{ Vec2{(float)elapsed.count(), 0}, {0,1}, 1, map[current_note].type, elapsed });
            current_note++;
        }
    }

    cam.position.x = elapsed.count();

    for (int i = particles.size() - 1; i >= 0; i--) {
        Particle& p = particles[i];
        p.position += p.velocity * delta_time.count();

        if (elapsed - p.start > 1s) {
            particles.erase(particles.begin() + i);
        }
    }

    UI ui;

    Style style{};
    style.anchor = { 1,0 };
    ui.begin_group(style);
    std::string score_text = std::to_string(score);
    ui.rect(score_text.data());
    ui.end_group();



    BeginDrawing();

    ClearBackground(BLANK);
    DrawRectangle(0, 0, 1280, 960, BLACK);

    float x = elapsed.count();
    Vec2 p1 = cam.world_to_screen({ x, 0.5f });
    Vec2 p2 = cam.world_to_screen({ x, -0.5f });
    DrawLine(p1.x, p1.y, p2.x, p2.y, YELLOW);

    ui.draw();

    Vector2 drum_pos = { 0, (window_height - textures.inner_drum.height) / 2};
    Vector2 right_pos = drum_pos;
    right_pos.x += textures.inner_drum.width;
	Rectangle rect{ 0, 0, textures.inner_drum.width, textures.inner_drum.height };
    Rectangle flipped_rect = rect;
    flipped_rect.width *= -1;

    for (int i = inputs.size() - 1; i >= 0; i--) {
        const InputRecord& input = inputs[i];
        if (elapsed.count() - input.time > input_indicator_duration) {
            break;
        }

        switch (input.type) {
        case Input::don_left:
            DrawTextureRec(textures.inner_drum, rect, drum_pos, WHITE);
            break;
        case Input::don_right:
            DrawTextureRec(textures.inner_drum, flipped_rect, right_pos, WHITE);
            break;
        case Input::kat_left:
			DrawTextureRec(textures.outer_drum, flipped_rect, drum_pos, WHITE);
            break;
        case Input::kat_right:
			DrawTextureRec(textures.outer_drum, rect, right_pos, WHITE);
            break;
        }
    }


    Vec2 target = cam.world_to_screen(cam.position);
    DrawCircle(target.x, target.y, 50, WHITE);
    DrawText((std::to_string(delta_time.count() * 1000) + " ms").data(), 100, 100, 24, WHITE);
    draw_map(map, cam, current_note);

    draw_particles(cam, particles, elapsed);

    EndDrawing();

}

const Vec2 note_hitbox = { 0.5, 0.5 };

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
        Vec2 center = { map.times[i], 0 };
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

void draw_wire_box(const Vec2& pos, const Vec2& scale, const Color& color) {
    Vec2 p2 = pos + Vec2{ 1, 0 } * scale;
    Vec2 p3 = pos + scale;
    Vec2 p4 = pos + Vec2{ 0, 1 } * scale;
    DrawLine(pos.x, pos.y, p2.x, p2.y, color);
    DrawLine(p2.x, p2.y, p3.x, p3.y, color);
    DrawLine(p3.x, p3.y, p4.x, p4.y, color);
    DrawLine(p4.x, p4.y, pos.x, pos.y, color);
}

void draw_map_editor(const Map& map, const Cam& cam, int current_note) {
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
        Vec2 center_pos = cam.world_to_screen({map.times[i], 0});
        float scale = (map.flags_vec[i] & NoteFlagBits::normal_or_big) ? 0.9f : 1.4f;
        Vec2 circle_pos = center_pos;
        circle_pos.x -= textures.circle.width / 2 * scale;
        circle_pos.y -= textures.circle.height / 2 * scale;
        Color color = (map.flags_vec[i] & NoteFlagBits::don_or_kat) ? RED : BLUE;

        Vec2 select_pos = { center_pos.x - textures.select_circle.width / 2, center_pos.y - textures.select_circle.height / 2};

        DrawTextureEx(textures.circle, { circle_pos.x, circle_pos.y }, 0, scale, color);
        DrawTextureEx(textures.circle_overlay, { circle_pos.x, circle_pos.y }, 0, scale, WHITE);

        Vec2 hitbox_bounds = cam.world_to_screen_scale(note_hitbox);
        Vec2 hitbox_pos = center_pos - (hitbox_bounds / 2.0f);
        draw_wire_box(hitbox_pos, hitbox_bounds, RED);

        if (map.selected[i]) {
			DrawTextureEx(textures.select_circle, { select_pos.x, select_pos.y }, 0, 1, WHITE);
        }
    }
}

enum class EditorMode {
    select,
    insert,
};

class Editor {
public:
    void update(std::chrono::duration<double> delta_time);
    void init();
private:
    Cam cam = {{0,0}, {4,3}};

    EditorMode editor_mode = EditorMode::select;
    std::vector<int> selected;

    std::optional<Vec2> box_select_begin;

    NoteFlags insert_flags = NoteFlagBits::don_or_kat | NoteFlagBits::normal_or_big;
    //NoteType note_type;


    Map map;
    //std::vector<Note> map;
    std::chrono::duration<double> offset;
    float bpm = 60;

    double quarter_interval = 60 / bpm / 4;
    double collision_range = quarter_interval / 2;

    bool paused = true;

    int current_note = -1;

    UI ui;

    Music music;

    Sound don_sound = LoadSound("don.wav");
    Sound kat_sound = LoadSound("kat.wav");
};

void Editor::init() {
    music = LoadMusicStream("kinoko.mp3");
    SetMusicVolume(music, 0.2f);

    PlayMusicStream(music);
    offset = std::chrono::duration<double>(0);

    PauseMusicStream(music);
}

void add_note(std::vector<Note>& map, Note note) {
    auto cmp = [](Note l, Note r) { return l.time < r.time; };
    map.insert(std::upper_bound(map.begin(), map.end(), note, cmp), note);
}

void Editor::update(std::chrono::duration<double> delta_time) {
    UpdateMusicStream(music);

    float elapsed = GetMusicTimePlayed(music);

    ui.input();

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

    ui.begin_group(Style{ {0,1} }); {
		std::string time = std::to_string(cam.position.x) + " s";
		ui.rect(time.data());
		ui.slider(elapsed / GetMusicTimeLength(music), [&](float fraction) {
			SeekMusicStream(music, fraction * GetMusicTimeLength(music));
		});
    }
    ui.end_group();

    ui.begin_group(Style{ {1,1} });
    auto frame_time = std::to_string(((float)std::chrono::duration_cast<std::chrono::microseconds>(delta_time).count()) / 1000) + " ms";
    ui.rect(frame_time.data());
    ui.end_group();

	Vec2 cursor_pos = cam.screen_to_world({ (float)GetMouseX(), (float)GetMouseY() });

    if (IsKeyPressed(KEY_A)) {
        int asdf = 0;
    }

    switch (editor_mode) {
    case EditorMode::select:
        if (!ui.clicked && IsMouseButtonPressed(0)) {
            std::cout << cursor_pos.x << '\n';
            std::optional<int> to_select = note_point_intersection(map, cursor_pos, current_note);

            if (to_select.has_value()) {
                map.selected[to_select.value()] = !map.selected[to_select.value()];
            } else { 
                box_select_begin = cursor_pos;
            }
        }

        if (IsMouseButtonDown(0)) {
            if (box_select_begin.has_value()) {
                auto hits = note_box_intersection(map, box_select_begin.value(), cursor_pos);

                std::fill(map.selected.begin(), map.selected.end(), false);
				for (auto& i : hits) {
                    map.selected[i] = true;
                }
            }

        }

        if (IsMouseButtonReleased(0)) {
            box_select_begin = {};
        }

        break;
    case EditorMode::insert:
		if (!ui.clicked && IsMouseButtonPressed(0)) {
			Vec2 cursor_pos = cam.screen_to_world({(float)GetMouseX(), (float)GetMouseY()});

			int i = std::round((cursor_pos.x - offset.count()) / quarter_interval);
			float time = offset.count() + i * quarter_interval;

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


    if (IsKeyPressed(KEY_F5)) {
        Game game{};
        game.run();
    }

    if (IsKeyPressed(KEY_Q)) { 
        editor_mode = EditorMode::select;
    }

    if (IsKeyPressed(KEY_W)) { 
        editor_mode = EditorMode::insert;
    }

    if (IsKeyPressed(KEY_E)) {
        insert_flags = insert_flags ^ NoteFlagBits::normal_or_big;
    }

    if (IsKeyPressed(KEY_R)) {
        insert_flags = insert_flags ^ NoteFlagBits::don_or_kat;
    }

    if (IsKeyPressed(KEY_SPACE)) {
        if (paused) {
            ResumeMusicStream(music);
            current_note = std::upper_bound(map.times.begin(), map.times.end(), elapsed) - map.times.begin();
            std::cout << current_note << '\n';
        } else {
            PauseMusicStream(music);
        }

        paused = !paused;
    }

    if (!paused && current_note < map.times.size() && elapsed >= map.times[current_note]) {
        if (map.flags_vec[current_note] & NoteFlagBits::don_or_kat) {
            PlaySound(don_sound);
        } else {
            PlaySound(kat_sound);
        }

        current_note++;
    }


    cam.position.x = elapsed;

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
        //std::cout << current_note << '\n';
    }



    BeginDrawing();
    ClearBackground(BLACK);

	float right_bound = cam.position.x + cam.bounds.x / 2;
	float left_bound = cam.position.x - cam.bounds.x / 2;

    int start = (left_bound - offset.count()) / quarter_interval + 2;
    int end = (right_bound - offset.count()) / quarter_interval - 1;

    for (int i = start; i < end; i++) {
        float x = offset.count() + i * quarter_interval;

        if (x > right_bound) {
            break;
        }

        float height;
        Color color;
        if (i % 4 == 0) {
            height = 0.2;
            color = WHITE;
        } else {
            height = 0.1;
            color = RED;
        }
        
        Vec2 p1 = cam.world_to_screen({x, 0});
        Vec2 p2 = cam.world_to_screen({x, height});

        DrawLine(p1.x, p1.y, p2.x, p2.y, color);
    }

    Vec2 p1 = cam.world_to_screen(cam.position);
    Vec2 p2 = cam.world_to_screen(cam.position + Vec2{0,0.6});

    DrawLine(p1.x, p1.y, p2.x, p2.y, YELLOW);


    draw_map_editor(map, cam, current_note);

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

        Vec2 scale = { end_pos.x - start_pos.x, end_pos.y - start_pos.y };
        DrawRectangle(start_pos.x, start_pos.y, scale.x, scale.y, { 255, 255, 255, 40 });
    }


    ui.draw();
    EndDrawing();
}


enum class View {
    main,
    map_select,
};

class MainMenu {
public:
    void update(std::function<void()> callback);

private:
    UI ui;
    View current_view = View::main;
};

void MainMenu::update(std::function<void()> callback) {
    if (IsKeyPressed(KEY_ONE)) {
        callback();
        std::cout << "fskadfh\n";
        return;
    }

    ui.input();


    switch (current_view) {
        case View::main:
            ui.begin_group({});
            ui.button("Play", [&]() {
                current_view = View::map_select;
                std::cout << "kfsahdfhj\n";
            });
            ui.rect("Settings");
            ui.rect("Exit");

            ui.end_group();
            break;
        case View::map_select:
            ui.rect("askjldhf");
            if (IsKeyPressed(KEY_ESCAPE)) {
                current_view = View::main;
            }
            break;
    }

    BeginDrawing();
    ClearBackground(BLACK);
    DrawText("editor", 400, 300, 24, WHITE);

    ui.draw();
    EndDrawing();
}


enum class Context {
    Menu,
    Editor,
    Game,
};

void run() {
    InitWindow(window_width, window_height, "taiko");
    //ToggleFullscreen();
    
    auto last_frame = std::chrono::high_resolution_clock::now();
    
    InitAudioDevice();
    SetExitKey(KEY_NULL);

    SetMasterVolume(0.5f);

    textures.inner_drum = LoadTexture("drum-inner.png");
    textures.outer_drum = LoadTexture("drum-outer.png");
    textures.circle = LoadTexture("circle.png");
    textures.circle_overlay = LoadTexture("circle-overlay.png");
    textures.big_circle = LoadTexture("big-circle.png");
    textures.big_circle_overlay = LoadTexture("big-circle-overlay.png");
    textures.select_circle = LoadTexture("circle-select.png");
    
    float pos = 400;
    auto start = std::chrono::high_resolution_clock::now();

    MainMenu menu;

    Editor editor;
    
    Game game;

    editor.init();

    Context context = Context::Editor;

    auto to_editor = [&]() {
        context = Context::Editor;
    };

    using namespace std::chrono;
    
    while (!WindowShouldClose()) {
        auto now = high_resolution_clock::now();
        duration<double> delta_time = now - last_frame;

        switch (context) {
		case Context::Menu:
			menu.update(to_editor);
			break;
		case Context::Editor:
			editor.update(delta_time);
			break;
		case Context::Game:
			game.update(delta_time);
			break;
        }

        last_frame = now;
    }
    
    CloseWindow();
}

int main() {
    run();
}