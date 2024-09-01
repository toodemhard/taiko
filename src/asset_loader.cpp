#include <tracy/Tracy.hpp>

#include "asset_loader.h"
#include <SDL3_image/SDL_image.h>
#include <format>
#include <iostream>
#include <filesystem>

#include "debug_macros.h"


const static std::filesystem::path asset_directory{ "data/" };

std::filesystem::path asset_path(const char* path) {
    return "data/" + std::string(path);
}

Image load_asset(SDL_Renderer* renderer, const char* file_name, std::optional<RGBA> tint_color) {
    SDL_Surface* surface = IMG_Load((asset_directory / file_name).string().data());

    if (tint_color.has_value()) {
        const auto& color_value = tint_color.value();
        SDL_SetSurfaceColorMod(surface, color_value.r, color_value.g, color_value.b);
    }

    Image image;
    image.width = surface->w;
    image.height = surface->h;
    image.texture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_DestroySurface(surface);

    return image;
}


Image AssetLoader::get_image(const char* name) {
    ZoneScoped;

    auto search = image_index.find(name);
    if (search == image_index.end()) {
        DEBUG_PANIC(std::format("could not find asset: {}\n", name))
        return {};
    }

    return images[search->second];
}

Mix_Chunk* AssetLoader::get_sound(const char* name) {
    ZoneScoped;

    auto search = sounds.find(name);
    if (search == sounds.end()) {
        DEBUG_PANIC(std::format("could not find asset: {}\n", name))
        return {};
    }

    return search->second;
}

void AssetLoader::init(SDL_Renderer* renderer, std::vector<ImageLoadInfo>& image_list, std::vector<SoundLoadInfo>& sound_list) {
    ZoneScoped;

    images = std::vector<Image>(image_list.size()); 
    for (int i = 0; i < image_list.size(); i++) {
        const auto& info = image_list[i];
        
        Image image;
        image = load_asset(renderer, info.file_name, info.color);

        images[i] = image;
        image_index[info.name] = i;
    }

    for (int i = 0; i < sound_list.size(); i++) {
        const auto& info = sound_list[i];
        sounds[info.name] = Mix_LoadWAV(asset_path(info.file_name).string().data()); 
    } 
} 