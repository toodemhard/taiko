#include "game.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_scancode.h"
#include "SDL3_mixer/SDL_mixer.h"
#include "constants.h"
#include "assets.h"
#include "events.h"
#include "input.h"
#include "map.h"
#include "serialize.h"
#include "ui.h"
#include "vec.h"
#include <cstdint>

using namespace std::chrono_literals;

constexpr double input_indicator_duration = 0.1;
constexpr std::chrono::duration<float> end_screen_delay = 1s;

using namespace constants;

Vec2 rect_center(const SDL_FRect& rect) {
    return { rect.x + rect.w / 2, rect.y + rect.h / 2 };
}

SDL_FRect rect_at_center_point(Vec2 center, float width, float height) {
    return { center.x - width / 2.0f, center.y - width / 2.0f, width, height };
}

float cerp(float a, float b, float t) {
    t -= 1.0f;
    t = t*t*t + 1.0f;
    return a + (b - a) * t;
}

constexpr float normal_scale = 0.9f;
constexpr float big_scale = 1.3333f;

Vec2 Cam::world_to_screen(const Vec2& point) const {
    Vec2 relative_pos = {point.x - position.x, position.y - point.y};
    Vec2 normalized = relative_pos / bounds + Vec2::one() * 0.5f; // 0 to 1
        
    return Vec2 {
        normalized.x * window_width,
        normalized.y * window_height
    };
}
Vec2 Cam::world_to_screen_scale(const Vec2& scale) const {
    return { world_to_screen_scale(scale.x), world_to_screen_scale(scale.y) };
}

float Cam::world_to_screen_scale(const float& length) const {
    return length / bounds.y * window_height;

}

Vec2 Cam::screen_to_world(const Vec2& point) const {
    Vec2 normalized = { point.x / window_width, point.y / window_height };
    Vec2 relative_pos = (normalized - Vec2::one() * 0.5f) * bounds;
    return { relative_pos.x + position.x, position.y - relative_pos.y };
}

