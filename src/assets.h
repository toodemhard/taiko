#pragma once
#include "asset_loader.h"

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
    bg,
    back_frame,
    crosshair_rim,
    crosshair_rim_outer,
    crosshair_fill,

    count, // dont touch this
};
}

namespace SoundID {
enum SoundID {
    don,
    kat,

    count, // leave at end
};
}

using AssetLoader = engine::AssetLoader<ImageID::count, SoundID::count>;
