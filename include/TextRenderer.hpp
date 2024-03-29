//
// Created by Filip on 14.6.2023.
//

#ifndef Z80SDL_TEXTRENDERER_HPP
#define Z80SDL_TEXTRENDERER_HPP

#include <SDL2pp/SDL2pp.hh>
#include "FTWrapper.hpp"

class TextRenderer {
public:
    union TextAttributes {
        struct {
            uint8_t fgcolor : 4;
            uint8_t bgcolor : 4;
        };
        uint8_t as_byte;

        bool operator != (TextAttributes other) const;

        TextAttributes() {};
        TextAttributes(uint8_t byte) : as_byte(byte) {};
        TextAttributes(const TextRenderer::TextAttributes& other) : as_byte(other.as_byte) {};
    };

    struct RawInfo {
        char* text;
        TextAttributes* attrs;
        int size;
    };

    explicit TextRenderer(SDL_Renderer *output);
    ~TextRenderer();

    char& charAt(int line, int col);
    TextAttributes& attrAt(int line, int col);

    void print(int line, int col, const std::string &str);
    void print(int line, int col, const std::string &str, TextAttributes attr);

    RawInfo getRawInfo();

    void makeCharAtlas(const char* filepath);
    void render();
    void present();

    static constexpr int
        WIN_W = 1024, WIN_H = 768,
        N_LINES = WIN_H/16, N_COLS = WIN_W/8;

private:
    static int atlasPosition(uint8_t c);
    static std::u32string codepageConvert();
    static void drawChar(ftpp::FTFace &font, SDL2pp::Surface &atlas, char32_t c, int x);
    static void drawBitmap(FT_Bitmap bitmap, SDL2pp::Surface &atlas, int x, int y);
    void renderChar(char c, int row, int col, TextAttributes attr);

    static SDL_Color interpolateColor(SDL_Color from, SDL_Color to, float percent);

    std::vector<char> textBuffer;
    std::vector<TextAttributes> attrBuffer;

    SDL2pp::Renderer outputRenderer;
    SDL2pp::Surface frameBuffer;
    SDL2pp::Texture fbTex;

    std::vector<float> textAtlasNormalized;

    SDL_Color colorPalette[16];

    static constexpr int
        FONT_W = 8, FONT_H = 16,
        FONT_SIZE = 16, N_COLORS = 256;
    static constexpr const char* CODEPAGE = "cp852";
};


#endif //Z80SDL_TEXTRENDERER_HPP
