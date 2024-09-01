#include <tracy/Tracy.hpp>
#include <filesystem>

#include "app.h"
#include "constants.h"
#include "font.h"
#include "systems.h"
#include "editor.h"
#include "game.h"
#include "main_menu.h"

#include "serialize.h"

using namespace constants;

void create_dirs() {
    ZoneScoped;
    std::filesystem::create_directory(maps_directory);
}

namespace app {

enum class Context {
    Menu,
    Editor,
    Game,
};

int run() {
    create_dirs();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        SDL_Log("SDL_Init failed (%s)", SDL_GetError());
        return 1;
    }

    SDL_Window* window;
    SDL_Renderer* renderer;

    SDL_CreateWindowAndRenderer("", window_width, window_height, 0, &window, &renderer);
    SDL_SetWindowFullscreen(window, true);

    Input input{};
    Audio audio{};
    

    Mix_MasterVolume(MIX_MAX_VOLUME * 0.3);
    Mix_VolumeMusic(MIX_MAX_VOLUME * 0.2);


    init_font(renderer);

    std::vector<SoundLoadInfo> sound_list = {
        {"don.wav", "don"},
        {"kat.wav", "kat"},
    };

    std::vector<ImageLoadInfo> image_list = {
        {"hit_effect_ok.png", "hit_effect_ok"},
        {"hit_effect_perfect.png", "hit_effect_perfect"},
        {"circle-overlay.png", "circle_overlay"},
        {"circle-select.png", "select_circle"},
        {"circle.png", "kat_circle", RGBA{ 60, 219, 226, 255 }},
        {"circle.png", "don_circle", RGBA{ 252, 78, 60, 255 }},
        {"drum-inner.png", "inner_drum"},
        {"drum-outer.png", "outer_drum"},
    };

    AssetLoader assets{};
    assets.init(renderer, image_list, sound_list);

    EventQueue event_queue{};

    Systems systems{
        renderer,
        input,
        audio,
        assets,
        event_queue
    };

    std::vector<Context> context_stack{ Context::Menu };

    std::unique_ptr<Editor> editor{};
    std::unique_ptr<Game> game{};
    std::unique_ptr<MainMenu> menu{ std::make_unique<MainMenu>(renderer, input, audio, assets, event_queue) };

    SDL_StartTextInput(window);

    auto last_frame = std::chrono::high_resolution_clock::now();
    while (1) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> delta_time = now - last_frame;

        if (delta_time < 1ms) {
            continue;
        }

        ZoneNamedN(var, "update", true);

        last_frame = now;

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
                game = std::make_unique<Game>(systems, game::InitConfig{}, editor->m_map);
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
            }
            break;
            case EventType::PlayMap: {
                auto& event = std::get<Event::PlayMap>(event_union);
                context_stack.push_back(Context::Game);
                Map map;
                load_binary(map, event.mapset_directory / event.map_filename);
                audio.load_music((event.mapset_directory / std::string("audio.mp3")).string().data());
                game = std::make_unique<Game>(systems, game::InitConfig{}, map);
            }
            break;
            case EventType::Return:
                switch (context_stack.back()) {
                case Context::Editor:
                    editor->quit();
                    editor.reset();
                    break;
                case Context::Game:
                    game.reset();
                    break;
                }
                context_stack.pop_back();
                break;
            }
        }

        switch (context_stack.back()) {
        case Context::Menu:
            menu->update();
            break;
        case Context::Editor:
            editor->update(delta_time);
            break;
        case Context::Game:
            game->update(delta_time);
            break;
        }

        input.end_frame();

        FrameMark;
    }


    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
}

}
