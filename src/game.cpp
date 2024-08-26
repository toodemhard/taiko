#include "game.h"
#include "constants.h"

constexpr double input_indicator_duration = 0.1;

using namespace constants;

void draw_map(SDL_Renderer* renderer, AssetLoader& assets, const Map& map, const Cam& cam, int current_note) {
    ZoneScoped;

    int right = map.times.size() - 1;
    int left = 0;
    constexpr float circle_padding = 0.2f;
    float right_bound = cam.position.x + cam.bounds.x / 2 + circle_padding;
    float left_bound = cam.position.x - (cam.bounds.x / 2 + circle_padding);

    for (int i = current_note; i < map.times.size(); i++) {
        if (map.times[i] >= right_bound) {
            right = i - 1;
            break;
        }
    }
    for (int i = current_note; i >= 0; i--) {
        if (i >= map.times.size()) {
            continue;
        }
        if (map.times[i] <= left_bound) {
            left = i + 1;
            break;
        }
    }

    for (int i = right; i >= left; i--) {
        Vec2 center_pos = cam.world_to_screen({ (float)map.times[i], 0 });

        float scale = (map.flags_vec[i] & NoteFlagBits::normal_or_big) ? 0.9f : 1.4f;
        Image circle_image = (map.flags_vec[i] & NoteFlagBits::don_or_kat) ? assets.get_image("don_circle") : assets.get_image("kat_circle");
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

Game::Game(SDL_Renderer* _renderer, Input& _input, Audio& _audio, AssetLoader& _assets, EventQueue& _event_queue, Map map)
    : renderer{ _renderer }, input{ _input }, audio{ _audio }, assets{ _assets }, event_queue{ _event_queue }, map{ map } {}

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
        event_queue.push_event(Event::QuitTest{});
        return;
    }

    std::vector<DrumInput> inputs{};

    if (auto_mode) {
        if (current_note < map.times.size()) {
            if (elapsed >= map.times[current_note]) {
                if (map.flags_vec[current_note] & NoteFlagBits::don_or_kat) {
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
        if ( input == DrumInput::don_left || input == DrumInput::don_right) {
            Mix_PlayChannel(-1, assets.get_sound("don"), 0);
        } else if (input == DrumInput::kat_left || input == DrumInput::kat_right) {
            Mix_PlayChannel(-1, assets.get_sound("kat"), 0);
        }
    }

    if (current_note < map.times.size()) {
        for (const auto& thing : inputs) {
            double hit_normalized = (elapsed - map.times[current_note]) / hit_range.count() + 0.5;
            if (hit_normalized >= 0 && hit_normalized < 1) {
                if (map.flags_vec[current_note] & NoteFlagBits::normal_or_big) {
                    auto actual_type = (uint8_t)(map.flags_vec[current_note] & NoteFlagBits::don_or_kat);
                    auto input_type = (uint8_t)(thing & DrumInputFlagBits::don_kat);
                    if (actual_type == input_type) {
                        score += 300;
                        combo++;
                        current_note++;
                    }
                    //particles.push_back(Particle{ Vec2{(float)elapsed.count(), 0}, {0,1},1, note.type, elapsed });
                } else {
                    

                }
            }

        }

        //if (elapsed > map[current_note].time - (hit_range / 2)) {
        //    //PlaySound(don_sound);
        //    particles.push_back(Particle{ Vec2{(float)elapsed.count(), 0}, {0,1}, 1, map[current_note].type, elapsed });
        //    current_note++;
        //}
    }

    cam.position.x = elapsed;

    //for (int i = particles.size() - 1; i >= 0; i--) {
    //    Particle& p = particles[i];
    //    p.position += p.velocity * delta_time.count();

    //    if (elapsed - p.start > 1s) {
    //        particles.erase(particles.begin() + i);
    //    }
    //}


    //Style style{};
    //style.anchor = { 1,0 };
    //ui.begin_group(style);
    //std::string score_text = std::to_string(score);
    //ui.rect(score_text.data());
    //ui.end_group();
    ui.begin_group(Style{ {1,0} }); 
        auto score_text = std::to_string(score);
        ui.rect(score_text.data(), {});
    ui.end_group();

    ui.begin_group(Style{ {0,1} }); 
        auto combo_text = std::format("{}x", combo);
        ui.rect(combo_text.data(), {});
    ui.end_group();

    ui.begin_group(Style{ {1,1} }); 
        auto frame_time = std::to_string(((float)std::chrono::duration_cast<std::chrono::microseconds>(delta_time).count()) / 1000) + " ms";
        ui.rect(frame_time.data(), {});
    ui.end_group();

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    float x = elapsed;
    Vec2 p1 = cam.world_to_screen({ x, 0.5f });
    Vec2 p2 = cam.world_to_screen({ x, -0.5f });

    SDL_SetRenderDrawColor(renderer, 255, 255, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);

    draw_map(renderer, assets, map, cam, current_note);

    auto inner_drum = assets.get_image("inner_drum");
    auto outer_drum = assets.get_image("outer_drum");
    Vec2 drum_pos = { 0, (window_height - inner_drum.height) / 2};

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


    Vec2 target = cam.world_to_screen(cam.position);
    //DrawCircle(target.x, target.y, 50, WHITE);
    //DrawText((std::to_string(delta_time.count() * 1000) + " ms").data(), 100, 100, 24, WHITE);
    //draw_particles(cam, particles, elapsed);

    ui.draw(renderer);

    SDL_RenderPresent(renderer);
}