namespace game {
void Game::draw_map() {
    ZoneScoped;

    int right = m_map.times.size() - 1;
    int left = 0;
    constexpr float circle_padding = 0.2f;
    float right_bound = cam.position.x + cam.bounds.x / 2 + circle_padding;
    float left_bound = cam.position.x - (cam.bounds.x / 2 + circle_padding);

    for (int i = current_note_index; i < m_map.times.size(); i++) {
        if (m_map.times[i] >= right_bound) {
            right = i - 1;
            break;
        }
    }

    for (int i = current_note_index; i >= 0; i--) {
        if (i >= m_map.times.size()) {
            continue;
        }
        if (m_map.times[i] <= left_bound) {
            left = i + 1;
            break;
        }
    }

    for (int i = right; i >= left; i--) {
        if (note_alive_list[i] == false) {
            continue;
        }

        Vec2 center_pos = cam.world_to_screen({ (float)m_map.times[i], 0 });

        float scale = (m_map.flags_list[i] & NoteFlagBits::small) ? normal_scale : big_scale;
        Image circle_image = (m_map.flags_list[i] & NoteFlagBits::don) ? assets.get_image(ImageID::don_circle) : assets.get_image(ImageID::kat_circle);
        Image circle_overlay = assets.get_image(ImageID::circle_overlay);
        Image select_circle = assets.get_image(ImageID::select_circle);

        Vec2 circle_pos = center_pos;
        circle_pos.x -= circle_image.width / 2.0f * scale;
        circle_pos.y -= circle_image.height / 2.0f * scale;

        {
            SDL_FRect rect = { circle_pos.x, circle_pos.y, circle_image.width * scale, circle_image.height * scale };
            SDL_RenderTexture(renderer, circle_image.texture, NULL, &rect);
            SDL_RenderTexture(renderer, circle_overlay.texture, NULL, &rect);
        }

    }
}

void draw_note(SDL_Renderer* renderer, AssetLoader& assets, const NoteFlags& note_type, const Vec2& center_point) {
        float scale = (note_type & NoteFlagBits::small) ? normal_scale : big_scale;
        Image circle_image = (note_type & NoteFlagBits::don) ? assets.get_image(ImageID::don_circle) : assets.get_image(ImageID::kat_circle);
        Image circle_overlay = assets.get_image(ImageID::circle_overlay);
        Image select_circle = assets.get_image(ImageID::select_circle);

        Vec2 circle_pos = center_point;
        circle_pos.x -= circle_image.width / 2.0f * scale;
        circle_pos.y -= circle_image.height / 2.0f * scale;

        {
            SDL_FRect rect = { circle_pos.x, circle_pos.y, circle_image.width * scale, circle_image.height * scale };
            SDL_RenderTexture(renderer, circle_image.texture, NULL, &rect);
            SDL_RenderTexture(renderer, circle_overlay.texture, NULL, &rect);
        }
}

Game::Game(Systems systems, game::InitConfig config) :
    renderer{ systems.renderer },
    input{ systems.input }, 
    audio{ systems.audio },
    assets{ systems.assets },
    event_queue{ systems.event_queue },
    m_auto_mode{ config.auto_mode },
    m_test_mode{ config.test_mode },
    config{config}
{}

const static double min_buffer_duration = 1;

void Game::start() {
    load_binary(m_map, config.mapset_directory / config.map_filename);
    note_alive_list = std::vector<bool>(m_map.times.size(), true);
    auto music_file = find_music_file(config.mapset_directory);
    if (music_file.has_value()) {
        audio.load_music(music_file.value().string().data());
    }
    // audio.resume();
    if (m_map.times[current_note_index] < min_buffer_duration)  {
        m_buffer_elapsed = m_map.times[current_note_index] - min_buffer_duration;

    } else {
        audio.play(0);
        m_audio_started = true;
    }

    SDL_HideCursor();
}


void Game::update(std::chrono::duration<double> delta_time) {
    if (!initialized) {
        this->start();
        initialized = true;
    }

    double elapsed{};
    if (!m_audio_started) {
        if (m_buffer_elapsed >= 0) {
            audio.play(0);
            m_audio_started = true;
        } else {

            if (m_view != View::paused) {
                m_buffer_elapsed += delta_time.count();
            }
            elapsed = m_buffer_elapsed;
        }
    }

    if (m_audio_started) {
        elapsed = audio.get_position();
    }

    ui.input(input);
    ui.begin_frame(constants::window_width, constants::window_height);

    if (input.key_down(SDL_SCANCODE_L)) {
        m_view = View::end_screen;
        SDL_ShowCursor();
    }

    if (input.key_down(SDL_SCANCODE_ESCAPE)) {
        SDL_ShowCursor();
        if (m_test_mode) {
            event_queue.push_event(Event::QuitTest{});
        }
    }


    if (m_view == View::main) {
        if (input.key_down(SDL_SCANCODE_ESCAPE)) {
            audio.pause();
            Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_back), 0);
            m_view = View::paused;
        }


        double last_note_time = (m_map.times.size() == 0) ? 0 : m_map.times.back();
        if (elapsed >= last_note_time + end_screen_delay.count()) {
            if (m_test_mode) {
                event_queue.push_event(Event::QuitTest{});               
            } else {
                m_view = View::end_screen;
                SDL_ShowCursor();
            }
        }

        std::vector<DrumInput> inputs{};

        if (m_auto_mode) {
            if (current_note_index < m_map.times.size()) {
                if (elapsed >= m_map.times[current_note_index]) {
                    if (m_map.flags_list[current_note_index] & NoteFlagBits::don) {
                        input_history.push_back(InputRecord{ DrumInput::don_left, elapsed });
                        inputs.push_back(DrumInput::don_left);
                    }
                    else {
                        input_history.push_back(InputRecord{ DrumInput::kat_left, elapsed });
                        inputs.push_back(DrumInput::kat_left);
                    }
                }
            }
        }

        if (input.action_down(Input::ActionID::don_left)) {
            input_history.push_back(InputRecord{ DrumInput::don_left, elapsed });
            inputs.push_back(DrumInput::don_left);
        }

        if (input.action_down(Input::ActionID::don_right)) {
            input_history.push_back(InputRecord{ DrumInput::don_right, elapsed });
            inputs.push_back(DrumInput::don_right);
        }

        if (input.action_down(Input::ActionID::kat_left)) {
            input_history.push_back(InputRecord{ DrumInput::kat_left, elapsed });
            inputs.push_back(DrumInput::kat_left);
        }

        if (input.action_down(Input::ActionID::kat_right)) {
            input_history.push_back(InputRecord{ DrumInput::kat_right, elapsed });
            inputs.push_back(DrumInput::kat_right);
        }

        if (elapsed - hit_effect_time_point_seconds > hit_effect_duration.count()) {
            m_current_hit_effect = hit_effect::none;
        }

