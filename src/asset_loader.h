#pragma once
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <vector>
#include <unordered_map>
#include <optional>
#include <filesystem>

#include "color.h"

struct Image {
    SDL_Texture* texture;
    int width, height;
};

struct ImageLoadInfo {
    const char* file_name;
    const char* name;
    std::optional<RGBA> color;
};

struct SoundLoadInfo {
    const char* file_name;
    const char* name;
};

class AssetLoader {
public:
    // init() file with "image.png" then get_image() with "image"
    void init(SDL_Renderer* renderer, std::vector<ImageLoadInfo>& image_list, std::vector<SoundLoadInfo>& sound_list);
    Image get_image(const char* name);
    Mix_Chunk* get_sound(const char* name);
private:
    std::vector<Image> images;
    std::unordered_map<const char*, int> image_index;

    std::unordered_map<const char*, Mix_Chunk*> sounds;
};
