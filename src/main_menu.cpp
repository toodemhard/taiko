#include "main_menu.h"

#include "SDL3/SDL_scancode.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "map.h"
#include "serialize.h"
#include "ui.h"
#include <limits>

using namespace constants;

MainMenu::MainMenu(
    SDL_Renderer* _renderer,
    Input& _input,
    Audio& _audio,
    AssetLoader& _assets,
    EventQueue& _event_queue
)
    : renderer{_renderer}, input{_input}, audio{_audio}, assets{_assets}, event_queue{_event_queue} {
    reload_maps();
}

void MainMenu::reload_maps() {
    m_mapmetas.clear();
    m_parent_mapset.clear();
    m_mapsets.clear();
    m_mapset_paths.clear();
    m_map_buffers.clear();

    for (const auto& mapset : std::filesystem::directory_iterator(maps_directory)) {
        m_mapset_paths.push_back(mapset.path());
        m_mapsets.push_back({});
        int mapset_index = m_mapsets.size() - 1;

        auto maps_buffer = BufferHandle{(int)m_mapmetas.size()};

        load_binary(m_mapsets.back(), mapset.path() / mapset_filename);

        for (const auto& entry : std::filesystem::directory_iterator(mapset.path())) {
            if (entry.path().extension().string().compare(map_file_extension) == 0) {
                m_mapmetas.push_back({});
                partial_load_map_metadata(m_mapmetas.back(), entry.path());
                m_parent_mapset.push_back(mapset_index);

                maps_buffer.count++;
            }
        }

        m_map_buffers.push_back(maps_buffer);
    }
}

struct ButtonInfo {
    const char* text;
    Style style;
    OnClick on_click;
};

