#include <cmath>
#include <iostream>
#include <limits>
#include <tracy/Tracy.hpp>

#include "editor.h"
#include "constants.h"
#include "map.h"
#include "serialize.h"
#include "color.h"
#include "game.h"

#include "assets.h"
#include "ui.h"

using namespace constants;


Map Editor::copy_map() {
    return m_map;
}

void insert_note(Map& map, std::vector<bool>& selected, double time, NoteFlags flags) {
    int i = std::upper_bound(map.times.begin(), map.times.end(), time) - map.times.begin();
    map.times.insert(map.times.begin() + i, time);
    map.flags_list.insert(map.flags_list.begin() + i, flags);
    selected.insert(selected.begin() + i, false);
}

void remove_note(Map& map, std::vector<bool>& selected, int i) {
    map.times.erase(map.times.begin() + i);
    map.flags_list.erase(map.flags_list.begin() + i);
    selected.erase(selected.begin() + i);
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

void draw_map_editor(SDL_Renderer* renderer, AssetLoader& assets, const Map& map, const std::vector<bool>& selected, const Cam& cam, int current_note) {
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

        float scale = (map.flags_list[i] & NoteFlagBits::small) ? 0.9f : 1.4f;
        Image circle_image = (map.flags_list[i] & NoteFlagBits::don) ? assets.get_image(ImageID::don_circle) : assets.get_image(ImageID::kat_circle);
        Image circle_overlay = assets.get_image(ImageID::circle_overlay);
        Image select_circle = assets.get_image(ImageID::select_circle);

        Vec2 circle_pos = center_pos;
        circle_pos.x -= circle_image.width / 2.0f * scale;
        circle_pos.y -= circle_image.height / 2.0f * scale;


        {
            SDL_FRect rect = { circle_pos.x, circle_pos.y, circle_image.width * scale, circle_image.height * scale };
            SDL_RenderTexture(renderer, circle_image.texture, NULL, &rect);
            SDL_RenderTexture(renderer, circle_overlay.texture, NULL, &rect);
        }

        Vec2 hitbox_bounds = cam.world_to_screen_scale(note_hitbox);
        Vec2 hitbox_pos = center_pos - (hitbox_bounds / 2.0f);
        //draw_wire_box(hitbox_pos, hitbox_bounds, RED);


        if (selected[i]) {
            SDL_FRect rect = {
                center_pos.x - select_circle.width / 2.0f * scale,
                center_pos.y - select_circle.height / 2.0f * scale,
                select_circle.width * scale,
                select_circle.height * scale,
            };
            SDL_RenderTexture(renderer, select_circle.texture, NULL, &rect);
        }
    }
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

Editor::Editor(SDL_Renderer* _renderer, Input& _input, Audio& _audio, AssetLoader& _assets, EventQueue& _event_queue)
    : renderer{ _renderer }, input{ _input }, audio{ _audio }, assets{ _assets }, event_queue{ _event_queue } {}

Editor::~Editor() {
}

void Editor::quit() {
    audio.stop();
}

void Editor::load_mapset(std::filesystem::path& mapset_directory) {
    m_mapset_directory = mapset_directory;

    load_binary(mapset_info, (m_mapset_directory / mapset_filename));
    m_selected = std::vector<bool>(m_map.times.size(), false);

    auto music_file = find_music_file(mapset_directory);

    if (music_file.has_value()) {
        audio.load_music(music_file.value().string().data());
        audio.play(std::numeric_limits<int>::max());
    }

    m_map_infos.clear();

    refresh_maps();

    if (m_map_paths.size() > 0) {
        this->load_map(m_current_map_index);
    }
}

void Editor::load_map(int map_index) {
    m_current_map_index = map_index;

    load_binary(m_map, m_map_paths[map_index]);
    m_selected = std::vector<bool>(m_map.times.size(), false);

    audio.set_position(0);
}

void Editor::remove_map(int map_index) {
    std::filesystem::remove(m_map_paths[map_index]);
    
    this->refresh_maps();
    return;
}

void Editor::refresh_maps() {
    m_map_infos.clear();
    m_map_paths.clear();

    for (const auto& entry : std::filesystem::directory_iterator(m_mapset_directory)) {
        if (entry.path().extension().string().compare(map_file_extension) == 0) {
            m_map_paths.push_back(entry.path());

            m_map_infos.push_back({});
            partial_load_map_metadata(m_map_infos.back(), entry.path());
        }
    }
}

void Editor::rename_map(int map_index, const std::string& new_name) {
    m_map.m_meta_data.difficulty_name = new_name;
    auto new_path = m_mapset_directory / (new_name + map_file_extension);
    auto& old_path = m_map_paths[map_index];

    if (old_path.compare(new_path) == 0) {
        return;
    }
    
    std::filesystem::rename(old_path, new_path);
    save_binary(m_map, new_path);

    this->refresh_maps();
}

int Editor::load_song(const char* file_path) {
    return audio.load_music(file_path);
}

void Editor::update(std::chrono::duration<double> delta_time) {
    ZoneScoped;

    ui.input(input);
    ui.begin_frame(constants::window_width, constants::window_height);

    Style style{};
    style.position = Position::Anchor{ 0.5, 0.5 };
    style.stack_direction = StackDirection::Vertical;
    style.background_color = { 9, 30, 64, 255 };

    float l = 25;
    style.padding = { l, l, l, l };

    StringCache strings;


    auto inactive_style = Style{};
    inactive_style.text_color = RGBA{ 128, 128, 128, 255 };


    ui.begin_row({});
    auto button_style = (view == EditorView::Main) ? Style{} : inactive_style;
    ui.button("Editor", button_style, [&]() {
        view = EditorView::Main;
    });

    button_style = (view == EditorView::MapSet) ? Style{} : inactive_style;
    ui.button("Mapset", button_style, [&]() {
        view = EditorView::MapSet;
    });

    button_style = (view == EditorView::Timing) ? Style{} : inactive_style;
    ui.button("Timing", button_style, [&]() {
        view = EditorView::Timing;
    });
    ui.end_row();

    if (creating_map) {
        ui.begin_row(style);

        ui.begin_row({});
        ui.text("title: ", {});;
        ui.text_field(&title, { .border_color = {255, 255, 255, 0}, .width = Scale::Min{200} });
        ui.end_row();

        ui.begin_row({});
        ui.text("artist: ", {});
        ui.text_field(&artist, { .border_color = {255, 255, 255, 0}, .width = Scale::Min{200} });
        ui.end_row();

        const char* text = (m_song_path.has_value()) ? strings.add(m_song_path.value().filename().string()) : "choose song";

        ui.button(text, {}, [&]() {
            auto callback = [](void* userdata, const char* const* filelist, int filter) {
                auto editor = (Editor*)userdata;

                if (filelist[0] == nullptr) {
                    std::cerr << "no files selected\n";
                    return;
                }

                if (editor->load_song(filelist[0]) != 0) {
                    std::cerr << "invalid file type\n";
                    return;
                }

                editor->m_song_path = std::filesystem::path(filelist[0]);
            };

            SDL_ShowOpenFileDialog(callback, this, NULL, NULL, 0, NULL, 0);
        });

        ui.button("enter", {}, [&]() {
            if (title.text.length() == 0) {
                std::cerr << "no title\n";
                return;
            }
            if (!m_song_path.has_value()) {
                std::cerr << "no song file\n";
                return;
            }

            m_mapset_directory = "data/maps/" + std::format("{} - {}", artist.text, title.text);

            std::filesystem::create_directories(m_mapset_directory);
            SDL_CopyFile(m_song_path.value().string().data(), (m_mapset_directory / m_song_path.value().filename().c_str() ).string().data());

            mapset_info = { title.text, artist.text };


            save_binary(mapset_info, (m_mapset_directory / mapset_filename));

            creating_map = false;
        });

        ui.end_row();
    }


    switch (view) {
    case EditorView::Main: {
        main_update();
        break;
    }
    case EditorView::MapSet: { 
        if (input.key_down(SDL_SCANCODE_F2)) {
            m_renaming_map = true;

            m_map_rename_field.text = m_map_infos[m_current_map_index].difficulty_name;
        }

        if (m_renaming_map == true) {
            if (input.key_down(SDL_SCANCODE_RETURN)) {
                std::cerr << "enter\n";
                std::cerr << m_map_rename_field.text;

                m_renaming_map = false;

                this->rename_map(m_current_map_index, m_map_rename_field.text);

            }

        }

        Style style = {};
        style.position = Position::Anchor{ 0.25, 0.25 };
        style.stack_direction = StackDirection::Vertical;
        ui.begin_row(style);
        ui.text(mapset_info.title.data(), {.font_size = 52});
        ui.text(mapset_info.artist.data(), { .text_color = RGBA{180, 180, 180, 255} });

        ui.end_row();

        style = {};
        style.position = Position::Anchor{ 0.5, 0.5 };
        style.stack_direction = StackDirection::Vertical;
        style.gap = 36;
        ui.begin_row(style);

        style = {};
        style.stack_direction = StackDirection::Vertical;
        ui.begin_row(style);

        auto selected_style = Style{};
        selected_style.border_color = { 255, 255, 255, 255 };

        auto deselected_style = Style{};
        deselected_style.text_color = RGBA{ 180, 180, 180, 255 };
        
        
        for (int i = 0; i < m_map_infos.size(); i++) {
            auto& diff = m_map_infos[i];
            
            if (i == m_current_map_index && m_renaming_map) {
                ui.text_field(&m_map_rename_field, { .text_color{RGBA{255, 0, 0, 255}} });
                continue;
            }

            auto entry_style = [&]() -> const Style& {
                if (m_current_map_index == i) {
                    return selected_style;
                } else {
                    return deselected_style;
                }
            }();

            ui.begin_row({});

            ui.button(diff.difficulty_name.data(), entry_style, [&, i]() {
                this->load_map(i);
            });

            ui.button("x", {}, [&, i]() {
                this->remove_map(i);
            });

            ui.end_row();
        }

        ui.end_row();

        auto button_style = Style{};
        button_style.background_color = { 255, 255, 255, 255 };
        button_style.text_color = RGBA{ 0, 0, 0, 255 };
        button_style.padding = even_padding(10);
        ui.button("Add Difficulty", button_style, [&]() {
            int duplicate_number = 0;
            auto unique_file_name = std::string("new difficulty");

            while (std::filesystem::exists(m_mapset_directory / (unique_file_name + map_file_extension))) {
                duplicate_number += 1;
                unique_file_name = std::format("new difficulty({})", duplicate_number);
            }

            m_map = Map{
                MapMeta { .difficulty_name = unique_file_name}
            };


            save_binary(m_map, m_mapset_directory / (unique_file_name + map_file_extension));

            refresh_maps();
        });


        ui.end_row();

        break;
    }
    case EditorView::Timing: {
        auto style = Style{};
        style.position = Position::Anchor{ 0.5, 0.5 };
        style.stack_direction = StackDirection::Vertical;
        ui.begin_row(style);

        auto fs = Style{};
        fs.width = Scale::Min{200};

        if (!bpm.focused) {
            bpm.text = std::format("{}", m_map.m_meta_data.bpm);
        }

        ui.begin_row({ .gap{10} });
        ui.text("bpm", {});
        ui.text_field(&bpm, fs);
        ui.end_row();

        if (bpm.focused && input.key_down(SDL_SCANCODE_RETURN)) {
            bpm.focused = false;
            m_map.m_meta_data.bpm = std::stof(bpm.text);
        }

        if (!offset.focused) {
            offset.text = std::format("{:.2f}", m_map.m_meta_data.offset);
        }

        ui.begin_row({ .gap{10} });
        ui.text("offset", {});
        ui.text_field(&offset, fs);
        ui.end_row();

        if (offset.focused && input.key_down(SDL_SCANCODE_RETURN)) {
            offset.focused = false;
            m_map.m_meta_data.offset = std::stof(offset.text);
        }

        ui.end_row();
        break;
    }
    }

    ui.end_frame();

    ui.draw(renderer);
}

void set_draw_color(SDL_Renderer* renderer, RGBA color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void Editor::main_update() {
    ZoneScoped;

    double elapsed = audio.get_position();
    last_pos = elapsed;

    Style style = {};
    style.position = Position::Anchor{ 0, 0.5 };
    style.stack_direction = StackDirection::Vertical;

    ui.begin_row(style); {
        const char* note_color_text = (insert_flags & NoteFlagBits::don) ? "Don" : "Kat";

        const char* note_size_text = (insert_flags & NoteFlagBits::small) ? "Normal" : "Big";

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
            insert_flags = insert_flags ^ NoteFlagBits::small;
        });

        ui.button(note_color_text, {}, [&]() {
            insert_flags = insert_flags ^ NoteFlagBits::don;
        });
    }
    ui.end_row();

    ui.begin_row(Style{ Position::Anchor{0,1} });
        ui.text(ui.strings.add(std::to_string(cam.position.x) + " s"), {});
    //ui.slider(elapsed / GetMusicTimeLength(music), [&](float fraction) {
    //    SeekMusicStream(music, fraction * GetMusicTimeLength(music));
    //});
    ui.end_row();



    Vec2 cursor_pos = cam.screen_to_world(input.mouse_pos);

    if (input.key_down(SDL_SCANCODE_ESCAPE)) {
        event_queue.push_event(Event::Return{});
    }

    std::optional<int> note_at_cursor_index = note_point_intersection(m_map, cursor_pos, current_note);
    if (input.mouse_down(SDL_BUTTON_RMASK) && note_at_cursor_index.has_value()) {
        if (m_selected[note_at_cursor_index.value()]) {
            for (int i = m_selected.size() - 1; i >= 0; i--) {
                if (m_selected[i]) {
                    remove_note(m_map, m_selected, i);
                }
            }
        }
        else {
            remove_note(m_map, m_selected, note_at_cursor_index.value());
        }
    }

    if (input.key_down(SDL_SCANCODE_S) && input.modifier(SDL_KMOD_LCTRL)) {
        save_binary(m_map, m_map_paths[m_current_map_index]);
    }

    if (input.key_down(SDL_SCANCODE_A) && input.modifier(SDL_KMOD_LCTRL)) {
        std::fill(m_selected.begin(), m_selected.end(), true);
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

    auto& offset = m_map.m_meta_data.offset;
    auto& bpm = m_map.m_meta_data.bpm;

    double quarter_interval = 60 / bpm / 4;
    double collision_range = quarter_interval / 2;

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
            if (note_at_cursor_index.has_value()) {
                m_selected[note_at_cursor_index.value()] = m_selected[note_at_cursor_index.value()];
            } else { 
                box_select_begin = cursor_pos;
            }
        }

        if (input.mouse_held(SDL_BUTTON_LMASK)) {
            if (box_select_begin.has_value()) {
                auto hits = note_box_intersection(m_map, box_select_begin.value(), cursor_pos);
                std::fill(m_selected.begin(), m_selected.end(), false);

                for (auto& i : hits) {
                    m_selected[i] = true;
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
            for (int i = 0; i < m_map.times.size(); i++) {
                diff = std::abs((m_map.times[i] - time));
                if (diff < collision_range) {
                    collision = true;
                    break;
                }
            }

            if (!collision) {
                insert_note(m_map, m_selected, time, insert_flags);

                if (time < elapsed) {
                    current_note++;
                }
            }
        }
        break;
    }

    if (input.key_down(SDL_SCANCODE_F5)) {
        event_queue.push_event(Event::TestMap{});
    }

    if (input.key_down(SDL_SCANCODE_Q)) { 
        editor_mode = EditorMode::select;
    }

    if (input.key_down(SDL_SCANCODE_W)) { 
        editor_mode = EditorMode::insert;
    }

    if (input.key_down(SDL_SCANCODE_E)) {
        insert_flags = insert_flags ^ NoteFlagBits::small;
    }

    if (input.key_down(SDL_SCANCODE_R)) {
        insert_flags = insert_flags ^ NoteFlagBits::don;
    }

    if (input.key_down(SDL_SCANCODE_SPACE)) {
        if (audio.paused()) {
            audio.resume();
            current_note = std::upper_bound(m_map.times.begin(), m_map.times.end(), elapsed) - m_map.times.begin();
        } else {
            audio.pause();
        }
    }

    if (!audio.paused() && current_note < m_map.times.size() && elapsed >= m_map.times[current_note]) {
        switch (m_map.flags_list[current_note]) {
        case (NoteFlagBits::don | NoteFlagBits::small):
            Mix_PlayChannel(-1, assets.get_sound(SoundID::don), 0);
            break;
        case (0 | NoteFlagBits::small):
            Mix_PlayChannel(-1, assets.get_sound(SoundID::kat), 0);
            break;
        case (NoteFlagBits::don | 0):
            Mix_PlayChannel(-1, assets.get_sound(SoundID::kat), 0);
            break;
        case (0 | 0):
            Mix_PlayChannel(-1, assets.get_sound(SoundID::kat), 0);
            break;
        }

        current_note++;
    }


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
        RGBA color;
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

    draw_map_editor(renderer, assets, m_map, m_selected, cam, current_note);

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

        set_draw_color(renderer, { 255, 255, 255, 255 });
        SDL_RenderRect(renderer, &rect);
    }
}
