#pragma once

#include "vec.h"
#include <filesystem>

namespace constants {

    const inline std::filesystem::path maps_directory{ "data/maps/" };
    constexpr const char* map_file_extension = ".tko";
    constexpr const char* mapset_filename = "mapset";
    constexpr int window_width = 1920;
    constexpr int window_height = 1080;

    using namespace std::chrono_literals;

    constexpr std::chrono::duration<double> ok_range = 140ms;
    constexpr std::chrono::duration<double> perfect_range = 50ms;

    constexpr std::chrono::duration<float> hit_effect_duration = 80ms;

    constexpr float total_flight_time_seconds = 0.3;

    constexpr float circle_radius = 0.1f;
    constexpr float circle_outer_radius = 0.11f;

    constexpr Vec2 note_hitbox{ 0.25, 0.25 };

    constexpr float perfect_accuracy_weight = 1;
    constexpr float ok_accuracy_weight = 0.5;
    constexpr float miss_accuracy_weight = 0;
}
