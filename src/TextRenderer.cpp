//
// Created by Filip on 14.6.2023.
//

#include "../include/TextRenderer.hpp"
#include <unicode/ucnv.h>

bool TextRenderer::TextAttributes::operator != (const TextRenderer::TextAttributes other) const {
    return as_byte != other.as_byte;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-member-init"
TextRenderer::TextRenderer(SDL_Renderer *output)
    : outputRenderer(output)
    , frameBuffer(SDL_CreateRGBSurfaceWithFormat(0, WIN_W, WIN_H, 32, SDL_PIXELFORMAT_RGBA8888))
    , fbTex(outputRenderer, frameBuffer)
    , textAtlas(0, 256 * FONT_W, FONT_H, 8, 0, 0, 0, 0)
    , surfacePalette(new SDL_Color[N_COLORS])
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

    memset(surfacePalette, 0, N_COLORS * sizeof(SDL_Color));
}
#pragma clang diagnostic pop

TextRenderer::~TextRenderer() {
    delete[] surfacePalette;
}

char& TextRenderer::charAt(int line, int col) {
    if (line < 0) line = N_LINES + line;
    if (col < 0) col = N_COLS + col;
    return textBuffer[line * N_COLS + col];
}

TextRenderer::TextAttributes& TextRenderer::attrAt(int line, int col) {
    if (line < 0) line = N_LINES + line;
    if (col < 0) col = N_COLS + col;
    return attrBuffer[line * N_COLS + col];
}

int TextRenderer::atlasPosition(uint8_t c) {
    return c * FONT_W;
}

void TextRenderer::makeCharAtlas(const char *filepath) {
    ftpp::FTLib lib;
    ftpp::FTFace font{lib, filepath};
    font.setPixelSizes(FONT_SIZE);

    auto charset = codepageConvert();
    for (int i = 0; i < 256; i++) drawChar(font, charset[i], atlasPosition(i));
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
        charsetStr[0] = c; // NOLINT(cppcoreguidelines-narrowing-conversions)
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
    ypos = FONT_H - glyphSlot->bitmap_top - 3;
    font.renderGlyph(FT_RENDER_MODE_NORMAL);
    drawBitmap(glyphSlot->bitmap, xpos, ypos);
}

void TextRenderer::render() {
    for (int row = 0; row < N_LINES; row++) {
        for (int col = 0; col < N_COLS; col++) {
            int bfridx = row * N_COLS + col;
            renderChar(textBuffer[bfridx], row, col, attrBuffer[bfridx]);
        }
    }

    fbTex.Update(SDL2pp::NullOpt, frameBuffer);
    present();
}

void TextRenderer::present() {
    outputRenderer.Clear();
    outputRenderer.Copy(fbTex);
}

void TextRenderer::renderChar(char c, int row, int col, TextRenderer::TextAttributes attr) {
#if 1
    auto atlasPX = (uint8_t*)textAtlas.Get()->pixels;
    auto fbPX = (uint32_t*)frameBuffer.Get()->pixels;

    static const size_t c_fb_pxPerLine = FONT_W * N_COLS;
    static const size_t c_fb_pxPerCharRow = FONT_H * c_fb_pxPerLine;
    static const size_t c_ta_pxPerLine = FONT_W * 256;

    const size_t c_fb_charRowOffset = row * c_fb_pxPerCharRow;
    const size_t c_fb_charColOffset = col * FONT_W;
    const size_t c_fb_offset = c_fb_charRowOffset + c_fb_charColOffset;
    const int c_ta_hpos = atlasPosition(c);
    //SDL_PixelFormat const* format = frameBuffer.Get()->format;

    size_t idx;
    for (int i = 0; i < FONT_H; i++) {
        for (int j = 0; j < FONT_W; j++) {
            idx = c_ta_hpos + j + i * c_ta_pxPerLine;
            SDL_Color color = interpolateColor(colorPalette[attr.bgcolor], colorPalette[attr.fgcolor], atlasPX[idx]/255.0f);

            idx = c_fb_offset + i * c_fb_pxPerLine + j;
            //fbPX[idx] = SDL_MapRGBA(format, color.r, color.g, color.b, color.a);
            fbPX[idx] = (color.r << 24) | (color.g << 16) | (color.b << 8) | (color.a);
        }
    }
#else
    /* ---- HNUS ---- */
    surfacePalette[0] = colorPalette[attr.bgcolor];
    surfacePalette[255] = colorPalette[attr.fgcolor];
    SDL_SetPaletteColors(textAtlas.Get()->format->palette, surfacePalette, 0, 256);
    /* -------------- */

    SDL2pp::Rect fbPos{ col * FONT_W, row * FONT_H, FONT_W, FONT_H };

    textAtlas.Blit(SDL2pp::Rect{ atlasPosition(c), 0, FONT_W, FONT_H }, frameBuffer, fbPos);
#endif
}

void TextRenderer::drawBitmap(FT_Bitmap bitmap, int x, int y) {
    if (bitmap.width == 0 || bitmap.rows == 0) return;
    if (bitmap.width + x > textAtlas.GetWidth() || bitmap.rows + y > textAtlas.GetHeight()) return;

    SDL2pp::Rect dstrect{ x, y, (int)bitmap.width, (int)bitmap.rows };

    SDL2pp::Surface bm { bitmap.buffer, (int)bitmap.width, (int)bitmap.rows, 8, bitmap.pitch, 0, 0, 0, 0 };
    bm.Blit(SDL2pp::NullOpt, textAtlas, dstrect);
}

void TextRenderer::print(int line, int col, const std::string &str) {
    strncpy(&charAt(line, col), str.c_str(), std::min(str.length(), textBuffer.size() - (line * N_COLS + col)));
}

void TextRenderer::print(int line, int col, const std::string &str, const TextRenderer::TextAttributes attr) {
    print(line, col, str);
    TextAttributes *attrBegin = &attrAt(line, col);
    size_t length = std::min(str.length(), textBuffer.size() - (line * N_COLS + col));
    for (size_t i = 0; i < length; i++) {
        attrBegin[i] = attr;
    }
}

SDL_Color TextRenderer::interpolateColor(SDL_Color from, SDL_Color to, float percent) {
    // c = a + (b - a) * t
    SDL_Color out;

    out.r = from.r + (to.r - from.r) * percent;
    out.g = from.g + (to.g - from.g) * percent;
    out.b = from.b + (to.b - from.b) * percent;
    out.a = from.a + (to.a - from.a) * percent;

    return out;
}

TextRenderer::RawInfo TextRenderer::getRawInfo() {
    return RawInfo {
        .text = textBuffer.data(),
        .attrs = attrBuffer.data(),
        .size = N_LINES * N_COLS
    };
}
