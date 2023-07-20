#include <iostream>
#include <exception>
#include <chrono>
#include <fmt/format.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <unicode/ucnv.h>
#include <SDL.h>
#include "TextRenderer.hpp"
#include "Z80Computer.hpp"
//#include <CLI/CLI.hpp>

#define FONT_FILE "../data/PxPlus_IBM_MDA.ttf"

using namespace SDL2pp;
using namespace std::chrono;
using std::string;
using fmt::format;

const int FPS = 20;
const int msPerFrame = 1000 / FPS;

class UTF8_CP852_Converter {
public:
    UTF8_CP852_Converter() {
        err = U_ZERO_ERROR;

        from_utf8 = ucnv_open("utf8", &err);
        detectFailure();

        to_cp852 = ucnv_open("cp852", &err);
        detectFailure();
    }

    ~UTF8_CP852_Converter() {
        ucnv_close(from_utf8);
        ucnv_close(to_cp852);
    }

    string operator()(const string& text) {
        std::wstring wstr;
        wstr.resize(text.size());

        int len = ucnv_toUChars(from_utf8, reinterpret_cast<UChar *>(wstr.data()), wstr.size(), text.data(), text.size(), &err);
        detectFailure();
        wstr.resize(len);

        string result;
        result.resize(text.size());

        len = ucnv_fromUChars(to_cp852, result.data(), result.size(), reinterpret_cast<const UChar *>(wstr.c_str()), wstr.size(), &err);
        detectFailure();
        result.resize(len);

        return result;
    }

private:
    void detectFailure() {
        if (U_FAILURE(err)) throw std::runtime_error(u_errorName(err));
    };

    UConverter *from_utf8, *to_cp852;
    UErrorCode err;

};
UTF8_CP852_Converter converter;

template <size_t N>
void renderStrArray(TextRenderer &renderer, int row, int col, const string strings[N], TextRenderer::TextAttributes attrs) {
    string converted;
    for (int i = 0; i < N; i++) {
        converted = converter(strings[i]);
        strcpy(&renderer.charAt(row + i, col), converted.c_str());
        for (int j = 0; j < converted.length(); j++) {
            renderer.attrAt(row + i, col + j) = attrs;
        }
    }
}

//https://stackoverflow.com/a/4063229
size_t utf8length(const char* str) {
    size_t len = 0;
    while (*str) len += (*str++ & 0xc0) != 0x80;
    return len;
}
size_t utf8length(const string& str) { return utf8length(str.c_str()); }

int nDigits10IntegerPart(float num) {
    if (num >= 1000) return floor(log10f(num)) + 1;
    if (num >= 100) return 3;
    if (num >= 10) return 2;
    else return 1;
}

/*
 ┌───────┬─────┤ CPU TEMP ├─────┐
 │ 46 °C │ ███████              │
 └───────┴─┬───┬───┬───┬───┬───┬┘
           20  35  50  65  80  95
 */