        auto update_accuracy = [&]() {
            accuracy_fraction = (perfect_accuracy_weight * perfect_count + ok_accuracy_weight * ok_count + miss_accuracy_weight * miss_count) / (current_note_index);
        };


        // bool big_note_sound_played = false;
        if (current_note_index < m_map.times.size()) {
            for (const auto& input : inputs) {
                auto error_duration = elapsed - m_map.times[current_note_index];
                if (std::abs(error_duration) <= ok_range.count() / 2) {
                    auto actual_type = (uint8_t)(m_map.flags_list[current_note_index] & NoteFlagBits::don);
                    auto input_type = (uint8_t)(input & DrumInputFlagBits::don_kat);
                    if (actual_type == input_type) {
                        note_alive_list[current_note_index] = false;
                        hit_effect_time_point_seconds = elapsed;
                        combo++;
                        if (error_duration <= perfect_range.count() / 2) {
                            m_current_hit_effect = hit_effect::perfect;
                            score += 300;
                            perfect_count++;
                        }
                        else {
                            m_current_hit_effect = hit_effect::ok;
                            score += 100;
                            ok_count++;
                        }

                        in_flight_notes.times.push_back(elapsed);
                        in_flight_notes.flags.push_back(m_map.flags_list[current_note_index]);
                    }
                    else {
                        combo = 0;
                        miss_count++;

                        m_miss_effects.push_back(elapsed);
                    }
                    current_note_index++;

                    update_accuracy();
                }
            }
        }

        // if(!big_note_sound_played) {
        for (const auto& input : inputs) {
            if (input == DrumInput::don_left || input == DrumInput::don_right) {
                Mix_PlayChannel(-1, assets.get_sound(SoundID::don), 0);
            }
            else if (input == DrumInput::kat_left || input == DrumInput::kat_right) {
                Mix_PlayChannel(-1, assets.get_sound(SoundID::kat), 0);
            }
        }

        if (current_note_index < m_map.times.size()) {
            // if current note passed by completely without input attempts
            if (elapsed - m_map.times[current_note_index] > ok_range.count() / 2) {
                combo = 0;
                miss_count++;

                m_miss_effects.push_back(elapsed);

                current_note_index++;
                update_accuracy();
            }
        }

        for (int i = 0; i < in_flight_notes.times.size(); i++) {
            auto flight_elapsed = elapsed - in_flight_notes.times[i];

            if (flight_elapsed <= total_flight_time_seconds) {
                break;
            }

            in_flight_notes.times.erase(in_flight_notes.times.begin());
            in_flight_notes.flags.erase(in_flight_notes.flags.begin());
        }

        {
            float height = 220;
            auto rect = SDL_FRect{0, (constants::window_height - height) / 2.0f, (float)constants::window_width, height};
            SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
            SDL_RenderFillRect(renderer, &rect);
        }

        float thing_x = 30;
        float thing_scale = 1.25;
        auto back_frame = assets.get_image(ImageID::back_frame);
        float fix_scale = 1.055;
        back_frame.width *= fix_scale * thing_scale;
        back_frame.height *= fix_scale * thing_scale;

        auto inner_drum = assets.get_image(ImageID::inner_drum);
        inner_drum.width *= thing_scale;
        inner_drum.height *= thing_scale;
        auto outer_drum = assets.get_image(ImageID::outer_drum);
        outer_drum.width *= thing_scale;
        outer_drum.height *= thing_scale;
        auto crosshair_rim = assets.get_image(ImageID::crosshair_rim);

        float crosshair_x = inner_drum.width * 2 + crosshair_rim.width / 2.0f + 100;
        auto crosshair_rect = SDL_FRect{ crosshair_x - crosshair_rim.width / 2.0f, (window_height - crosshair_rim.height) / 2.0f, (float)crosshair_rim.width, (float)crosshair_rim.height };
        SDL_RenderTexture(renderer, crosshair_rim.texture, NULL, &crosshair_rect);

        // auto crosshair_rim_outer = assets.get_image(ImageID::crosshair_rim_outer);
        // crosshair_rim_outer.width *= 1.3;
        // crosshair_rim_outer.height *= 1.3;
        // auto dst_rect = SDL_FRect{
        //     crosshair_x - crosshair_rim_outer.width / 2.0f,
        //     (window_height - crosshair_rim_outer.height) / 2.0f,
        //     (float)crosshair_rim_outer.width, 
        //     (float)crosshair_rim_outer.height
        // };
        // SDL_RenderTexture(renderer, crosshair_rim_outer.texture, NULL, &dst_rect);


