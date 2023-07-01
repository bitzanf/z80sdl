//
// Created by Filip on 14.6.2023.
//

#include "TextRenderer.hpp"
#include <unicode/ucnv.h>

bool TextRenderer::TextAttributes::operator != (const TextRenderer::TextAttributes other) const {
    return as_byte != other.as_byte;
}

TextRenderer::TextRenderer(SDL_Renderer *output)
    : outputRenderer(output)
    , frameBuffer(SDL_CreateRGBSurfaceWithFormat(0, WIN_W, WIN_H, 32, SDL_PIXELFORMAT_RGBA8888))
    , fbTex(outputRenderer, frameBuffer)
    //, textAtlas(0, 16 * FONT_W, 16 * FONT_H, 1, 1, 1, 1, 1)
    , textAtlas(SDL_CreateRGBSurfaceWithFormat(0, 256 * FONT_W, FONT_H, 1, SDL_PIXELFORMAT_INDEX1MSB))
{
    textBuffer.resize(N_LINES * N_COLS);
    attrBuffer.resize(N_LINES * N_COLS);

    for (int i = 0; i < 8; i++) {
        colorPalette[i].r = (i & 0b100) ? 0x7f : 0;
        colorPalette[i].g = (i & 0b010) ? 0x7f : 0;
        colorPalette[i].b = (i & 0b001) ? 0x7f : 0;
        colorPalette[i].a = 0xff;
    }

    for (int i = 9; i < 16; i++) {
        colorPalette[i].r = (i & 0b100) ? 0xff : 0;
        colorPalette[i].g = (i & 0b010) ? 0xff : 0;
        colorPalette[i].b = (i & 0b001) ? 0xff : 0;
        colorPalette[i].a = 0xff;
    }
}

TextRenderer::~TextRenderer() {

}

char& TextRenderer::charAt(int line, int col) {
    return textBuffer[line * N_COLS + col];
}

TextRenderer::TextAttributes& TextRenderer::attrAt(int line, int col) {
    return attrBuffer[line * N_COLS + col];
}

int TextRenderer::atlasPosition(char c) {
    return c * FONT_W;
}

void TextRenderer::makeCharAtlas(const char *filepath) {
    ftpp::FTLib lib;
    ftpp::FTFace font{lib, filepath};
    font.setPixelSizes(FONT_H);

    auto charset = codepageConvert();
    for (int i = 0; i < 256; i++) drawChar(font, charset[i], atlasPosition(i));

    auto * window = new SDL2pp::Window{"meow", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, FONT_W*256, FONT_H, SDL_WINDOW_SHOWN};
    SDL2pp::Renderer renderer{*window, -1, SDL_RENDERER_ACCELERATED};
    SDL2pp::Texture texture{renderer, textAtlas};
    renderer.Clear();
    renderer.Copy(texture);
    renderer.Present();
}

std::u32string TextRenderer::codepageConvert() {
    UErrorCode err = U_ZERO_ERROR;
    auto detectFailure = [&err]{
        if (U_FAILURE(err)) throw std::runtime_error(u_errorName(err));
    };

    UConverter *conv = ucnv_open(CODEPAGE, &err);
    detectFailure();

    UChar32 character;
    std::u32string result;
    char charsetStr[2];
    charsetStr[1] = 0;

    const char *source, *sourceLimit;

    for (unsigned char c = 0; c < 0xff; c++) {
        charsetStr[0] = c;
        source = charsetStr;
        sourceLimit = charsetStr + 1;

        while (source < sourceLimit) {
            character = ucnv_getNextUChar(conv, &source, sourceLimit, &err);
            detectFailure();
            result.push_back(character);
        }
    }

    ucnv_close(conv);
    return result;
}

void TextRenderer::drawChar(ftpp::FTFace &font, char32_t c, int x) {
    FT_GlyphSlot glyphSlot = font->glyph;
    int xpos, ypos;

    FT_UInt idx = font.getCharIndex(c);
    if (!idx) return;
    font.loadGlyph(idx, FT_LOAD_DEFAULT);

    xpos = x + glyphSlot->bitmap_left;
    ypos = (FONT_H - 1) - glyphSlot->bitmap_top;
    font.renderGlyph(FT_RENDER_MODE_MONO);
    drawBitmap(glyphSlot->bitmap, xpos, ypos);
}

