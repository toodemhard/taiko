#include <tracy/Tracy.hpp>
#include <vector>
#include <fstream>

#include "font.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

stbtt_fontinfo font_info;
stbtt_bakedchar char_data[96];
SDL_Texture* font_texture;
constexpr int ft_width = 512, ft_height = 512;
constexpr int ft_length = ft_width * ft_height;

std::vector<char> read_file(const char* file_name) {
    std::string path = "data/" + std::string(file_name);
    std::ifstream file{ path, std::ios::binary | std::ios::ate };
    if (!file) {
        exit(1);
    }

    auto size = file.tellg();
    std::vector<char> data(size, '\0');
    file.seekg(0);
    file.read(data.data(), size);

    return data;
}

void init_font(SDL_Renderer* renderer) {
    auto ttf_buffer = read_file("JetBrainsMonoNLNerdFont-Regular.ttf");
    stbtt_InitFont(&font_info, (unsigned char*)ttf_buffer.data(), 0);


    std::vector<unsigned char> bitmap(ft_length);

    stbtt_BakeFontBitmap((unsigned char*)ttf_buffer.data(), 0, 32, bitmap.data(), ft_width, ft_height, 32, 96, char_data);

    std::vector<unsigned char> asdf(ft_length * 4);
    for (int i = 0; i < ft_length; i++) {
        asdf[i * 4] = bitmap[i];
        asdf[i * 4 + 1] = bitmap[i];
        asdf[i * 4 + 2] = bitmap[i];
        asdf[i * 4 + 3] = bitmap[i];
    }

    auto surface = SDL_CreateSurfaceFrom(asdf.data(), ft_width, ft_height, ft_width * 4, SDL_PIXELFORMAT_RGBA8888);
    font_texture = SDL_CreateTextureFromSurface(renderer, surface);
}

 
void draw_text(SDL_Renderer* renderer, const char* text, Vec2 position)
{
    ZoneScoped;

    stbtt_aligned_quad quad;

    while (*text) {
        stbtt_GetBakedQuad(char_data, ft_width, ft_height, *text - 32, &position.x, &position.y, &quad, 1);
        float w = (quad.s1 - quad.s0) * ft_width;
        float h = (quad.t1 - quad.t0) * ft_height;
        auto rect = SDL_FRect{ quad.s0 * ft_width, quad.t0 * ft_height, w, h };
        auto dst = SDL_FRect{ position.x, position.y + char_data[*text - 32].yoff, w, h };

        SDL_RenderTexture(renderer, font_texture, &rect, &dst);

        ++text;
    }
}

float font_width(const char* text, float font_size) {
    return 100;
}

float font_height(float font_size) {
    return font_size;
}
