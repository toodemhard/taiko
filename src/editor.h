#pragma once

// #include "systems.h"
#include "map.h"
#include "memory.h"
#include "ui.h"
#include "constants.h"
#include "game.h"

enum class EditorMode {
    select,
    insert,
};

enum class EditorView {
    Main,
    MapSet,
    Timing,
};

class Editor {
public:
    Editor(MemoryAllocators& memory, SDL_Renderer* _renderer, Input::Input& _input, Audio& _audio, AssetLoader& _assets, EventQueue& _event_queue);
    ~Editor();
    void update(std::chrono::duration<double> delta_time);
    void load_mapset(std::filesystem::path& map_path);
    void refresh_maps();
    void quit();
    Map copy_map();

    int load_song(const char* file_path);


    double last_pos{};

    bool creating_map = false;
    std::optional<std::filesystem::path> m_song_path;

private:
    SDL_Renderer* renderer{};
    MemoryAllocators& memory;
    Input::Input& input;
    Audio& audio;
    AssetLoader& assets;
    EventQueue& event_queue;

    UI ui;

    Cam cam = {{0,0}, {2,1.5f}};

    MapSetInfo mapset_info{};

    Map m_map{};
    std::vector<bool> m_selected{};

    EditorView view = EditorView::Main;
    EditorMode editor_mode = EditorMode::select;
    std::optional<Vec2> box_select_begin;
    NoteFlags insert_flags = NoteFlagBits::don | NoteFlagBits::small;

    std::filesystem::path m_mapset_directory;
    std::vector<MapMeta> m_map_infos;
    std::vector<std::filesystem::path> m_map_paths;
    int m_current_map_index = 0;
    
    TextFieldState m_map_rename_field;
    bool m_renaming_map = false;

    bool paused = true;

    int current_note = -1;

    TextFieldState title{};
    TextFieldState artist{};

    // timing tab
    TextFieldState bpm{};
    TextFieldState offset{};

    void main_update();
    void load_map(int map_index);
    void remove_map(int map_index);
    void rename_map(int map_index, const std::string& new_name);
};
