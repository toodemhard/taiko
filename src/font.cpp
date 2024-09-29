#include <tracy/Tracy.hpp>
#include <vector>
#include <fstream>

#include "font.h"
#include "SDL3/SDL_render.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

stbtt_fontinfo font_info;
stbtt_bakedchar char_data[96];
SDL_Texture* font_texture;
SDL_Texture* icon_font_texture;
constexpr int ft_width = 512, ft_height = 512;
constexpr int ft_length = ft_width * ft_height;

const float pixel_height = 36;

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

void kms() {
    auto ttf_buffer = read_file("JetBrainsMonoNLNerdFont-Regular.ttf");

    std::vector<unsigned char> bitmap(ft_length);

    stbtt_pack_context spc;
    stbtt_PackBegin(&spc, bitmap.data(), ft_width, ft_height, 0, 1, nullptr);

    std::vector<int> chars = {
        U'󰒫',
        U'󰒬',
        U'󰐊',
        U'󰏤',
    };

    const int num_chars = 2;
    stbtt_packedchar char_data[num_chars];

    for (int c : chars) {
        stbtt_PackFontRange(&spc, (unsigned char*)ttf_buffer.data(), 0, 36, c, 1, char_data);
    }

    stbtt_PackEnd(&spc);

    stbi_write_png("idk.png", ft_width, ft_height, 1, bitmap.data(), 1 * ft_width);
}

std::vector<unsigned char> main_atlas() {
    auto ttf_buffer = read_file("Avenir LT Std 95 Black.ttf");
    stbtt_pack_context spc;
    std::vector<unsigned char> bitmap(ft_length);
    stbtt_PackBegin(&spc, bitmap.data(), ft_width, ft_height, 0, 1, nullptr);

    stbtt_packedchar char_data[96];
    stbtt_PackFontRange(&spc, (unsigned char*)ttf_buffer.data(), 0, 48, 32, 96, char_data);

    stbtt_PackEnd(&spc);

    return bitmap;
}

void init_font(SDL_Renderer* renderer) {
    auto ttf_buffer = read_file("Avenir LT Std 95 Black.ttf");
    stbtt_InitFont(&font_info, (unsigned char*)ttf_buffer.data(), 0);


    std::vector<unsigned char> bitmap(ft_length);

    stbtt_BakeFontBitmap((unsigned char*)ttf_buffer.data(), 0, pixel_height, bitmap.data(), ft_width, ft_height, 32, 96, char_data);

    std::vector<unsigned char> asdf(ft_length * 4);
    for (int i = 0; i < ft_length; i++) {
        asdf[i * 4] = bitmap[i];
        asdf[i * 4 + 1] = bitmap[i];
        asdf[i * 4 + 2] = bitmap[i];
        asdf[i * 4 + 3] = bitmap[i];
    }

    auto surface = SDL_CreateSurfaceFrom(ft_width, ft_height, SDL_PIXELFORMAT_RGBA8888, asdf.data(), ft_width * 4);
    font_texture = SDL_CreateTextureFromSurface(renderer, surface);
}

 
void draw_text(SDL_Renderer* renderer, const char* text, float font_size, Vec2 position, RGBA color) {
    ZoneScoped;
    if (text == nullptr) {
        return;
    }

    SDL_SetTextureColorMod(font_texture, color.r, color.b, color.g);
    SDL_SetTextureAlphaMod(font_texture, 128);

    float size_ratio = font_size / pixel_height;

    float x = position.x;
    while (*text) {
        auto& char_info = char_data[*text - 32];
        float w = char_info.x1 - char_info.x0;
        float h = char_info.y1 - char_info.y0;
        auto src = SDL_FRect{ (float)char_info.x0, (float)char_info.y0, w, h };
        auto dst = SDL_FRect{ x + char_info.xoff, font_size + position.y + char_info.yoff * size_ratio, w * size_ratio, h * size_ratio};

        SDL_RenderTexture(renderer, font_texture, &src, &dst);

        x += char_info.xadvance * size_ratio;
        ++text;
    }
}

void draw_text_cutoff(SDL_Renderer* renderer, const char* text, float font_size, Vec2 position, RGBA color, float max_width) {
    ZoneScoped;
    if (text == nullptr) {
        return;
    }

    SDL_SetTextureColorMod(font_texture, color.r, color.b, color.g);
    SDL_SetTextureAlphaMod(font_texture, color.a);

    float size_ratio = font_size / pixel_height;

    float x = position.x;
    while (*text && x - position.x < max_width) {
        if (*text == '\n') {
            position.y += font_size;
            x = position.x;
        } else {
            auto& char_info = char_data[*text - 32];
            float w = char_info.x1 - char_info.x0;
            float h = char_info.y1 - char_info.y0;
            auto src = SDL_FRect{ (float)char_info.x0, (float)char_info.y0, w, h };
            auto dst = SDL_FRect{ x + char_info.xoff, font_size + position.y + char_info.yoff * size_ratio, w * size_ratio, h * size_ratio};

            SDL_RenderTexture(renderer, font_texture, &src, &dst);
            x += char_info.xadvance * size_ratio;
        }

        ++text;
    }
}

Vec2 text_dimensions(const char* text, float font_size) {
    float max_width = 0;
    float width = 0;

    if (text == nullptr) {
        return {};
    }

    float size_ratio = font_size / pixel_height;

    int lines = 1;

    while (*text) {
        if (*text == '\n') {
            max_width = std::max(max_width, width);
            width = 0;
            lines++;
        } else {
            width += char_data[*text - 32].xadvance;
        }

        ++text;
    }
    max_width = std::max(max_width, width);

    return {max_width * size_ratio, lines * font_size};
}

float text_height(const char* text, float font_size) {
    if (text == nullptr) {
        return 0;
    }

    return font_size;
}
