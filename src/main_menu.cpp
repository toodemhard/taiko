#include "main_menu.h"

#include "serialize.h"

using namespace constants;

MainMenu::MainMenu(SDL_Renderer* _renderer, Input& _input, Audio& _audio, AssetLoader& _assets, EventQueue& _event_queue)
    : renderer{ _renderer }, input{ _input }, audio{ _audio }, assets{ _assets }, event_queue{ _event_queue } {
    reload_maps();
}

void MainMenu::reload_maps() {
    map_list.clear();
    mapset_index_list.clear();
    mapsets.clear();
    mapset_paths.clear();

    for (const auto& mapset : std::filesystem::directory_iterator(maps_directory)) {
        mapset_paths.push_back(mapset.path());
        mapsets.push_back({});
        int mapset_index = mapsets.size() - 1;

        load_binary(mapsets.back(), mapset.path() / mapset_filename);

        for (const auto& entry : std::filesystem::directory_iterator(mapset.path())) {
            if (entry.path().extension().string().compare(map_file_extension) == 0) {
                map_list.push_back({});
                load_binary(map_list.back(), entry.path());
                mapset_index_list.push_back(mapset_index);
            }
        }
    }

}

struct ButtonInfo {
    const char* text;
    Style style;
    std::function<void()> on_click;
};

void MainMenu::update() {
    std::vector<ButtonInfo> option;

    if (input.key_down(SDL_SCANCODE_F5)) {
        reload_maps();
    }

    Style inactive_style{};
    inactive_style.text_color = { 128, 128 ,128, 255 };

    Style active_style{};

    option.push_back({ "Play", inactive_style, [&]() {
        entry_mode = EntryMode::Play;
    } });

    option.push_back({ "Edit", inactive_style, [&]() {
        entry_mode = EntryMode::Edit;
    } });


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

        ui.button("New Map", {}, [&]() {
            event_queue.push_event(Event::EditNewMap{});
            });

        ui.end_group();
    }

    auto group_style = Style{};
    group_style.stack_direction = StackDirection::Vertical;
    group_style.gap = 25;
    group_style.anchor = { 1, 0.5 };
    group_style.padding = { .right = 25 };

    ui.begin_group(group_style);



    if (entry_mode == EntryMode::Play) {
        // list maps
        for (int i = 0; i < map_list.size(); i++) {
            auto& map_info = map_list[i];
            int mapset_index = mapset_index_list[i];
            auto& mapset = mapsets[mapset_index];
            auto& mapset_directory = mapset_paths[mapset_index];

            group_style = {};
            group_style.stack_direction = StackDirection::Vertical;
            //group_style.background_color = { 20, 20, 20, 255 };
            group_style.border_color = color::white;
            float l = 25;
            group_style.padding = { l, l, l, l };

            OnClick edit_map = [&](ClickInfo info) {
                event_queue.push_event(Event::PlayMap{ mapset_directory, (map_info.difficulty_name + map_file_extension)});
            };

            ui.begin_group_v2(group_style, { edit_map });
            ui.rect(mapset.title.data(), {});
            ui.rect(mapset.artist.data(), { .font_size{24}, .text_color = color::grey });
            ui.rect(map_info.difficulty_name.data(), { .font_size{24} });
            ui.end_group_v2();
        }

    }
    else if (entry_mode == EntryMode::Edit) {
        // list mapsets
        for (int i = 0; i < mapsets.size(); i++) {
            auto& mapset = mapsets[i];
            auto& mapset_directory = mapset_paths[i];
            group_style = {};
            group_style.stack_direction = StackDirection::Vertical;
            group_style.background_color = { 20, 20, 20, 255 };
            group_style.border_color = { 100, 100, 50, 255 };
            float l = 25;
            group_style.padding = { l, l, l, l };

            OnClick edit_map = [&](ClickInfo info) {
                event_queue.push_event(Event::EditMap{ mapset_directory, {} });
                };

            ui.begin_group_v2(group_style, { edit_map });
            ui.rect(mapset.title.data(), {});
            ui.rect(mapset.artist.data(), {});
            ui.end_group_v2();
        }

    }


    ui.end_group();

    ui.input(input);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);


    ui.draw(renderer);
    SDL_RenderPresent(renderer);
}