void TextRenderer::drawBitmap(FT_Bitmap bitmap, int x, int y) {
    if (bitmap.width == 0 || bitmap.rows == 0) return;
    if (bitmap.width + x > textAtlas.GetWidth() || bitmap.rows + y > textAtlas.GetHeight()) return;

    /*for (int row = 0; row < bitmap.rows; row++) {
        for (int col = 0; col <= bitmap.width / 8; col++) {
            int pixelIDX = row * bitmap.width / 8 + col;
            pixels[x/8 + col + (row + y) * pxw] = bitmap.buffer[pixelIDX];
        }
    }*/

    //SDL_PIXELFORMAT_INDEX1MSB
    struct PackedPixels1 {
        uint8_t operator ()(int x, int y) const {
            return (pixels[y] >> (7-x)) & 1;
        }

        unsigned int w, h;
        uint8_t *pixels;
    };

    struct PackedPixels2 {
        void write(int x, int y, uint8_t val) {
            val &= 1;
            int pxnum = x + y * w;
            int px = pxnum % 8;
            int idx = pxnum / 8;

            if (val) {
                pixels[idx] |= (1 << (7-px));
            } else {
                pixels[idx] &= ~(1 << (7-px));
            }
        }

        int w, h;
        uint8_t *pixels;
    };

    PackedPixels1 src {
        .w = bitmap.width,
        .h = bitmap.rows,
        .pixels = (uint8_t*)bitmap.buffer
    };
    PackedPixels2 dst {
        .w = textAtlas.GetWidth(),
        .h = textAtlas.GetHeight(),
        .pixels = (uint8_t*)textAtlas.Get()->pixels
    };

    for (int row = 0; row < bitmap.rows; row++) {
        for (int col = 0; col < bitmap.width; col++) {
            dst.write(x + col, y + row, src(col, row));
        }
    }
}

void TextRenderer::render() {
    for (int row = 0; row < N_LINES; row++) {
        for (int col = 0; col < N_COLS; col++) {
            int bfridx = row * N_COLS + col;
            renderChar(textBuffer[bfridx], row, col, attrBuffer[bfridx]);
        }
    }

    fbTex.Update(SDL2pp::NullOpt, frameBuffer);
    outputRenderer.Clear();
    outputRenderer.Copy(fbTex);
    outputRenderer.Present();
}

void TextRenderer::renderChar(char c, int row, int col, TextRenderer::TextAttributes attr) {
    SDL_Color colors[2] { colorPalette[attr.bgcolor], colorPalette[attr.fgcolor] };
    SDL_SetPaletteColors(textAtlas.Get()->format->palette, colors, 0, 2);

    SDL2pp::Rect fbPos{ col * FONT_W, row * FONT_H, FONT_W, FONT_H };

    textAtlas.Blit(SDL2pp::Rect{ atlasPosition(c), 0, FONT_W, FONT_H }, frameBuffer, fbPos);
}




/*
void FTRenderer::drawGlyphSlot(const std::u32string &codepoints, SDL_Color color, int x, int y) {
    FT_Face pFTFace = face.get();
    int x_orig = x;
    int line_shift = pFTFace->size->metrics.y_ppem;

    int xpos, ypos;
    FT_UInt idx;
    for (auto c : codepoints) {
        idx = face.getCharIndex(c);
        if (!idx) continue;
        face.loadGlyph(idx, FT_LOAD_DEFAULT);

        xpos = x + pFTFace->glyph->bitmap_left;
        ypos = y - pFTFace->glyph->bitmap_top;
        if (xpos > drawSurface->GetWidth() + (pFTFace->glyph->advance.x >> 6)) {
            x = x_orig;
            y += line_shift;
            xpos = x + pFTFace->glyph->bitmap_left;
            ypos = y - pFTFace->glyph->bitmap_top;
        }
        if (ypos > drawSurface->GetHeight()) break; // overflow
        face.renderGlyph(FT_RENDER_MODE_NORMAL);
        drawBitmap(xpos, ypos, color);

        x += pFTFace->glyph->advance.x >> 6;
        y += pFTFace->glyph->advance.y >> 6;
    }
}

void TextRenderer::drawBitmap(FT_Bitmap bitmap, int x, int y) {
    if (bitmap.width == 0 || bitmap.rows == 0) return;
    if (bitmap.width + x > textAtlas.GetWidth() || bitmap.rows + y > textAtlas.GetHeight()) return;

    auto *pixels = (uint8_t *)textAtlas.Get()->pixels;

    SDL2pp::Rect dstrect{ x, y, (int)bitmap.width, (int)bitmap.rows };
    SDL_PixelFormat *fmt = textAtlas.Get()->format;

    //SDL2pp::Surface bm { SDL_CreateRGBSurfaceFrom(bitmap.buffer, bitmap.width, bitmap.rows, 1, 1, 0, 0, 0, 0) };
    SDL2pp::Surface bm { SDL_CreateRGBSurfaceWithFormatFrom(bitmap.buffer, bitmap.width, bitmap.rows, 1, 1, SDL_PIXELFORMAT_INDEX1MSB) };
    bm.Blit(SDL2pp::NullOpt, textAtlas, dstrect);
}*/
