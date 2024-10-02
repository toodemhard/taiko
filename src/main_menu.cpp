#include "main_menu.h"

#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "assets.h"
#include "audio.h"
#include "constants.h"
#include "input.h"
#include "map.h"
#include "serialize.h"
#include "ui.h"
#include <limits>
#include <mutex>

using namespace constants;

std::mutex map_reloading_mutex;

MainMenu::MainMenu(
    SDL_Renderer* _renderer,
    Input::Input& _input,
    Audio& _audio,
    AssetLoader& _assets,
    EventQueue& _event_queue
)
    : renderer{_renderer}, input{_input}, audio{_audio}, assets{_assets}, event_queue{_event_queue} {
}

void MainMenu::play_selected_music() {
    if (m_mapset_paths.size() > 0) {
        auto mapset_directory = m_mapset_paths[m_selected_mapset_index];
        auto music_file = find_music_file(mapset_directory);
        if (music_file.has_value()) {
            audio.load_music(music_file.value().string().data());
            audio.fade_in(std::numeric_limits<int>::max(), 300);
            audio.set_position(m_mapsets[m_selected_mapset_index].preview_time);
            audio.resume();
        }
    }
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

    m_mapset_buttons = std::vector<AnimState>(m_mapsets.size());

    this->play_selected_music();
}

struct ButtonInfo {
    const char* text;
    OnClick on_click;
};

void MainMenu::awake() {
    reload_maps();
}

