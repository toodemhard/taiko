#include <tracy/Tracy.hpp>

#include "asset_loader.h"
#include <SDL3_image/SDL_image.h>
#include <optional>
#include <format>
#include <iostream>

#include "debug_macros.h"
#include "color.h"

std::string asset_path(const char* path) {
    return std::string("data/") + std::string(path);
}

Image load_asset(SDL_Renderer* renderer, const char* path) {
    SDL_Surface* surface = IMG_Load(asset_path(path).data());

    Image image;
    image.w = surface->w;
    image.h = surface->h;
    image.texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_DestroySurface(surface);

    return image;
}

Image load_asset_tint(SDL_Renderer* renderer, const char* path, const Color& color) {
    SDL_Surface* surface = IMG_Load(asset_path(path).data());

    SDL_SetSurfaceColorMod(surface, color.r, color.g, color.b);

    Image image;
    image.w = surface->w;
    image.h = surface->h;
    image.texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_DestroySurface(surface);

    return image;
}

struct AssetLoadInfo {
    const char* file_name;
    const char* name;
    std::optional<Color> color;
};

Image Assets::get(const char* name) {
    ZoneScoped;

    auto search = index.find(name);
    if (search == index.end()) {
        DEBUG_PANIC(std::format("could not find asset: {}\n", name))
        return {};
    }

    return things[search->second];
}

void Assets::init(SDL_Renderer* renderer) {
    ZoneScoped;

    std::vector<AssetLoadInfo> infos = {
        {"circle-overlay.png", "circle_overlay"},
        {"circle-select.png", "select_circle"},
        {"circle.png", "kat_circle", Color{ 60, 219, 226, 255 }},
        {"circle.png", "don_circle", Color{ 252, 78, 60, 255 }},
    };

    things = std::vector<Image>(infos.size()); 
    for (int i = 0; i < infos.size(); i++) {
        const auto& info = infos[i];
        
        Image image;
        if (!info.color.has_value()) {
            image = load_asset(renderer, info.file_name);
        }
        else {
            image = load_asset_tint(renderer, info.file_name, info.color.value());
        }

        things[i] = image;
        index[info.name] = i;
    }
}
