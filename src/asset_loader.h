#pragma once
#include <tracy/Tracy.hpp>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_image/SDL_image.h>
#include <optional>
#include <filesystem>
#include <array>

#include "color.h"

struct Image {
    SDL_Texture* texture;
    int width, height;
};

struct ImageLoadInfo {
    const char* file_name;
    int index;
    std::optional<RGBA> color;
};

struct SoundLoadInfo {
    const char* file_name;
    int index;
};

namespace engine {

    template<int image_count, int sound_count>
    class AssetLoader {
    public:
        void init(SDL_Renderer* renderer, std::vector<ImageLoadInfo>& image_list, std::vector<SoundLoadInfo>& sound_list);
        Image get_image(int id);
        Mix_Chunk* get_sound(int id);
    private:
        std::array<Image, image_count> images;
        std::array<Mix_Chunk*, sound_count> sounds;
    };

}

const inline std::filesystem::path asset_directory{ "data/" };

inline Image load_asset(SDL_Renderer* renderer, const char* file_name, std::optional<RGBA> tint_color) {
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

namespace engine {

    template<int image_count, int sound_count>
    Image AssetLoader<image_count, sound_count>::get_image(int id) {
        return images[id];
    }

    template<int image_count, int sound_count>
    Mix_Chunk* AssetLoader<image_count, sound_count>::get_sound(int id) {
        return sounds[id];
    }

    template<int image_count, int sound_count>
    void AssetLoader<image_count, sound_count>::init(SDL_Renderer* renderer, std::vector<ImageLoadInfo>& image_list, std::vector<SoundLoadInfo>& sound_list) {
        ZoneScoped;

        for (const auto& load_info : image_list) {
            images[load_info.index] = load_asset(renderer, load_info.file_name, load_info.color);

        }

        for (const auto& load_info : sound_list) {
            sounds[load_info.index] = Mix_LoadWAV((asset_directory / load_info.file_name).string().data());
        }
    }

}