void MainMenu::update(double delta_time) {
    if (input.key_down(SDL_SCANCODE_F5)) {
        reload_maps();
    }

    if (input.modifier(SDL_KMOD_LCTRL) && input.key_down(SDL_SCANCODE_W)) {
        m_view = View::Main;
        m_entry_mode = EntryMode::Play;
    }

    if (input.modifier(SDL_KMOD_LCTRL) && input.key_down(SDL_SCANCODE_E)) {
        m_view = View::Main;
        m_entry_mode = EntryMode::Edit;
    }

    if (input.modifier(SDL_KMOD_LCTRL) && input.key_down(SDL_SCANCODE_R)) {
        m_view = View::Settings;
    }

    // if (input.action_down(ActionID::don_left)) {
    //     std::cerr << "akjhfkalsdhf\n";
    // }

    m_ui.input(input);

    m_ui.begin_frame(constants::window_width, constants::window_height);


    auto row_st = Style{};
    auto anim_style = AnimStyle{};
    anim_style.alt_background_color = color::bg_highlight;


    auto begin_banner = [&](){
        Style inactive_style{};
        inactive_style.text_color = RGBA{128, 128, 128, 255};

        Style active_style{};

        std::vector<ButtonInfo> option;
        option.push_back({"Play", [&]() {
            m_view = View::Main;
            m_entry_mode = EntryMode::Play; 
        }});

        option.push_back({"Edit", [&]() {
            m_view = View::Main;
            m_entry_mode = EntryMode::Edit; 
        }});

        option.push_back({"Settings", [&](){
            m_view = View::Settings;
        }});


        int selected = 0;
        if (m_view == View::Main) {
            if (m_entry_mode == EntryMode::Play) {
                selected = 0;
            } else if (m_entry_mode == EntryMode::Edit){
                selected = 1;
            }
        } else if (m_view == View::Settings) {
            selected = 2;
        }

        Style style{};
        style.gap = 20;
        style.background_color = RGBA{30, 30, 30, 255};
        style.padding = even_padding(20);
        style.width = Scale::Fixed{(float)constants::window_width};

        m_ui.begin_row(style);

        for (int i = 0; i < option.size(); i++) {
            auto& info = option[i];
            auto style = inactive_style;
            if (selected == i) {
                style = active_style;
            }
            m_ui.button(info.text, style, std::move(info.on_click));
        }

    };

    auto end_banner = [&]() {
        m_ui.end_row();
    };

    SDL_RenderTexture(renderer, assets.get_image(ImageID::bg).texture, NULL, NULL);


    if (m_choosing_mapset_index.has_value()) {
        auto mapset_index = m_choosing_mapset_index.value();
        auto map_buffer = m_map_buffers[mapset_index];

        if (input.key_down(SDL_SCANCODE_LEFT) && m_selected_diff_index > 0) {
            m_selected_diff_index--;
            Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_select), 0);
        }
        if (input.key_down(SDL_SCANCODE_RIGHT) && m_selected_diff_index < map_buffer.count - 1) {
            m_selected_diff_index++;
            Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_select), 0);
        }
        if (input.key_down(SDL_SCANCODE_ESCAPE)) {
            m_choosing_mapset_index = {};
            Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_back), 0);
        }
        if (input.key_down(SDL_SCANCODE_RETURN)) {
            auto& map = m_mapmetas[map_buffer.index + m_selected_diff_index];
            Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_confirm), 0);
            event_queue.push_event(
                Event::PlayMap{m_mapset_paths[mapset_index], (map.difficulty_name + map_file_extension)}
            );
        }

        {
            Style prst{};
            prst.position = Position::Anchor{0, 1};
            prst.padding.left = 40;
            prst.padding.bottom = 40;

            m_ui.begin_row(prst);

            Style rst{};
            // rst.width = Scale::Fixed{300};
            rst.stack_direction = StackDirection::Vertical;
            rst.background_color = color::bg;
            rst.padding = even_padding(20);

            m_ui.begin_row(rst);
            m_ui.text("Diffifculty Guide", {.padding.bottom=10});
            m_ui.text(
            R"(Kantan = Easy
Futsuu = Normal
Muzukashii = Hard
Oni = Insane
Inner Oni = harder than Oni
Non standard name = hardest diff)"
            , {.font_size = 32, .text_color=color::grey});

            m_ui.end_row();
            m_ui.end_row();

        }

        row_st = {};
        row_st.stack_direction = StackDirection::Vertical;
        row_st.position = Position::Anchor{0.5, 0.5};
        row_st.gap = 25;
        m_ui.begin_row(row_st);

        for (int i = 0; i < map_buffer.count; i++) {
            auto diff_st = Style{};
            diff_st.padding = even_padding(25);
            diff_st.width = Scale::Fixed{500};
            diff_st.background_color = RGBA{31, 31, 31, 200};

            if (i == m_selected_diff_index) {
                diff_st.background_color = color::white;
                diff_st.text_color = color::black;
                m_ui.button(
                    m_mapmetas[map_buffer.index + i].difficulty_name.data(),
                    diff_st,
                    [&, i, mapset_index, map_buffer]() {
                        auto& map = m_mapmetas[map_buffer.index + m_selected_diff_index];
                        event_queue.push_event(
                            Event::PlayMap{m_mapset_paths[mapset_index], (map.difficulty_name + map_file_extension)}
                        );
                    }
                );

            } else {
                m_ui.button_anim(
                    m_mapmetas[map_buffer.index + i].difficulty_name.data(),
                    &m_diff_buttons[i],
                    diff_st,
                    anim_style,
                    [&, i]() {
                        m_selected_diff_index = i;
                        Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_select), 0);
                    }
                );
            }

        }

        m_ui.end_row();

    } else if(m_view == View::Main) {
        SliderStyle slider_st{};



        auto cb = [this]() {
            auto callback = [](void* userdata, const char* const* filelist, int filter) {
                int i = 0;
                while(filelist[i] != nullptr) {
                    std::cerr << std::format("{}\n", filelist[i]);
                    if (load_osz(std::filesystem::path(filelist[i])) != 0) {
                        std::cerr << "failed to load: not osz\n";
                    }

                    i++;
                }

                map_reloading_mutex.lock();
                (*(MainMenu*)userdata).reload_maps();
                map_reloading_mutex.unlock();
            };
            SDL_ShowOpenFileDialog(callback, this, NULL, NULL, 0, NULL, 1);
        };

        Style r_st = {};
        r_st.position = Position::Anchor{1, 0.5};
        r_st.padding.right = 30;
        m_ui.begin_row(r_st); {
            Style st{};
            st.background_color = color::bg;
            st.padding = even_padding(20);

            AnimStyle anim_st{};
            anim_st.alt_background_color = color::bg_highlight;

            m_ui.button_anim("+ Load .osz", &m_load_button, st, anim_st, std::move(cb));
        } m_ui.end_row();

        r_st = {};
        r_st.position = Position::Anchor{0, 1};
        r_st.padding.left = 40;
        r_st.padding.bottom = 40;
        m_ui.begin_row(r_st); {

            Style st{};
            st.stack_direction = StackDirection::Vertical;
            st.background_color = color::bg;
            st.padding = even_padding(20);
            m_ui.begin_row(st);

            m_ui.text("Menu Controls", {.padding.bottom=10});

            m_ui.text(R"(Left/Right
Return
Escape)",
            {.font_size=32, .text_color=color::grey});

            m_ui.end_row();
        } m_ui.end_row();


        auto enter_mapset = [&]() {
            m_choosing_mapset_index = m_selected_mapset_index;
            m_selected_diff_index = 0;
            m_diff_buttons = std::vector<AnimState>(m_map_buffers[m_selected_mapset_index].count);
        };

        if (m_entry_mode == EntryMode::Play) {
            if (input.key_down(SDL_SCANCODE_RETURN)) {
                m_choosing_mapset_index = m_selected_mapset_index;
                m_selected_diff_index = 0;
                m_diff_buttons = std::vector<AnimState>(m_map_buffers[m_selected_mapset_index].count);
                Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_confirm), 0);
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
        } else if (m_entry_mode == EntryMode::Edit) {
            if (input.key_down(SDL_SCANCODE_RETURN)) {
                event_queue.push_event(Event::EditMap{m_mapset_paths[m_selected_mapset_index], {}});
            }


            auto st = Style{};
            st.position = Position::Anchor{1, 0.5};
            m_ui.begin_row(st);

            m_ui.end_row();

            
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
            Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_select), 0);
            this->play_selected_music();
        }
        if (input.key_down(SDL_SCANCODE_RIGHT) && m_selected_mapset_index < m_mapsets.size() - 1) {
            m_selected_mapset_index++;
            Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_select), 0);
            this->play_selected_music();
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

        float map_item_width{800};
        float map_item_height{120};
        auto start_pos = Vec2{(constants::window_width - map_item_width) / 2.0f, (constants::window_height - map_item_height) / 2.0f};
        float gap{20};

        map_reloading_mutex.lock();
        for (int i = 0; i < m_mapsets.size(); i++) {
            auto item_st = Style{};
            item_st.stack_direction = StackDirection::Vertical;
            item_st.width = Scale::Fixed{map_item_width};
            item_st.height = Scale::Fixed{map_item_height};
            item_st.padding = even_padding(25);
            item_st.background_color = RGBA{31, 31, 31, 200};


            auto index_offset = m_scroll_pos - i;
            auto offset = index_offset * Vec2{0, map_item_height + gap};

            item_st.position = Position::Absolute{start_pos - offset};

            auto& mapset = m_mapsets[i];
            auto& mapset_directory = m_mapset_paths[i];

            Style idk{};
            idk.text_color = Inherit{};
            idk.width = Scale::FitParent{};

            auto idk2 = idk;
            idk2.font_size=28;

            if (i == m_selected_mapset_index) {
                item_st.background_color = color::white;
                item_st.text_color = RGBA{31, 31, 31, 200};

                m_ui.begin_row_button(item_st, [&, i]() {
                    if (m_entry_mode == EntryMode::Play) {
                        m_choosing_mapset_index = m_selected_mapset_index;
                        m_diff_buttons = std::vector<AnimState>(m_map_buffers[m_selected_mapset_index].count);
                        m_selected_diff_index = {};
                    } else {
                        event_queue.push_event(Event::EditMap{m_mapset_paths[m_selected_mapset_index], {}});
                    }
                });
                m_ui.text(mapset.title.data(), idk);
                m_ui.text(mapset.artist.data(), idk2);
                m_ui.end_row();
            } else {
                AnimStyle anim_st = {};
                anim_st.alt_background_color = color::bg_highlight;

                m_ui.begin_row_button_anim(&m_mapset_buttons[i], item_st, anim_st, [&, i]() {
                    m_selected_mapset_index = i; 
                    Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_select), 0);
                    this->play_selected_music();
                });
                m_ui.text(mapset.title.data(), idk);
                m_ui.text(mapset.artist.data(), idk2);
                m_ui.end_row();

            }
        }
        map_reloading_mutex.unlock();


        begin_banner();

        row_st = {};
        row_st.position = Position::Anchor{1, 0.5};
        // row_st.height = Scale::Fixed{200};
        row_st.align_items = Alignment::Center;
        // row_st.stack_direction = StackDirection::Vertical;
        m_ui.begin_row(row_st);


        {
            const char* text = audio.paused() ? "Play" : "Pause";
            m_ui.button(text, {}, [&](){
                if (audio.paused()) {
                    audio.resume();
                } else {
                    audio.pause();
                }
            });
        }


        auto duration = Mix_MusicDuration(audio.m_music);

        slider_st = {};
        slider_st.width = 300;
        slider_st.height = 30;
        slider_st.fg_color = color::white;
        slider_st.bg_color = RGBA{80, 80, 80, 255};

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

        {
            struct time {
                int mins;
                int secs;
            };
            auto split_time = [](double duration) {
                int mins = duration / 60;
                int secs = duration - (mins * 60);

                return time{mins,secs};
            };

            auto current = split_time(audio.get_position());
            auto total = split_time(Mix_MusicDuration(audio.m_music));

            auto text = m_ui.strings.add(std::format("{:02}:{:02}/{:02}:{:02}", current.mins, current.secs, total.mins, total.secs));
            m_ui.text(text, {.padding.left=8, .font_size=28}); }

        m_ui.end_row();

        end_banner();

    } else if (m_view == View::Settings) {
        if (input.key_down(SDL_SCANCODE_ESCAPE)) {
            // m_view = View::Main;
            m_remapping_action = {};
        }

        if (m_remapping_action.has_value()) {
            if (input.m_key_this_frame.has_value()) {
                input.keybindings[m_remapping_action.value()] = input.m_key_this_frame.value();
                m_remapping_action = {};
            }
        }

        {
            Style st{};
            st.position = Position::Absolute{0,0};
            st.width = Scale::FitParent{};
            st.height = Scale::FitParent{};
            st.background_color = RGBA{40, 40, 40, 160};

            m_ui.begin_row(st);
            m_ui.end_row();
        }

        begin_banner();
        end_banner();



        Style r_st{};
        r_st.position = Position::Absolute{200, 200};
        r_st.stack_direction = StackDirection::Vertical;
        r_st.gap = 50;
        m_ui.begin_row(r_st);

        // auto r_st = Style{}
        // r_st.position = Position::Anchor{0.5, 0};
        // r_st.stack_direction = StackDirection::Vertical;
        // r_st.gap = 50;
        // m_ui.begin_row(r_st);

        Style header{};
        header.font_size = 44;
        header.padding.bottom = 20;
        m_ui.begin_row({.stack_direction=StackDirection::Vertical});
        {
            m_ui.text("Audio", header);

            SliderStyle slider_st{};
            slider_st.width = 300;
            slider_st.height = 50;
            slider_st.fg_color = color::red;
            slider_st.bg_color = color::bg;

            float volume_fraction = (Mix_GetMusicVolume(audio.m_music) / (float)MIX_MAX_VOLUME);
            m_ui.begin_row({.stack_direction=StackDirection::Vertical, .gap=20});

            Style line{};
            line.width = Scale::Fixed{800};
            line.justify_items = JustifyItems::apart;
            line.align_items = Alignment::Center;

            m_ui.begin_row(line);
            m_ui.text("Music", {});

            m_ui.begin_row({.gap=10});
            m_ui.slider( m_music_slider, slider_st, volume_fraction, {[](float fraction) {
                    Mix_VolumeMusic(MIX_MAX_VOLUME * fraction);
            }});
            m_ui.text(m_ui.strings.add(std::format("{:.0f}%", volume_fraction * 100)), {});
            m_ui.end_row();
            m_ui.end_row();

            m_ui.begin_row(line);
            m_ui.text("Effect", {.padding.bottom=10});

            m_ui.begin_row({.gap=10});
            float effect_fraction = (effect_volume / (float)MIX_MAX_VOLUME);
            m_ui.slider(m_master_slider, slider_st, effect_fraction, SliderCallbacks{[&](float fraction) {
                effect_volume = MIX_MAX_VOLUME * fraction;
                Mix_MasterVolume(effect_volume);
            }});
            m_ui.text(m_ui.strings.add(std::format("{:.0f}%", effect_fraction * 100)), {});
            m_ui.end_row();
            m_ui.end_row();

            m_ui.end_row();
        }
        m_ui.end_row();

        m_ui.begin_row({.stack_direction=StackDirection::Vertical}); {
            m_ui.text("Keybinds", header);
            m_ui.begin_row({.stack_direction=StackDirection::Vertical, .align_items=Alignment::Center, .gap=10});
            for (int action_id = 0; action_id < Input::ActionID::count; action_id++) {
                m_ui.begin_row({.width=Scale::Fixed{800}, .justify_items=JustifyItems::apart});
                m_ui.text(Input::action_names[action_id], {});

                Style st{};
                st.width = Scale::Fixed{400};
                st.background_color = color::bg;
                st.text_align = TextAlign::Center;
                st.padding = even_padding(5);

                AnimStyle alt_st{};
                alt_st.alt_background_color = RGBA {130, 130, 130, 200};
                if (m_remapping_action.has_value() && m_remapping_action.value() == action_id) {
                    st.background_color = color::white;
                    st.text_color = color::black;
                    m_ui.text("enter a key", st);
                } else {
                    m_ui.button_anim(SDL_GetScancodeName(input.keybindings[action_id]), &m_remap_buttons[action_id], st, alt_st, [&, action_id]() {
                        m_remapping_action = (Input::ActionID)action_id;
                    });
                }
                m_ui.end_row();
            }
            m_ui.end_row();
        }

        m_ui.end_row();

        m_ui.end_row();

    }

    m_ui.end_frame();

    m_ui.draw(renderer);
}
