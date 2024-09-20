#pragma once

#include "constants.h"
#include "map.h"
#include "systems.h"
#include "ui.h"

enum class EntryMode {
    Play,
    Edit,
};

struct BufferHandle {
    int index;
    int count;
};

class MainMenu {
  public:
    MainMenu(SDL_Renderer* _renderer, Input& _input, Audio& _audio, AssetLoader& _assets, EventQueue& _event_queue);

    void update(double delta_time);
    void render(SDL_Renderer* renderer);
    void reload_maps();

  private:
    SDL_Renderer* renderer;
    Input& input;
    Audio& audio;
    AssetLoader& assets;
    EventQueue& event_queue;

    UI m_ui{constants::window_width, constants::window_height};

    EntryMode entry_mode = EntryMode::Play;

    std::vector<const char*> m_resolutions{"1920x1080", "1024x768"};
    DropDown m_resolution_dropdown{};
    int m_selected_resolution_index{};

    Slider m_music_slider{};
    Slider m_master_slider{};

    int master{};
    Slider m_music_playback_slider{};

    std::optional<int> m_choosing_mapset_index{};
    std::vector<AnimState> m_diff_buttons{};
    int m_selected_diff_index{};

    int m_selected_mapset_index{};
    float m_scroll_pos{};

    // maps
    std::vector<MapMeta> m_mapmetas;
    std::vector<int> m_parent_mapset;

    // mapsets
    std::vector<MapSetInfo> m_mapsets;
    std::vector<BufferHandle> m_map_buffers;
    std::vector<std::filesystem::path> m_mapset_paths;

    TextFieldState search{.text = "ashkjfhkjh"};
};