void
Gauge(TextRenderer &renderer, int row, int col, const string &label, const string &unit, float valueMin, float valueMax,
      float value, int precision, int gradeTicks, uint8_t color) {
    const float range = valueMax - valueMin;
    float fill;
    if (value <= valueMin) fill = 0;
    else if (value >= valueMax) fill = 1;
    else fill = (value - valueMin) / range;

    const float step = range / (gradeTicks + 1);

    string sLabel = "┤ " + label + " ├";
    string sValue = format("{:.{}g}{}", value, precision + nDigits10IntegerPart(value), unit);

    const int valueLength = utf8length(sValue) + 2;
    const int fillLength = fill * 20;

    std::vector<float> gradation;
    gradation.reserve(2 + gradeTicks);
    float v = valueMin;
    for (int i = 0; i < gradeTicks + 2; i++) {
        gradation.push_back(v);
        v += step;
    }

    string sGradation, sGradationNumbers;
    const int gradationLength = 19;
    const int gradationSpacingLength = ceil(gradationLength / (gradeTicks + 1));

    if (gradeTicks == 0) {
        sGradation.reserve(gradationLength * 4);
        for (int i = 0; i < gradationLength; i++) sGradation += "─";

        ///TODO: dodelat
    }
    else {
        string gradationInfill;
        gradationInfill.reserve(gradationSpacingLength * 4);
        for (int i = 0; i < gradationSpacingLength; i++) gradationInfill += "─";

        for (int i = 0; i < gradeTicks; i++) {
            sGradation.append(gradationInfill);
            sGradation.append("┬");
        }

        int sGradationLength = utf8length(sGradation);
        while (sGradationLength++ < gradationLength) sGradation += "─";

        /*std::vector<int> tickPositions;
        tickPositions.push_back(valueLength + 3);
        for (int i = 0; i < gradeTicks; i++) tickPositions.push_back(tickPositions.back() + gradationSpacingLength + 1);
        tickPositions.push_back(valueLength + 21);*/

        const int gradationVecLength = gradation.size();
        for (int i = 0; i < gradationVecLength; i++) {
            float f = gradation[i];
            string number;
            /*const int nDigitsIntegerPart = nDigits10IntegerPart(f);
            const int remainingNumberLength = gradationSpacingLength - nDigitsIntegerPart - 2; // xxx.0
            int numberPrecision;
            if (remainingNumberLength < 0) numberPrecision = 0;
            else if (roundf(f) == f) numberPrecision = 0;
            else numberPrecision = remainingNumberLength;*/

            //number = format("{:.{}f}", f, numberPrecision);
            number = format("{:.{}g}", f, precision + nDigits10IntegerPart(f));
            sGradationNumbers.append(number);
            if (i < gradationVecLength - 1) {
                const int numberLength = utf8length(number);
                sGradationNumbers.append(gradationSpacingLength - numberLength + 1, ' ');
            }
        }
    }

    string output[4] = {
        format("┌{:─<{}}┬─{:─^20}─┐", "", valueLength, sLabel),
        format("│{: ^{}}│ {:█<{}}{: <{}} │", sValue, valueLength, "", fillLength, "", 20 - fillLength),
        format("└{:─<{}}┴─┬{}┬┘", "", valueLength, sGradation),
        format("{: <{}}{}", "", valueLength + 3, sGradationNumbers)

    };

    TextRenderer::TextAttributes attrs;
    attrs.bgcolor = 0;
    attrs.fgcolor = 0b0111;
    renderStrArray<4>(renderer, row, col, output, attrs);

    for (int i = 0; i < 20; i++) renderer.attrAt(row + 1, col + i + 2 + valueLength).fgcolor = color;
}

void Indicator(TextRenderer &renderer, int row, int col, const string& label, TextRenderer::TextAttributes attrs) {
    string labelConverted = converter(label);
    int sep = labelConverted.find('\n');
    std::string_view lines[] = {
        {labelConverted.begin(), labelConverted.begin() + sep},
        {labelConverted.begin() + sep + 1, labelConverted.end()}
    };

    for (int i = row; i < row + 2; i++) {
        for (int j = col; j < col + 6; j++) renderer.attrAt(i, j) = attrs;
        strcpy(&renderer.charAt(i, col), format("{:^6}", lines[i - row]).c_str());
    }
}

bool g_hasUART = false, g_UIRedraw = true;

void drawUART(TextRenderer &renderer) {
    /*using df = duration<float>;

    static auto time_start = steady_clock::now();
    static bool isHBLit = false;

    auto& hbpos = renderer.attrAt(-1, 7);*/
    auto rs232pos = &renderer.attrAt(-1, 0);

    if (!g_hasUART) {
        //isHBLit = false;
        for (int i = 0; i < 7; i++) rs232pos[i].bgcolor = 0b0100;
        //hbpos.fgcolor = 0x7;
    } else {
        //auto now = steady_clock::now();
        for (int i = 0; i < 7; i++) rs232pos[i].bgcolor = 0b0010;
        /*df s = now - time_start;
        if (s.count() > 1) {
            time_start = now;
            if (isHBLit) hbpos.fgcolor = 0xf;
            else hbpos.fgcolor = 0x7;
            isHBLit = !isHBLit;
            g_UIRedraw = true;
        }*/
    }
}

