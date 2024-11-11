#pragma once

#include "constants.h"
#include "input.h"
#include "map.h"
#include "systems.h"
#include "ui.h"

enum class EntryMode {
    Play,
    Edit,
};

enum class View {
    Main,
    Settings,
};

struct Slice {
    int index;
    int count;
};

class MainMenu {
  public:
    MainMenu(MemoryAllocators& memory, SDL_Renderer* _renderer, Input::Input& _input, Audio& _audio, AssetLoader& _assets, EventQueue& _event_queue);

    void awake();
    void update(double delta_time);
    void reload_maps();
    void play_selected_music();

  private:
    SDL_Renderer* renderer;
    MemoryAllocators& memory;
    Input::Input& input;
    Audio& audio;
    AssetLoader& assets;
    EventQueue& event_queue;

    EntryMode m_entry_mode = EntryMode::Play;
    View m_view = View::Main;

    std::vector<const char*> m_resolutions{"1920x1080", "1024x768"};
    DropDown m_resolution_dropdown{};
    int m_selected_resolution_index{};

    Slider m_music_slider{};
    Slider m_master_slider{};

    Slider m_slider{};
    float kys{};

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
    std::vector<Slice> m_map_buffers;
    std::vector<std::filesystem::path> m_mapset_paths;

    std::vector<AnimState> m_mapset_buttons{};

    std::optional<Input::ActionID> m_remapping_action;

    std::array<AnimState, Input::ActionID::count> m_remap_buttons;

    TextFieldState search{.text = "ashkjfhkjh"};

    AnimState m_load_button;
};
