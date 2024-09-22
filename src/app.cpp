#include <chrono>
#include <filesystem>
#include <ratio>
#include <tracy/Tracy.hpp>

#include "app.h"
#include "constants.h"
#include "font.h"

#include "editor.h"
#include "game.h"
#include "main_menu.h"

#include "systems.h"

#include "assets.h"
#include "serialize.h"

#include "ui_test.h"

using namespace constants;

void create_dirs() {
    ZoneScoped;
    std::filesystem::create_directory(maps_directory);
    std::filesystem::create_directory("temp");
}

namespace app {

enum class Context {
    Menu,
    Editor,
    Game,
    UI_Test,
};

int run() {
    create_dirs();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        SDL_Log("SDL_Init failed (%s)", SDL_GetError());
        return 1;
    }

    SDL_Window* window;
    SDL_Renderer* renderer;

    SDL_CreateWindowAndRenderer("taiko", window_width, window_height, 0, &window, &renderer);
    SDL_SetWindowFullscreen(window, true);

    Input input{};
    Audio audio{};

    Mix_MasterVolume(MIX_MAX_VOLUME * 0.3);
    Mix_VolumeMusic(MIX_MAX_VOLUME * 0.2);

    init_font(renderer);

    std::vector<SoundLoadInfo> sound_list = {
        {"don.wav", SoundID::don},
        {"kat.wav", SoundID::kat},
    };

    std::vector<ImageLoadInfo> image_list = {
        {"hit_effect_ok.png", ImageID::hit_effect_ok},
        {"hit_effect_perfect.png", ImageID::hit_effect_perfect},
        {"circle-overlay.png", ImageID::circle_overlay},
        {"circle-select.png", ImageID::select_circle},
        {"circle.png", ImageID::kat_circle, RGBA{60, 219, 226, 255}},
        {"circle.png", ImageID::don_circle, RGBA{252, 78, 60, 255}},
        {"drum-inner.png", ImageID::inner_drum},
        {"drum-outer.png", ImageID::outer_drum},
    };

    AssetLoader assets{};
    assets.init(renderer, image_list, sound_list);

    EventQueue event_queue{};

    Systems systems{renderer, input, audio, assets, event_queue};

    std::vector<Context> context_stack{Context::Menu};

    std::unique_ptr<Editor> editor{};
    std::unique_ptr<Game> game{};
    std::unique_ptr<MainMenu> menu{
        std::make_unique<MainMenu>(renderer, input, audio, assets, event_queue)
    };

    UI_Test ui_test{input};

    SDL_StartTextInput(window);

    UI frame_time_ui{constants::window_width, constants::window_height};


    auto last_frame_start = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> last_frame_duration{};


    while (1) {
        auto frame_start = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> delta_time = frame_start - last_frame_start;

        if (delta_time < 1ms) {
            continue;
        }

        ZoneNamedN(var, "update", true);

        last_frame_start = frame_start;

        bool quit = false;

        {
            float total_wheel{};
            bool moved{};

            input.begin_frame();
            input.input_text = std::nullopt;

            SDL_Event event;
            ZoneNamedN(var, "poll input", true);
            while (SDL_PollEvent(&event)) {
                ZoneNamedN(var, "event", true);
                if (event.type == SDL_EVENT_QUIT) {
                    quit = true;
                    break;
                }

                if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                    total_wheel += event.wheel.y;
                }

                if (event.type == SDL_EVENT_KEY_DOWN) {
                    input.keyboard_repeat[event.key.scancode] = true;
                }

                if (event.type == SDL_EVENT_TEXT_INPUT) {
                    input.input_text = std::string(event.text.text);
                }
            }
            input.wheel = total_wheel;
        }
        if (quit) {
            break;
        }

        EventUnion event_union;
        while (event_queue.pop_event(&event_union)) {
            switch (event_union.index()) {
            case EventType::TestMap:
                game =
                    std::make_unique<Game>(systems, game::InitConfig{false, true}, std::move(editor->copy_map()));
                context_stack.push_back(Context::Game);
                break;
            case EventType::QuitTest:
                game = {};
                audio.set_position(editor->last_pos);
                audio.pause();
                context_stack.pop_back();
                game.reset();
                break;
            case EventType::EditNewMap:
                context_stack.push_back(Context::Editor);
                editor = std::make_unique<Editor>(renderer, input, audio, assets, event_queue);
                editor->creating_map = true;
                break;
            case EventType::EditMap: {
                auto& event = std::get<Event::EditMap>(event_union);
                context_stack.push_back(Context::Editor);
                editor = std::make_unique<Editor>(renderer, input, audio, assets, event_queue);
                editor->load_mapset(event.map_directory);
            } break;
            case EventType::PlayMap: {
                auto& event = std::get<Event::PlayMap>(event_union);
                context_stack.push_back(Context::Game);
                Map map;
                load_binary(map, event.mapset_directory / event.map_filename);
                auto music_file = find_music_file(event.mapset_directory);
                if (music_file.has_value()) {
                    audio.load_music(music_file.value().string().data());
                }
                game = std::make_unique<Game>(systems, game::InitConfig{}, map);
            } break;
            case EventType::Return:
                switch (context_stack.back()) {
                case Context::Editor:
                    editor->quit();
                    editor.reset();
                    break;
                case Context::Game:
                    game.reset();
                    audio.stop();
                    break;
                }
                context_stack.pop_back();
                break;
            }
        }


        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        switch (context_stack.back()) {
        case Context::Menu:
            menu->update(delta_time.count());
            break;
        case Context::Game:
            game->update(delta_time);
            break;
        case Context::Editor:
            editor->update(delta_time);
            break;
        case Context::UI_Test:
            ui_test.update(delta_time.count());
            break;
        }

        auto frame_time_string = std::format("{:.3f} ms", std::chrono::duration<double,std::milli>(last_frame_duration).count());

        auto st = Style{};
        st.text_color = color::white;
        st.position = Position::Anchor{{1, 1}};
        // st.padding = Padding{10,10,10,10};
        frame_time_ui.begin_group(st);
        frame_time_ui.text(frame_time_string.data(), st);
        frame_time_ui.end_group();
        frame_time_ui.end_frame();

        frame_time_ui.draw(renderer);

        // if (input.key_down(SDL_SCANCODE_F)) {
        //     std::cout << std::format("{}\n", frame_time_string);
        // }

        {
            SDL_RenderPresent(renderer);
            ZoneNamedN(v, "Render Present", true);
        }


        // case Context::Editor:
        //   editor->update(delta_time);
        //   break;
        // case Context::Game:
        //   game->update(delta_time);
        //   break;

        last_frame_duration = std::chrono::high_resolution_clock::now() - frame_start;

        input.end_frame();

        FrameMark;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}

} // namespace app
