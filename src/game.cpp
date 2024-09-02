#include "game.h"
#include "constants.h"

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

Vec2 linear_interp(Vec2 p1, Vec2 p2, float t) {
    return p1 + (p2 - p1) * t;
}

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

        float scale = (m_map.flags_list[i] & NoteFlagBits::normal_or_big) ? 0.9f : 1.4f;
        Image circle_image = (m_map.flags_list[i] & NoteFlagBits::don_or_kat) ? assets.get_image("don_circle") : assets.get_image("kat_circle");
        Image circle_overlay = assets.get_image("circle_overlay");
        Image select_circle = assets.get_image("select_circle");

        Vec2 circle_pos = center_pos;
        circle_pos.x -= circle_image.width / 2 * scale;
        circle_pos.y -= circle_image.height / 2 * scale;

        {
            SDL_FRect rect = { circle_pos.x, circle_pos.y, circle_image.width * scale, circle_image.height * scale };
            SDL_RenderTexture(renderer, circle_image.texture, NULL, &rect);
            SDL_RenderTexture(renderer, circle_overlay.texture, NULL, &rect);
        }

    }
}

void draw_note(SDL_Renderer* renderer, AssetLoader& assets, const NoteFlags& note_type, const Vec2& center_point) {
        float scale = (note_type & NoteFlagBits::normal_or_big) ? 0.9f : 1.4f;
        Image circle_image = (note_type & NoteFlagBits::don_or_kat) ? assets.get_image("don_circle") : assets.get_image("kat_circle");
        Image circle_overlay = assets.get_image("circle_overlay");
        Image select_circle = assets.get_image("select_circle");

        Vec2 circle_pos = center_point;
        circle_pos.x -= circle_image.width / 2 * scale;
        circle_pos.y -= circle_image.height / 2 * scale;

        {
            SDL_FRect rect = { circle_pos.x, circle_pos.y, circle_image.width * scale, circle_image.height * scale };
            SDL_RenderTexture(renderer, circle_image.texture, NULL, &rect);
            SDL_RenderTexture(renderer, circle_overlay.texture, NULL, &rect);
        }
}

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

Game::Game(Systems systems, game::InitConfig config, Map map) :
    renderer{ systems.renderer },
    input{ systems.input }, 
    audio{ systems.audio },
    assets{ systems.assets },
    event_queue{ systems.event_queue },
    m_map{ map },
    m_auto_mode{ config.auto_mode },
    m_test_mode{ config.test_mode },
    note_alive_list{ std::vector<bool>(m_map.times.size(), true) }
{}

void Game::start() {
    audio.resume();
}

