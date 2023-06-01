//
// Created by Filip on 19.4.2023.
//

#include "FTRenderer.hpp"
#include <unicode/ucnv.h>

FTRenderer::FTRenderer(SDL2pp::Renderer &_renderer, ftpp::FTFace &_face, int lines, int columns)
    : renderer(_renderer)
    , face(_face)
    , glyphBuffer(mkGlyphBuffer(128, 128))
    , drawSurface(nullptr)
    , texture(nullptr)
    , nLines(lines)
    , nColumns(columns)
    {
    resize(renderer.GetOutputWidth(), renderer.GetOutputHeight());
    textBuffer.resize(lines * columns);
    attributeBuffer.resize(lines * columns);

    for (int i = 0; i < 256; i++) palette[i].a = i;

    for (int i = 0; i < 8; i++) {
        textColors[i].r = (i & 0b100) ? 0x7f : 0;
        textColors[i].g = (i & 0b010) ? 0x7f : 0;
        textColors[i].b = (i & 0b001) ? 0x7f : 0;
        textColors[i].a = 0xff;
    }

    for (int i = 9; i < 16; i++) {
        textColors[i].r = (i & 0b100) ? 0xff : 0;
        textColors[i].g = (i & 0b010) ? 0xff : 0;
        textColors[i].b = (i & 0b001) ? 0xff : 0;
        textColors[i].a = 0xff;
    }

    mkGlyphBuffer(128, 128);
}

char &FTRenderer::charAt(int line, int column) {
    return textBuffer.at(linearBufferIdx(line, column));
}

int FTRenderer::linearBufferIdx(int line, int column) const {
    if (line > nLines || column > nColumns || line < 0 || column < 0) throw std::length_error("Wrong buffer index specified!");
    return nColumns * line + column;
}

void FTRenderer::render() {
    auto metrics = face->size->metrics;
    float w = metrics.x_ppem / 2.0;
    float h = metrics.y_ppem;
    drawSurface->FillRect(SDL2pp::NullOpt, 0);

    for (int line = 0; line < nLines; line++) {
        for (int column = 0; column < nColumns; column++) {
            TextAttributes attrs = attrAt(line, column);
            SDL2pp::Rect bgRect{(int)roundf(column * w), (int)(line * h), (int)roundf(w), (int)h};

            renderer.SetDrawColor(textColors[attrs.bgcolor]);
            renderer.FillRect(bgRect);
        }
    }

    for (int line = 0; line < nLines; line++) {
        for (int column = 0; column < nColumns; column++) {
            renderChar(line, column);
        }
    }

    texture->Update(SDL2pp::NullOpt, *drawSurface);
}

void FTRenderer::mkMapForCharset(const std::string &charset) {
    UErrorCode err = U_ZERO_ERROR;
    auto detectFailure = [&err]{
        if (U_FAILURE(err)) throw std::runtime_error(u_errorName(err));
    };

    UConverter *conv = ucnv_open(charset.c_str(), &err);
    detectFailure();

    UChar32 character;
    std::u32string result;
    char charsetStr[2];
    charsetStr[1] = 0;

    char *source, *sourceLimit;

    charsetMapping.clear();
    for (unsigned char c = 0; c < 0xff; c++) {
        charsetStr[0] = c;
        source = charsetStr;
        sourceLimit = charsetStr + 1;

        while (source < sourceLimit) {
            character = ucnv_getNextUChar(conv, (const char**)&source, sourceLimit, &err);
            detectFailure();
            result.push_back(character);
        }

        charsetMapping.insert({c, result});
        result.clear();
    }

    ucnv_close(conv);
}

void FTRenderer::renderChar(int line, int column) {
    std::u32string& codepoints = charsetMapping[charAt(line, column)];
    auto metrics = face->size->metrics;

    drawGlyphSlot(
        codepoints,
        textColors[attrAt(line, column).fgcolor],
        //roundf(column * metrics.x_ppem / 2.0),
        column * metrics.x_ppem / 2,
        (line + 1) * metrics.y_ppem + (metrics.descender >> 6)
    );
}

size_t FTRenderer::bufferSize() {
    return textBuffer.size();
}

char *FTRenderer::textBufferPosition(int line, int column) {
    return textBuffer.data() + linearBufferIdx(line, column);
}

FTRenderer::TextAttributes *FTRenderer::attributeBufferPosition(int line, int column) {
    return attributeBuffer.data() + linearBufferIdx(line, column);
}

FTRenderer::TextAttributes &FTRenderer::attrAt(int line, int column) {
    return attributeBuffer.at(linearBufferIdx(line, column));
}

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

void FTRenderer::drawBitmap(int x, int y, SDL_Color color) {
    FT_Bitmap bm = face->glyph->bitmap;

    if (bm.width == 0 || bm.rows == 0) return;
    if (bm.width + x > drawSurface->GetWidth() || bm.rows + y > drawSurface->GetHeight()) return;

    if (bm.width > glyphBuffer.GetWidth() || bm.rows > glyphBuffer.GetHeight()) glyphBuffer = mkGlyphBuffer(bm.width * 2, bm.rows * 2);

    SDL2pp::Rect dstrect{ x, y, (int)bm.width, (int)bm.rows };
    SDL2pp::Rect srcrect{0, 0, (int)bm.width, (int)bm.rows };
    SDL_PixelFormat *fmt = glyphBuffer.Get()->format;
    auto *pixels = (Uint32 *)glyphBuffer.Get()->pixels;
    int pxw = glyphBuffer.GetWidth();

    for (int row = 0; row < bm.rows; row++) {
        for (int col = 0; col < bm.width; col++) {
            Uint32 c = SDL_MapRGBA(fmt, color.r, color.g, color.b, bm.buffer[row * bm.width + col]);
            pixels[row * pxw + col] = c;
        }
    }

    glyphBuffer.Blit(srcrect, *drawSurface, dstrect);
}

SDL2pp::Surface FTRenderer::mkGlyphBuffer(int w, int h) {
    SDL2pp::Surface bfr{ SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA8888)};
    bfr.FillRect(SDL2pp::NullOpt, 0);
    bfr.SetBlendMode(SDL_BLENDMODE_BLEND);
    return bfr;
}

void FTRenderer::resize(int w, int h) {
    face.setPixelSizes(h / nLines);
    reallocDrawArea(w, h);
}

SDL2pp::Point FTRenderer::resizeText(int charH) {
    face.setPixelSizes(charH);
    SDL2pp::Point sz{ (int)roundf(face->size->metrics.x_ppem * nColumns / 2.0), charH * nLines };
    reallocDrawArea(sz.x, sz.y);

    return sz;
}

void FTRenderer::reallocDrawArea(int w, int h) {
    delete drawSurface;
    delete texture;

    drawSurface = new SDL2pp::Surface{ SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA8888) };
    texture = new SDL2pp::Texture{renderer, *drawSurface};
}

void FTRenderer::present() {
    renderer.Copy(*texture);
}

FTRenderer::~FTRenderer() {
    delete drawSurface;
    delete texture;

    drawSurface = nullptr;
    texture = nullptr;
}

bool FTRenderer::TextAttributes::operator != (const FTRenderer::TextAttributes other) const {
    return as_byte != other.as_byte;
}
