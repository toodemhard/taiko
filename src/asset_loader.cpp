#include <tracy/Tracy.hpp>

#include "asset_loader.h"
#include <SDL3_image/SDL_image.h>
#include <format>
#include <iostream>
#include <filesystem>

#include "debug_macros.h"

std::filesystem::path asset_path(const char* path) {
    return "data/" + std::string(path);
}

Image load_asset(SDL_Renderer* renderer, const char* path) {
    SDL_Surface* surface = IMG_Load(asset_path(path).string().data());

    Image image;
    image.width = surface->w;
    image.height = surface->h;
    image.texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_DestroySurface(surface);

    return image;
}

Image load_asset_tint(SDL_Renderer* renderer, const char* path, const Color& color) {
    SDL_Surface* surface = IMG_Load(asset_path(path).string().data());

    SDL_SetSurfaceColorMod(surface, color.r, color.g, color.b);

    Image image;
    image.width = surface->w;
    image.height = surface->h;
    image.texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_DestroySurface(surface);

    return image;
}


Image Assets::get_image(const char* name) {
    ZoneScoped;

    auto search = index.find(name);
    if (search == index.end()) {
        DEBUG_PANIC(std::format("could not find asset: {}\n", name))
        return {};
    }

    return things[search->second];
}

Mix_Chunk* Assets::get_sound(const char* name) {
    ZoneScoped;

    auto search = sounds.find(name);
    if (search == sounds.end()) {
        DEBUG_PANIC(std::format("could not find asset: {}\n", name))
        return {};
    }

    return search->second;
}

void Assets::init(SDL_Renderer* renderer, std::vector<ImageLoadInfo> image_list, std::vector<SoundLoadInfo> sound_list) {
    ZoneScoped;

    things = std::vector<Image>(image_list.size()); 
    for (int i = 0; i < image_list.size(); i++) {
        const auto& info = image_list[i];
        
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

    for (int i = 0; i < sound_list.size(); i++) {
        const auto& info = sound_list[i];
        sounds[info.name] = Mix_LoadWAV(asset_path(info.file_name).string().data()); 
    } 
} 