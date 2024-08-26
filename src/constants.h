#pragma once

#include "vec.h"

namespace constants {

    const inline std::filesystem::path maps_directory{ "data/maps/" };
    constexpr const char* map_file_extension = ".tko";
    constexpr const char* mapset_filename = "mapset";
    constexpr int window_width = 1280;
    constexpr int window_height = 960;

    using namespace std::chrono_literals;

    constexpr std::chrono::duration<double> hit_range = 100ms;
    constexpr std::chrono::duration<double> perfect_range = 30ms;

    constexpr float circle_radius = 0.1f;
    constexpr float circle_outer_radius = 0.11f;

    constexpr Vec2 note_hitbox{ 0.25, 0.25 };

}
