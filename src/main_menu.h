#pragma once

#include "systems.h"
#include "map.h"
#include "ui.h"
#include "constants.h"

enum class EntryMode {
    Play,
    Edit,
};

class MainMenu {
public:
    MainMenu(SDL_Renderer* _renderer, Input& _input, Audio& _audio, AssetLoader& _assets, EventQueue& _event_queue);
    
    void update();
    void reload_maps();

private:
    SDL_Renderer* renderer{};
    Input& input;
    Audio& audio;
    AssetLoader& assets;
    EventQueue& event_queue;

    UI ui{constants::window_width, constants::window_height};

    EntryMode entry_mode = EntryMode::Play;

    //maps
    std::vector<MapMeta> map_list;
    std::vector<int> mapset_index_list;

    //mapsets
    std::vector<MapSetInfo> mapsets;
    std::vector<std::filesystem::path> mapset_paths;

    TextFieldState search{.text = "ashkjfhkjh"};
};