        auto crosshair_fill = assets.get_image(ImageID::crosshair_fill);
        crosshair_fill.width *= 0.9;
        crosshair_fill.height *= 0.9;
        {
            auto dst_rect = SDL_FRect{crosshair_x - crosshair_fill.width / 2.0f, (window_height - crosshair_fill.height) / 2.0f, (float)crosshair_fill.width, (float)crosshair_fill.height};
            SDL_RenderTexture(renderer, crosshair_fill.texture, NULL, &dst_rect);
        }

        if (m_current_hit_effect == hit_effect::perfect) {
            auto hit_effect_image = assets.get_image(ImageID::hit_effect_perfect);
            auto dst_rect = rect_at_center_point(rect_center(crosshair_rect), hit_effect_image.width, hit_effect_image.height);
            SDL_RenderTexture(renderer, hit_effect_image.texture, NULL, &dst_rect);
        }
        else if (m_current_hit_effect == hit_effect::ok) {
            auto hit_effect_image = assets.get_image(ImageID::hit_effect_ok);
            auto dst_rect = rect_at_center_point(rect_center(crosshair_rect), hit_effect_image.width, hit_effect_image.height);
            SDL_RenderTexture(renderer, hit_effect_image.texture, NULL, &dst_rect);
        }
        
        cam.position.x = elapsed;
        // find world space dist between crosshair and 0 time point
        auto cam_offset = elapsed - cam.screen_to_world({ crosshair_rect.x + crosshair_rect.w / 2, 0 }).x;

        cam.position.x += cam_offset;

        this->draw_map();

        {
            auto rect = SDL_FRect{0, (constants::window_height - back_frame.height) / 2.0f, thing_x, (float)back_frame.height};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(renderer, &rect);

            float y = (constants::window_height - back_frame.height) / 2.0f + 2;
            auto dst_rect = SDL_FRect{thing_x, y, (float)back_frame.width, (float)back_frame.height};
            SDL_RenderTexture(renderer, back_frame.texture, NULL, &dst_rect);
        }

        float x = elapsed;
        Vec2 p1 = cam.world_to_screen({ x, 0.25f });
        Vec2 p2 = cam.world_to_screen({ x, -0.25f });


        auto flight_start_point = rect_center(crosshair_rect);
        for (int i = 0; i < in_flight_notes.times.size(); i++) {
            auto flight_elapsed = elapsed - in_flight_notes.times[i];

            auto pos = linear_interp({ flight_start_point.x, flight_start_point.y }, { (float)constants::window_width, 0 }, flight_elapsed / total_flight_time_seconds);

            draw_note(renderer, assets, in_flight_notes.flags[i], pos);
        }


        constexpr auto miss_effect_duration = 0.4f;

        for (int i = m_miss_effects.size() - 1; i >= 0; i--) {
            if (elapsed - m_miss_effects[i] > miss_effect_duration) {
                m_miss_effects.erase(m_miss_effects.begin() + i);
            }
        }
    
        for (auto effect : m_miss_effects) {
            auto width = 20.0f;
            auto height = 20.0f;

            auto y = linear_interp(0, 100, (elapsed - effect) / miss_effect_duration) + 120;
            uint8_t a = linear_interp(255, 0, (elapsed - effect) / miss_effect_duration);

            Style st{};
            st.position = Position::Absolute{crosshair_x,  (constants::window_height - height) / 2.0f - y};

            ui.begin_row(st);
            ui.text("X", {.position = Position::Anchor{0.5, 0.5}, .font_size=68, .text_color=RGBA{255, 60, 60, a}});
            ui.end_row();
        }


        const Vec2 drum_pos = { thing_x, ((float)window_height - inner_drum.height) / 2 };
        auto left_rect = SDL_FRect{ drum_pos.x, drum_pos.y, (float)inner_drum.width, (float)inner_drum.height };
        auto right_rect = SDL_FRect{ drum_pos.x + inner_drum.width, drum_pos.y, (float)inner_drum.width, (float)inner_drum.height };