void drawUI(TextRenderer &renderer) {
    renderer.print(0, 0, format("{:<{}}", "", TextRenderer::N_COLS), 0b00011111);
    renderer.print(0, 0, converter(" Základní obrazovka "), 0b10011111);
    renderer.print(0, 20, converter("│ Napájení │ Zatížení PC │ Intel PCM"));

    {
        char* line1c = &renderer.charAt(0, 0);
        TextRenderer::TextAttributes* line1a = &renderer.attrAt(0, 0);
        char sep = converter("│")[0];
        for (int i = 0; i < TextRenderer::N_COLS; i++) {
            if (line1c[i] == sep) line1a[i].fgcolor = 0b0111;
        }
    }

    Gauge(renderer, 2, 2, "TEPLOTA CPU", " °C", 20, 95, 46, 0, 4, 0b0010);
    Gauge(renderer, 7, 2, "TEPLOTA GPU", " °C", 20, 95, 80, 0, 4, 0b0100);
    Gauge(renderer, 12, 2, "TEPLOTA VODY", " °C", 20, 60, 30, 0, 4, 0b0010);
    Gauge(renderer, 2, 40, "PRŮTOK CPU", " l/m", 0, 10, 7.62, 1, 4, 0b0010);
    Gauge(renderer, 7, 40, "PRŮTOK GPU", " l/m", 0, 10, 6, 0, 4, 0b0010);
    Gauge(renderer, 12, 40, "VENTILÁTOR", "", 0, 3000, 1337, 0, 1, 0b0010);

    renderer.print(18, 0, converter(format("{:<{}}", "", TextRenderer::N_COLS)), 0b00010111);

    Indicator(renderer, 20, 2, "PC\nZAP", 0b11000000);
    Indicator(renderer, 20, 10, "ČERP\n1", 0b10100000);
    Indicator(renderer, 20, 18, "ČERP\n2", 0b00000111/*0b01110000*/);
    Indicator(renderer, 20, 26, "PWR\nGOOD", 0b10100000);

    renderer.print(-1, 0, format("{:<{}}", "", TextRenderer::N_COLS), 0b00011111);
    renderer.print(-1, 0, converter(" RS232 "), 0b01001111);
    //renderer.print(-1, 7, converter("■"), 0b00010111);

    drawUART(renderer);
}

int main_u(int argc, char** argv) {
    SDL sdl(SDL_INIT_VIDEO);

    Window window(
        "z80sdl",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        TextRenderer::WIN_W, TextRenderer::WIN_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    Renderer sdlRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );
    TextRenderer textRenderer(sdlRenderer.Get());

    sdlRenderer.SetDrawColor(0, 0, 0);
    window.SetMinimumSize(TextRenderer::WIN_W, TextRenderer::WIN_H);

    textRenderer.makeCharAtlas(FONT_FILE);

    int bufferSize = TextRenderer::N_LINES * TextRenderer::N_COLS;
    memset(&textRenderer.attrAt(0, 0), 0b111, bufferSize);

    while (true) {
        auto startTime = steady_clock::now();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    goto konec;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                        case SDLK_q:
                            goto konec;
                        case SDLK_r:
                            window.SetSize(TextRenderer::WIN_W, TextRenderer::WIN_H);
                            break;
                        case SDLK_SPACE:
                            g_hasUART = !g_hasUART;
                            g_UIRedraw = true;
                            break;
                    }
                    break;
            }
        }

        drawUI(textRenderer);
        if (g_UIRedraw) {
            textRenderer.render();
            g_UIRedraw = false;
        } else textRenderer.present();

        sdlRenderer.Present();

        auto stopTime = steady_clock::now();
        auto elapsedTime = duration_cast<milliseconds>(stopTime - startTime).count();
        if (elapsedTime < msPerFrame) SDL_Delay(msPerFrame - elapsedTime);
    }
    konec:
    return 0;
}

#if 1
int main(int argc, char** argv) {
    try {
        return main_u(argc, argv);
    } catch (std::exception &ex) {
        std::cerr << ex.what() << std::endl;
        return -1;
    }
}
#else
int main() {
    CLI::App app;

    bool test = false;

    app.add_flag("-t,--test", test, "Testovací možnost");

    try {
        app.parse();
    } catch(const CLI::ParseError &e) {
        return app.exit(e);
    }

    print("test: {}\n", test);

    return 0;
}
#endif

/// cp 852
/// ÇüéâäůćçłëŐőîŹÄĆÉĹĺôöĽľŚśÖÜŤťŁ×čáíóúĄąŽžĘę¬źČş«»░▒▓│┤ÁÂĚŞ╣║╗╝Żż┐└┴┬├─┼Ăă╚╔╩╦╠═╬¤đĐĎËďŇÍÎě┘┌█▄ŢŮ▀ÓßÔŃńňŠšŔÚŕŰýÝţ´˝˛ˇ˘§÷¸°¨˙űŘř■