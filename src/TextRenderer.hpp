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
    };

    explicit TextRenderer(SDL_Renderer *output);
    ~TextRenderer();

    char& charAt(int line, int col);
    TextAttributes& attrAt(int line, int col);

    void print(int line, int col, const std::string &str);
    void print(int line, int col, const std::string &str, TextAttributes attr);

    char* charBfrRaw();
    TextAttributes* attrBfrRaw();

    void makeCharAtlas(const char* filepath);
    void render();

    static constexpr int
        WIN_W = 792, WIN_H = 588,
        N_LINES = 42, N_COLS = 88;

private:
    static int atlasPosition(uint8_t c);
    static std::u32string codepageConvert();
    void drawChar(ftpp::FTFace &font, char32_t c, int x);
    void drawBitmap(FT_Bitmap bitmap, int x, int y);
    void renderChar(char c, int row, int col, TextAttributes attr);

    std::vector<char> textBuffer;
    std::vector<TextAttributes> attrBuffer;

    SDL2pp::Renderer outputRenderer;
    SDL2pp::Surface textAtlas, frameBuffer;
    SDL2pp::Texture fbTex;

    SDL_Color colorPalette[16];
    SDL_Color *surfacePalette;

    static constexpr int
        FONT_W = 9, FONT_H = 14,
        FONT_SIZE = 16, N_COLORS = 256;
    static constexpr const char* CODEPAGE = "cp852";
};


#endif //Z80SDL_TEXTRENDERER_HPP