        for (int i = input_history.size() - 1; i >= 0; i--) {
            const InputRecord& input = input_history[i];
            if (elapsed - input.time > input_indicator_duration) {
                break;
            }

            switch (input.type) {
            case DrumInput::don_left:
                SDL_RenderTexture(renderer, inner_drum.texture, NULL, &left_rect);
                break;
            case DrumInput::don_right:
                SDL_RenderTextureRotated(renderer, inner_drum.texture, NULL, &right_rect, 0, NULL, SDL_FLIP_HORIZONTAL);
                break;
            case DrumInput::kat_left:
                SDL_RenderTextureRotated(renderer, outer_drum.texture, NULL, &left_rect, 0, NULL, SDL_FLIP_HORIZONTAL);
                break;
            case DrumInput::kat_right:
                SDL_RenderTexture(renderer, outer_drum.texture, NULL, &right_rect);
                break;
            }
        }

        Style style{};
        style.position = Position::Anchor{ 1,0 };
        style.stack_direction = StackDirection::Vertical;
        style.align_items = Alignment::End;

        ui.begin_row(style);
        ui.text(ui.strings.add(std::format("{}", score)), {.font_size=54 });

        ui.text(ui.strings.add(std::format("{:.2f}%", accuracy_fraction * 100)), {});
        ui.end_row();

        ui.begin_row(Style{ Position::Anchor{0,1} });
        ui.text(ui.strings.add(std::format("{}x", combo)), {.font_size=48});
        ui.end_row();


    } else if (m_view == View::paused) {
        const char* texts[] = {
            "Resume",
            "Retry",
            "Quit",
        };

        std::function<void()> lambdas[] = {
            [&]() {
                m_view = View::main;
                audio.resume();
                Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_confirm), 0);
            },
            [&]() {
                event_queue.push_event(Event::GameReset{});
                Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_confirm), 0);
            },
            [&]() {
                event_queue.push_event(Event::Return{});
                Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_confirm), 0);
            }
        };

        if (input.key_down(SDL_SCANCODE_ESCAPE)) {
            lambdas[0]();
            Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_back), 0);
        }

        if (input.key_down(SDL_SCANCODE_RETURN)) {
            lambdas[m_paused_selected_option]();
        }

        if (input.key_down(SDL_SCANCODE_LEFT) && m_paused_selected_option > 0) {
            m_paused_selected_option--;
            Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_select), 0);
        }

        if (input.key_down(SDL_SCANCODE_RIGHT) && m_paused_selected_option < 2) {
            m_paused_selected_option++;
            Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_select), 0);
        }

        Style st{};
        st.stack_direction = StackDirection::Vertical;
        st.position = Position::Anchor{0.5, 0.5};
        st.gap = 40;


        ui.begin_row(st); {
            for (int i = 0; i < 3; i++) {
                st = {};
                st.width = Scale::Fixed{400};
                st.padding = even_padding(30);
                st.font_size = 54;
                st.text_align = TextAlign::Center;
                if (i == m_paused_selected_option) {
                    st.background_color = color::white;
                    st.text_color = color::black;
                    ui.button(texts[i], st, std::move(lambdas[i]));
                } else {
                    AnimStyle anim_st{};
                    anim_st.alt_background_color = color::bg_highlight;
                    ui.button_anim(texts[i], &m_pause_menu_buttons[i], st, anim_st, [&, i]() {
                        m_paused_selected_option = i;
                        Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_select), 0);
                    });
                }

            }
        } ui.end_row();

    } else if (m_view == View::end_screen) {
        if (input.key_down(SDL_SCANCODE_ESCAPE)) {
            SDL_ShowCursor();
            if (m_test_mode) {
                event_queue.push_event(Event::QuitTest{});
                Mix_PlayChannel(-1, assets.get_sound(SoundID::menu_back), 0);
            }
            else {
                event_queue.push_event(Event::Return{});
            }
        }
        Style style{};
        style.stack_direction = StackDirection::Vertical;
        style.width = Scale::FitParent{};
        style.height = Scale::FitParent{};
        style.padding = even_padding(100);
        
        ui.begin_row(style);
        ui.text(ui.strings.add(std::format("{}", score)), {.font_size=56});
        ui.text(ui.strings.add(std::format("{:.2f}%", accuracy_fraction * 100)), {});
        ui.text(ui.strings.add(std::format("{} Perfect", perfect_count)), {});
        ui.text(ui.strings.add(std::format("{} Ok", ok_count)), {});
        ui.text(ui.strings.add(std::format("{} Miss", miss_count)), {});

        ui.button("Back", {.position=Position::Anchor{0, 1}, .font_size=40}, [&]() {
            event_queue.push_event(Event::Return{});
            });
        ui.end_row();
    }

    ui.end_frame();

    ui.draw(renderer);
}
}