void Game::update(std::chrono::duration<double> delta_time) {
    if (!initialized) {
        this->start();
        initialized = true;
    }

    double elapsed = audio.get_position();

    if (input.key_down(SDL_SCANCODE_ESCAPE)) {
        if (m_test_mode) {
            event_queue.push_event(Event::QuitTest{});
        }
        else {
            event_queue.push_event(Event::Return{});
        }

        return;
    }

    StringCache strings{};

    if (!m_end_screen) {
        if (elapsed >= m_map.times.back() + end_screen_delay.count()) {
            m_end_screen = true;
        }

        std::vector<DrumInput> inputs{};

        if (m_auto_mode) {
            if (current_note_index < m_map.times.size()) {
                if (elapsed >= m_map.times[current_note_index]) {
                    if (m_map.flags_list[current_note_index] & NoteFlagBits::don_or_kat) {
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

        if (input.key_down(SDL_SCANCODE_X)) {
            input_history.push_back(InputRecord{ DrumInput::don_left, elapsed });
            inputs.push_back(DrumInput::don_left);
        }

        if (input.key_down(SDL_SCANCODE_PERIOD)) {
            input_history.push_back(InputRecord{ DrumInput::don_right, elapsed });
            inputs.push_back(DrumInput::don_right);
        }

        if (input.key_down(SDL_SCANCODE_Z)) {
            input_history.push_back(InputRecord{ DrumInput::kat_left, elapsed });
            inputs.push_back(DrumInput::kat_left);
        }

        if (input.key_down(SDL_SCANCODE_SLASH)) {
            input_history.push_back(InputRecord{ DrumInput::kat_right, elapsed });
            inputs.push_back(DrumInput::kat_right);
        }

        for (const auto& input : inputs) {
            if (input == DrumInput::don_left || input == DrumInput::don_right) {
                Mix_PlayChannel(-1, assets.get_sound("don"), 0);
            }
            else if (input == DrumInput::kat_left || input == DrumInput::kat_right) {
                Mix_PlayChannel(-1, assets.get_sound("kat"), 0);
            }
        }

        if (elapsed - hit_effect_time_point_seconds > hit_effect_duration.count()) {
            m_current_hit_effect = hit_effect::none;
        }

        auto update_accuracy = [&]() {
            accuracy_fraction = (perfect_accuracy_weight * perfect_count + ok_accuracy_weight * ok_count + miss_accuracy_weight * miss_count) / (current_note_index);

            };

        if (current_note_index < m_map.times.size()) {
            for (const auto& thing : inputs) {
                auto error_duration = elapsed - m_map.times[current_note_index];
                if (std::abs(error_duration) <= ok_range.count() / 2) {
                    auto actual_type = (uint8_t)(m_map.flags_list[current_note_index] & NoteFlagBits::don_or_kat);
                    auto input_type = (uint8_t)(thing & DrumInputFlagBits::don_kat);
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
                    }
                    current_note_index++;

                    update_accuracy();
                }
            }
        }

        if (current_note_index < m_map.times.size()) {
            // if current note passed by completely without input attempts
            if (elapsed - m_map.times[current_note_index] > ok_range.count() / 2) {
                combo = 0;
                miss_count++;

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

        auto inner_drum = assets.get_image("inner_drum");
        auto outer_drum = assets.get_image("outer_drum");

        auto hit_target = assets.get_image("circle_overlay");
        auto hit_target_rect = SDL_FRect{ inner_drum.width * 2.0f, (window_height - hit_target.height) / 2.0f, (float)hit_target.width, (float)hit_target.height };

        cam.position.x = elapsed;
        // find world space dist between crosshair and 0 time point
        auto cam_offset = elapsed - cam.screen_to_world({ hit_target_rect.x + hit_target_rect.w / 2, 0 }).x;

        cam.position.x += cam_offset;


        Style style{};
        style.anchor = { 1,0 };
        style.stack_direction = StackDirection::Vertical;

        auto f = strings.add("idk");
        auto g = strings.add("wut");

        ui.begin_group(style);
        ui.rect(strings.add(std::format("{}", score)), {});


        //std::cerr << std::format("{}", )

        ui.rect(strings.add(std::format("{:.2f}%", accuracy_fraction * 100)), {});
        ui.end_group();

        ui.begin_group(Style{ {0,1} });
        ui.rect(strings.add(std::format("{}x", combo)), {});
        ui.end_group();


        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        float x = elapsed;
        Vec2 p1 = cam.world_to_screen({ x, 0.25f });
        Vec2 p2 = cam.world_to_screen({ x, -0.25f });

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);

        SDL_RenderTexture(renderer, hit_target.texture, NULL, &hit_target_rect);

        auto flight_start_point = rect_center(hit_target_rect);
        for (int i = 0; i < in_flight_notes.times.size(); i++) {
            auto flight_elapsed = elapsed - in_flight_notes.times[i];

            auto pos = linear_interp({ flight_start_point.x, flight_start_point.y }, { 1920, 0 }, flight_elapsed / total_flight_time_seconds);

            draw_note(renderer, assets, in_flight_notes.flags[i], pos);
        }


        if (m_current_hit_effect == hit_effect::perfect) {
            auto hit_effect_image = assets.get_image("hit_effect_perfect");
            auto dst_rect = rect_at_center_point(rect_center(hit_target_rect), hit_effect_image.width, hit_effect_image.height);
            SDL_RenderTexture(renderer, hit_effect_image.texture, NULL, &dst_rect);
        }
        else if (m_current_hit_effect == hit_effect::ok) {
            auto hit_effect_image = assets.get_image("hit_effect_ok");
            auto dst_rect = rect_at_center_point(rect_center(hit_target_rect), hit_effect_image.width, hit_effect_image.height);
            SDL_RenderTexture(renderer, hit_effect_image.texture, NULL, &dst_rect);
        }

        this->draw_map();

        const Vec2 drum_pos = { 0, (window_height - inner_drum.height) / 2 };
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

    } else {
        Style style{};
        style.anchor = { 0.5, 0.5 };
        style.stack_direction = StackDirection::Vertical;
        
        ui.begin_group_v2(style, {});
        ui.rect(strings.add(std::format("{}", score)), {});
        ui.rect(strings.add(std::format("{:.2f}%", accuracy_fraction * 100)), {});
        ui.rect(strings.add(std::format("{} Perfect", perfect_count)), {});
        ui.rect(strings.add(std::format("{} Ok", ok_count)), {});
        ui.rect(strings.add(std::format("{} Miss", miss_count)), {});
        ui.end_group_v2();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);
    }

    ui.begin_group(Style{ {1,1} });
    auto frame_time = std::to_string(((float)std::chrono::duration_cast<std::chrono::microseconds>(delta_time).count()) / 1000) + " ms";
    ui.rect(frame_time.data(), {});
    ui.end_group();

    ui.draw(renderer);

    SDL_RenderPresent(renderer);
}
