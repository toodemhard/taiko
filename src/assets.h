#pragma once

namespace ImageID {
enum ImageID {
    hit_effect_ok,
    hit_effect_perfect,
    circle_overlay,
    select_circle,
    kat_circle,
    don_circle,
    inner_drum,
    outer_drum,
    count
};
}

namespace SoundID {
enum SoundID {
    don,
    kat,
    count
};
}

using AssetLoader = engine::AssetLoader<ImageID::count, SoundID::count>;