void MainMenu::update(double delta_time) {
    if (input.key_down(SDL_SCANCODE_F5)) {
        reload_maps();
    }

    m_ui.input(input);

    // if (entry_mode == EntryMode::Edit) {
    //     Style style{};
    //     style.position = Position::Absolute{0.25f, 0.5f};
    //     m_ui.begin_group(style);
    //
    //     m_ui.button("New Map", {}, [&](_info) {
    //         event_queue.push_event(Event::EditNewMap{});
    //     });
    //
    //     m_ui.end_group();
    // }

    auto play_selected_music = [&]() {
        auto mapset_directory = m_mapset_paths[m_selected_mapset_index];
        auto music_file = find_music_file(mapset_directory);
        if (music_file.has_value()) {
            audio.load_music(music_file.value().string().data());
            audio.play(std::numeric_limits<int>::max());
            audio.set_position(Mix_MusicDuration(audio.m_music) * (1 / 4.0f));
            audio.resume();
        }
    };

    auto group_st = Style{};
    group_st.position = Position::Anchor{0.5, 0};
    group_st.stack_direction = StackDirection::Vertical;
    group_st.gap = 50;
    m_ui.begin_group(group_st);

    SliderStyle slider_st{};
    slider_st.position = Position::Anchor{0.5, 0};
    slider_st.width = 500;
    slider_st.height = 100;
    slider_st.fg_color = color::red;
    m_ui.slider(
        m_music_slider, slider_st, (Mix_GetMusicVolume(audio.m_music) / (float)MIX_MAX_VOLUME), {[](float fraction) {
            Mix_VolumeMusic(MIX_MAX_VOLUME * fraction);
        }}
    );

    m_ui.slider(m_master_slider, slider_st, (master / (float)MIX_MAX_VOLUME), SliderCallbacks{[&](float fraction) {
                    master = MIX_MAX_VOLUME * fraction;
                    Mix_MasterVolume(master);
                }});

    m_ui.end_group();

    group_st = {};
    group_st.position = Position::Anchor{1, 0};
    group_st.stack_direction = StackDirection::Vertical;
    m_ui.begin_group(group_st);

    m_ui.begin_group({});
    m_ui.text("<<", {});
    m_ui.text("||", {});
    m_ui.text(">>", {});
    m_ui.end_group();

    auto duration = Mix_MusicDuration(audio.m_music);

    slider_st = {};
    slider_st.width = 300;
    slider_st.height = 50;
    slider_st.fg_color = color::white;
    m_ui.slider(
        m_music_playback_slider,
        slider_st,
        audio.get_position() / duration,
        SliderCallbacks{
            [&, duration](float fraction) { audio.set_position(fraction * duration); },
            [&]() { audio.pause(); },
            [&]() { audio.resume(); }
        }
    );

    m_ui.end_group();

    // float slider_width = 500;
    // float slider_height = 50;
    // auto slider = Style{};
    // slider.position = Position::Anchor{{0.5, 0}};
    // slider.width = Scale::Fixed{slider_width};
    // slider.height = Scale::Fixed{slider_height};
    // slider.border_color = color::white;
    //
    // ui.begin_group_button(slider, [](_info) {
    //     // Mix_MasterVolume(MIX_MAX_VOLUME * 0.3);
    // });
    // auto filler_st = Style{};
    // filler_st.background_color = color::red;
    // filler_st.width = Scale::Fixed{slider_width * (Mix_GetMusicVolume(audio.m_music) /
    // (float)MIX_MAX_VOLUME)}; filler_st.height = Scale::Fixed{slider_height};
    // ui.begin_group(filler_st);
    // ui.end_group();

    group_st = {};
    group_st.position = Position::Anchor{0, 0.5};
    m_ui.begin_group(group_st);

    auto cb = [this]() {
        auto callback = [](void* userdata, const char* const* filelist, int filter) {
            int i = 0;
            while(filelist[i] != nullptr) {
                std::cerr << std::format("{}\n", filelist[i]);
                if (load_osz(std::filesystem::path(filelist[i])) != 0) {
                    std::cerr << "failed to load: not osz\n";
                }

                (*(MainMenu*)userdata).reload_maps();

                i++;
            }
        };
        SDL_ShowOpenFileDialog(callback, this, NULL, NULL, 0, NULL, 1);
    };

    m_ui.button("Load .osz", {.border_color=color::white}, std::move(cb));
    m_ui.end_group();

    float map_item_width{500};
    float map_item_height{100};
    auto start_pos =
        Vec2{(constants::window_width - map_item_width) / 2.0f, (constants::window_height - map_item_height) / 2.0f};
    float gap{10};

    auto anim_style = AnimStyle{};
    anim_style.alt_background_color = RGBA{255, 255, 255, 50};

    if (m_choosing_mapset_index.has_value()) {
        auto mapset_index = m_choosing_mapset_index.value();
        auto map_buffer = m_map_buffers[mapset_index];

        if (input.key_down(SDL_SCANCODE_LEFT) && m_selected_diff_index > 0) {
            m_selected_diff_index--;
        }
        if (input.key_down(SDL_SCANCODE_RIGHT) && m_selected_diff_index < map_buffer.count - 1) {
            m_selected_diff_index++;
        }
        if (input.key_down(SDL_SCANCODE_ESCAPE)) {
            m_choosing_mapset_index = {};
        }
        if (input.key_down(SDL_SCANCODE_RETURN)) {
            auto& map = m_mapmetas[map_buffer.index + m_selected_diff_index];
            event_queue.push_event(
                Event::PlayMap{m_mapset_paths[mapset_index], (map.difficulty_name + map_file_extension)}
            );
        }

        group_st = {};
        group_st.position = Position::Anchor{0.5, 0.5};
        group_st.gap = 25;
        m_ui.begin_group(group_st);

        for (int i = 0; i < map_buffer.count; i++) {
            auto diff_st = Style{};
            diff_st.padding = even_padding(25);
            diff_st.border_color = color::white;
            diff_st.width = Scale::Fixed{150};
            if (i == m_selected_diff_index) {
                diff_st.background_color = RGBA{255, 255, 255, 50};
            }

            m_ui.button_anim(
                m_mapmetas[map_buffer.index + i].difficulty_name.data(),
                &m_diff_buttons[i],
                diff_st,
                anim_style,
                []() {}
            );
        }

        m_ui.end_group();

    } else {
        Style inactive_style{};
        inactive_style.text_color = RGBA{128, 128, 128, 255};

        Style active_style{};

        std::vector<ButtonInfo> option;
        option.push_back({"Play", inactive_style, [&]() { entry_mode = EntryMode::Play; }});

        option.push_back({"Edit", inactive_style, [&]() { entry_mode = EntryMode::Edit; }});

        if (entry_mode == EntryMode::Play) {
            option[0].style = active_style;
        } else {
            option[1].style = active_style;
        }

        Style style{};

        m_ui.begin_group(style);

        for (auto& info : option) {
            m_ui.button(info.text, info.style, std::move(info.on_click));
        }

        m_ui.end_group();


        auto enter_mapset = [&]() {
            m_choosing_mapset_index = m_selected_mapset_index;
            m_diff_buttons = std::vector<AnimState>(m_map_buffers[m_selected_mapset_index].count);
        };

        if (entry_mode == EntryMode::Play) {
            if (input.key_down(SDL_SCANCODE_RETURN)) {
                m_choosing_mapset_index = m_selected_mapset_index;
                m_diff_buttons = std::vector<AnimState>(m_map_buffers[m_selected_mapset_index].count);
            }
            // event_queue.push_event(Event::PlayMap{
            //     mapset_directory, (map_info.difficulty_name + map_file_extension)
            // });
            // // list maps
            // for (int i = 0; i < map_list.size(); i++) {
            //
            //     group_style = {};
            //     group_style.stack_direction = StackDirection::Vertical;
            //     // group_style.background_color = { 20, 20, 20, 255 };
            //     group_style.border_color = color::white;
            //     float l = 25;
            //     group_style.padding = {l, l, l, l};
            //
            //
            //     ui.begin_group_button(group_style, std::move(edit_map));
            //     ui.rect(mapset.title.data(), {});
            //     ui.rect(mapset.artist.data(), {.font_size = 24, .text_color = color::grey});
            //     ui.rect(map_info.difficulty_name.data(), {.font_size = 24});
            //     ui.end_group_v2();
            // }
        } else if (entry_mode == EntryMode::Edit) {
            if (input.key_down(SDL_SCANCODE_RETURN)) {
                event_queue.push_event(Event::EditMap{m_mapset_paths[m_selected_mapset_index], {}});
            }


            auto st = Style{};
            st.position = Position::Anchor{1, 0.5};
            m_ui.begin_group(st);

            m_ui.button("New Map", {}, [&]() {
                event_queue.push_event(Event::EditNewMap{});
            });

            m_ui.end_group();

            
            // enter_mapset = [&]() {
            //
            // };
            // list mapsets
            // for (int i = 0; i < m_mapsets.size(); i++) {}
            //     auto& mapset = m_mapsets[i];
            //     auto& mapset_directory = m_mapset_paths[i];
            //     item_st = {};
            //     item_st.stack_direction = StackDirection::Vertical;
            //     item_st.background_color = {20, 20, 20, 255};
            //     item_st.border_color = {100, 100, 50, 255};
            //     float l = 25;
            //     item_st.padding = {l, l, l, l};
            //
            //     OnClick edit_map = [&](ClickInfo info) {
            //         event_queue.push_event(Event::EditMap{mapset_directory, {}});
            //     };
            //
            //     m_ui.begin_group_button(item_st, std::move(edit_map));
            //     m_ui.text(mapset.title.data(), {});
            //     m_ui.text(mapset.artist.data(), {});
            //     m_ui.end_group();
            // }
        }

        if (input.key_down(SDL_SCANCODE_LEFT) && m_selected_mapset_index > 0) {
            m_selected_mapset_index--;
            play_selected_music();
        }
        if (input.key_down(SDL_SCANCODE_RIGHT) && m_selected_mapset_index < m_mapsets.size() - 1) {
            m_selected_mapset_index++;
            play_selected_music();
        }

        float scroll_velocity = 10;
        float scroll_move = scroll_velocity * delta_time;
        if (m_scroll_pos != m_selected_mapset_index) {
            if (std::abs(m_scroll_pos - m_selected_mapset_index) <= scroll_move) {
                m_scroll_pos = m_selected_mapset_index;
            } else {
                if (m_scroll_pos > m_selected_mapset_index) {
                    scroll_move *= -1;
                }
                m_scroll_pos += scroll_move;
            }
        }

        for (int i = 0; i < m_mapsets.size(); i++) {
            auto item_st = Style{};
            item_st.stack_direction = StackDirection::Vertical;
            item_st.width = Scale::Fixed{500};
            item_st.padding = even_padding(25);

            if (i == m_selected_mapset_index) {
                item_st.background_color = RGBA{255, 255, 255, 70};
            }

            auto index_offset = m_scroll_pos - i;
            auto offset = index_offset * Vec2{0, map_item_height + gap};

            item_st.position = Position::Absolute{start_pos - offset};

            auto& mapset = m_mapsets[i];
            auto& mapset_directory = m_mapset_paths[i];

            OnClick select_mapset = [&, i]() { m_selected_mapset_index = i; };

            OnClick enter_mapset = [&, i]() {
                m_choosing_mapset_index = m_selected_mapset_index;
                m_diff_buttons = std::vector<AnimState>(m_map_buffers[m_selected_mapset_index].count);
                m_selected_diff_index = {};
            };

            OnClick on_click = select_mapset;

            if (i == m_selected_mapset_index) {
                on_click = enter_mapset;
            }

            m_ui.begin_group_button(item_st, std::move(on_click));
            m_ui.text(mapset.title.data(), {});
            m_ui.text(mapset.artist.data(), {});
            m_ui.end_group();
        }
    }

    m_ui.end_frame();

    m_ui.draw(renderer);
}

// void MainMenu::render(SDL_Renderer* renderer) {
// }
