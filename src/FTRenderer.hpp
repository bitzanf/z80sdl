//
// Created by Filip on 19.4.2023.
//

#ifndef Z80SDL_FTRENDERER_HPP
#define Z80SDL_FTRENDERER_HPP

#include <SDL2/SDL.h>
#include <SDL2pp/SDL2pp.hh>
#include "FTWrapper.hpp"
#include <map>

class FTRenderer {
public:
    union TextAttributes {
        struct {
            uint8_t fgcolor : 4;
            uint8_t bgcolor : 4;
        };
        uint8_t as_byte;

        bool operator != (TextAttributes other) const;
    };

    FTRenderer(SDL2pp::Renderer &_renderer, ftpp::FTFace &_face, int lines, int columns);
    ~FTRenderer();
    char& charAt(int line, int column);
    TextAttributes& attrAt(int line, int column);
    size_t bufferSize();
    char* textBufferPosition(int line, int column);
    TextAttributes* attributeBufferPosition(int line, int column);
    void render();
    void present();
    void mkMapForCharset(const std::string &charset);
    void resize(int w, int h);
    SDL2pp::Point resizeText(int charH);

private:
    void renderChar(int line, int column);
    int linearBufferIdx(int line, int column) const;
    void drawGlyphSlot(const std::u32string &codepoints, SDL_Color color, int x, int y);
    void drawBitmap(int x, int y, SDL_Color color);
    static SDL2pp::Surface mkGlyphBuffer(int w, int h);
    void reallocDrawArea(int w, int h);

    int nLines, nColumns;
    std::vector<char> textBuffer;
    std::vector<TextAttributes> attributeBuffer;
    std::map<char, std::u32string> charsetMapping;

    ftpp::FTFace &face;
    SDL2pp::Surface *drawSurface, glyphBuffer;
    SDL2pp::Texture *texture;
    SDL2pp::Renderer &renderer;
    SDL_Color palette[256], textColors[16];
};


#endif //Z80SDL_FTRENDERER_HPP
