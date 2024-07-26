#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <unordered_map>

struct Image {
    SDL_Texture* texture;
    int w, h;
};

class Assets {
public:
    void init(SDL_Renderer* renderer);
    Image get(const char* name);
    //int index(const char* name);
private:
    std::vector<Image> things;
    std::unordered_map<const char*, int> index;
};
