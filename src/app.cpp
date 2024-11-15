#include <chrono>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <ratio>
#include <string>
#include <tracy/Tracy.hpp>

#include "app.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_video.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "allocator.h"
#include "asset_loader.h"
#include "audio.h"
#include "constants.h"
#include "events.h"
#include "font.h"

#include "editor.h"
#include "game.h"
#include "input.h"
#include "main_menu.h"

#include "memory.h"
#include "systems.h"
#include "assets.h"
#include "ui.h"
#include "ui_test.h"

#include <vector>

void* operator new(std::size_t count)
{
    auto ptr = malloc(count);
    TracyAlloc (ptr , count);
    return ptr;
}

void operator delete(void* ptr) noexcept
{
    TracyFree (ptr);
    free(ptr);
}

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

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init failed (%s)", SDL_GetError());
        return 1;
    }

    auto display = SDL_GetPrimaryDisplay();
    auto display_info = SDL_GetDesktopDisplayMode(display);

    constants::window_width = display_info->w;
    constants::window_height = display_info->h;

    SDL_Window* window;
    SDL_Renderer* renderer;

    SDL_CreateWindowAndRenderer("taiko", window_width, window_height,
    SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_FULLSCREEN,
    &window, &renderer);

    SDL_GetRenderDriver(0);

    Input::Input input{};
    input.init_keybinds(Input::default_keybindings);
    Audio audio{};

    Mix_AllocateChannels(32);

    Mix_MasterVolume(effect_volume);
    Mix_VolumeMusic(MIX_MAX_VOLUME * 0.2f);

    Font2::init_fonts(renderer);

    std::vector<SoundLoadInfo> sound_list = {
        {"don.wav", SoundID::don},
        {"kat.wav", SoundID::kat},

        {"menu_select.wav", SoundID::menu_select},
        {"menu_confirm.wav", SoundID::menu_confirm},
        {"menu_back.wav", SoundID::menu_back},
    };

    std::vector<ImageLoadInfo> image_list = {
        {"hit_effect_ok.png", ImageID::hit_effect_ok},
        {"hit_effect_perfect.png", ImageID::hit_effect_perfect},
        {"circle-overlay.png", ImageID::circle_overlay},
        {"circle-select.png", ImageID::select_circle},
        {"circle.png", ImageID::kat_circle, RGBA{60, 219, 226, 255}},
        {"circle.png", ImageID::don_circle, RGBA{252, 78, 60, 255}},
        {"drum_inner.png", ImageID::inner_drum},
        {"drum_outer.png", ImageID::outer_drum},
        {"circle.png", ImageID::crosshair_fill, RGBA{59, 59, 59, 255}},
        {"approach_circle.png", ImageID::crosshair_rim, RGBA{110, 110, 110, 255}},
        {"approach_circle.png", ImageID::crosshair_rim_outer, RGBA{59, 59, 59, 255}},

        {"bg.png", ImageID::bg},
        {"bar-left.png", ImageID::back_frame},
    };

    AssetLoader assets{};
    assets.init(renderer, image_list, sound_list);

    Mix_VolumeChunk(assets.get_sound(SoundID::kat), MIX_MAX_VOLUME * 0.7);
    Mix_VolumeChunk(assets.get_sound(SoundID::don), MIX_MAX_VOLUME * 0.7);

    EventQueue event_queue{};



    auto everything_buffer = BufferOwned(5_MiB);
    auto everything_allocator = linear_allocator{};
    everything_allocator.init(everything_buffer.handle());

    MemoryAllocators memory = {.ui_allocator = linear_allocator{}};
    memory.ui_allocator.init(everything_allocator.allocate_buffer(2_MiB, sizeof(std::max_align_t)));

    linear_allocator debug_ui_allocator{};
    debug_ui_allocator.init(everything_allocator.allocate_buffer(1_MiB, sizeof(std::max_align_t)));


    Systems systems{renderer, memory, input, audio, assets, event_queue};

    std::unique_ptr<Editor> editor{};
    std::unique_ptr<game::Game> game{};
    std::unique_ptr<MainMenu> menu{
        std::make_unique<MainMenu>(memory, renderer, input, audio, assets, event_queue)
    };

    UI_Test ui_test{memory, renderer, input};

    // SDL_StartTextInput(window);
    


    auto last_frame_start = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> last_frame_duration{};

    std::vector<Context> context_stack{Context::Menu};
    menu->awake();


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
                    input.m_key_this_frame = event.key.scancode;
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

        std::filesystem::path mapset_directory;
        std::string map_filename;


        EventUnion event_union;
        while (event_queue.pop_event(&event_union)) {
            switch (event_union.index()) {
            case EventType::TestMap:
                // game = std::make_unique<game::Game>(systems, game::InitConfig{false, true}, std::move(editor->copy_map()));
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
                editor = std::make_unique<Editor>(memory, renderer, input, audio, assets, event_queue);
                editor->creating_map = true;
                break;
            case EventType::EditMap: {
                auto& event = std::get<Event::EditMap>(event_union);
                context_stack.push_back(Context::Editor);
                editor = std::make_unique<Editor>(memory, renderer, input, audio, assets, event_queue);
                editor->load_mapset(event.map_directory);
            } break;
            case EventType::PlayMap: {
                auto& event = std::get<Event::PlayMap>(event_union);
                context_stack.push_back(Context::Game);
                game = std::make_unique<game::Game>(systems, game::InitConfig{event.mapset_directory, event.map_filename});
            } break;
            case EventType::GameReset: {
                auto init_config = game->config;
                game = std::make_unique<game::Game>(systems, init_config);
                

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
                    menu->play_selected_music();
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

        UI debug_ui(debug_ui_allocator);

        auto frame_time_string = std::format("{:.3f} ms", std::chrono::duration<double,std::milli>(last_frame_duration).count());
        auto ui_memory_string = std::format("UI: {:.3f} / {:.3f} MB", (float)memory.ui_allocator.m_current / (float)1_MiB, (float) memory.ui_allocator.m_capacity / (float)1_MiB );

        auto st = Style{};
        st.position = Position::Anchor{{1, 1}};
        st.padding = Padding{10,10,10,10};
        st.stack_direction = StackDirection::Vertical;
        st.align_items = Alignment::End;

        debug_ui.begin_frame(constants::window_width, constants::window_height);

        debug_ui.begin_row(st);

        st = {};
        st.text_color = color::yellow;
        debug_ui.text(ui_memory_string.data(), st);
        debug_ui.text(frame_time_string.data(), st);
        debug_ui.end_row();

        debug_ui.end_frame(input);

        debug_ui.draw(renderer);

        {
            ZoneNamedN(v, "SDL_RenderPresent", true);
            SDL_RenderPresent(renderer);
        }

        last_frame_duration = std::chrono::high_resolution_clock::now() - frame_start;

        input.end_frame();

        debug_ui_allocator.clear();
        memory.ui_allocator.clear();

        FrameMark;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}

} // namespace app
