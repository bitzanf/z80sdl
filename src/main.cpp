#include <iostream>
#include <exception>
#include <chrono>
#include <fmt/format.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <unicode/ucnv.h>

#define SDL_MAIN_HANDLED
#include "TextRenderer.hpp"
#include "Z80Computer.hpp"


#define FONT_FILE "../data/PxPlus_IBM_MDA.ttf"

using namespace SDL2pp;
using namespace std::chrono;
using std::string;

const int FPS = 20;
const int msPerFrame = 1000 / FPS;

string UTF8Convert(const string &utf8txt) {
    UErrorCode err = U_ZERO_ERROR;
    auto detectFailure = [&err]{
        if (U_FAILURE(err)) throw std::runtime_error(u_errorName(err));
    };

    UConverter *conv = ucnv_open("utf8", &err);
    detectFailure();

    std::wstring wstr;
    wstr.resize(utf8txt.size());

    int len = ucnv_toUChars(conv, reinterpret_cast<UChar *>(wstr.data()), wstr.size(), utf8txt.data(), utf8txt.size(), &err);
    detectFailure();
    wstr.resize(len);
    ucnv_close(conv);

    conv = ucnv_open("cp852", &err);
    detectFailure();

    string result;
    result.resize(utf8txt.size());

    len = ucnv_fromUChars(conv, result.data(), result.size(), reinterpret_cast<const UChar *>(wstr.c_str()), wstr.size(), &err);
    detectFailure();
    result.resize(len);

    ucnv_close(conv);
    return result;
}

template <size_t N>
void renderStrArray(TextRenderer &renderer, int row, int col, const string strings[N], TextRenderer::TextAttributes attrs) {
    string converted;
    for (int i = 0; i < N; i++) {
        converted = UTF8Convert(strings[i]);
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
size_t utf8length(const std::string& str) { return utf8length(str.c_str()); }

int nDigits10IntegerPart(float num) {
    if (num >= 1000) return floor(log10f(num)) + 1;
    if (num >= 100) return 3;
    if (num >= 10) return 2;
    else return 1;
}

void
Gauge(TextRenderer &renderer, int row, int col, const string &label, const string &unit, float valueMin, float valueMax,
      float value, int precision, int gradeTicks, uint8_t color) {
    /*
     ┌───────┬─────┤ CPU TEMP ├─────┐
     │ 46 °C │ ███████              │
     └───────┴─┬───┬───┬───┬───┬───┬┘
               20  35  50  65  80  95
     */
    const float range = valueMax - valueMin;
    float fill;
    if (value <= valueMin) fill = 0;
    else if (value >= valueMax) fill = 1;
    else fill = (value - valueMin) / range;

    const float step = range / (gradeTicks + 1);

    string sLabel = "┤ " + label + " ├";
    string sValue = fmt::format("{:.{}g}", value, precision + nDigits10IntegerPart(value));

    sValue += unit;
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
        for (int i = 0; i < gradationLength; i++) sGradation.append("─");

        ///TODO: dodelat
    }
    else {
        string gradationInfill;
        gradationInfill.reserve(gradationSpacingLength * 4);
        for (int i = 0; i < gradationSpacingLength; i++) gradationInfill.append("─");

        for (int i = 0; i < gradeTicks; i++) {
            sGradation.append(gradationInfill);
            sGradation.append("┬");
        }

        int sGradationLength = utf8length(sGradation);
        while (sGradationLength++ < gradationLength) sGradation.append("─");

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

            //number = fmt::format("{:.{}f}", f, numberPrecision);
            number = fmt::format("{:.{}g}", f, precision + nDigits10IntegerPart(f));
            sGradationNumbers.append(number);
            if (i < gradationVecLength - 1) {
                const int numberLength = utf8length(number);
                sGradationNumbers.append(gradationSpacingLength - numberLength + 1, ' ');
            }
        }
    }

    string output[4] = {
        fmt::format("┌{:─<{}}┬─{:─^20}─┐", "", valueLength, sLabel),
        fmt::format("│{: ^{}}│ {:█<{}}{: <{}} │", sValue, valueLength, "", fillLength, "", 20 - fillLength),
        fmt::format("└{:─<{}}┴─┬{}┬┘", "", valueLength, sGradation),
        fmt::format("{: <{}}{}", "", valueLength + 3, sGradationNumbers)

    };

    TextRenderer::TextAttributes attrs;
    attrs.bgcolor = 0;
    attrs.fgcolor = 0b0111;
    renderStrArray<4>(renderer, row, col, output, attrs);

    for (int i = 0; i < 20; i++) renderer.attrAt(row + 1, col + i + 2 + valueLength).fgcolor = color;
}

void Indicator(TextRenderer &renderer, int row, int col, const string& label, TextRenderer::TextAttributes attrs) {
    string labelConverted = UTF8Convert(label);
    int sep = labelConverted.find('\n');
    std::string_view lines[] = {
        {labelConverted.begin(), labelConverted.begin() + sep},
        {labelConverted.begin() + sep + 1, labelConverted.end()}
    };

    for (int i = row; i < row + 2; i++) {
        for (int j = col; j < col + 6; j++) renderer.attrAt(i, j) = attrs;
        strcpy(&renderer.charAt(i, col), fmt::format("{:^6}", lines[i - row]).c_str());
    }
}

void makeFancyShit(TextRenderer &renderer) {
    //const char* cp852upper = "ÇüéâäůćçłëŐőîŹÄĆÉĹĺôöĽľŚśÖÜŤťŁ×čáíóúĄąŽžĘę¬źČş«»░▒▓│┤ÁÂĚŞ╣║╗╝Żż┐└┴┬├─┼Ăă╚╔╩╦╠═╬¤đĐĎËďŇÍÎě┘┌█▄ŢŮ▀ÓßÔŃńňŠšŔÚŕŰýÝţ´ ˝˛ˇ˘§÷¸°¨˙űŘř■ ";
    string str1u8[] = {
        "╔═══════╦───┤ CPU TEMP ├─────┐",
        "║ 48 °C ║ ████████░░░░░░░░░░ │",
        "╚═══════╩────────────────────┘"
    };
    string str1[3];
    for (int i = 0; i < 3; i++) str1[i] = UTF8Convert(str1u8[i]);

    strcpy(&renderer.charAt(4, 2), str1[0].c_str());
    strcpy(&renderer.charAt(5, 2), str1[1].c_str());
    strcpy(&renderer.charAt(6, 2), str1[2].c_str());

    string str2u8[] = {
        "╔═══════╦═══╣ GPU TEMP ╠═════╗",
        //"║ 85 °C ║ ███████████████    ║",
        "║ 85 °C ║ ███████████████░░░ ║",
        "╚═══════╩════════════════════╝"
    };
    string str2[3];
    for (int i = 0; i < 3; i++) str2[i] = UTF8Convert(str2u8[i]);
    /*for (int i = 0; i < 15; i++) str2[1][i+10] = 10;
    for (int i = 0; i < 3; i++) str2[1][i+25] = 9;*/

    strcpy(&renderer.charAt(4, 41), str2[0].c_str());
    strcpy(&renderer.charAt(5, 41), str2[1].c_str());
    strcpy(&renderer.charAt(6, 41), str2[2].c_str());

    string str3u8[] = {
        "┌───────╦═══╣ WATER TEMP ╠═══╗",
        //"│ 37 °C ║ ██████             ║",
        "│ 37 °C ║ ██████░░░░░░░░░░░░ ║",
        "└───────╩════════════════════╝"
    };
    string str3[3];
    for (int i = 0; i < 3; i++) str3[i] = UTF8Convert(str3u8[i]);

    strcpy(&renderer.charAt(8, 2), str3[0].c_str());
    strcpy(&renderer.charAt(9, 2), str3[1].c_str());
    strcpy(&renderer.charAt(10, 2), str3[2].c_str());

    string str4u8[] = {
        "┌───────┬───┤ FAN SPEED ├────┐",
        //"│ 1337  │ █████████          │",
        "│ 1337  │ █████████░░░░░░░░░ │",
        "└───────┴────────────────────┘"
    };
    string str4[3];
    for (int i = 0; i < 3; i++) str4[i] = UTF8Convert(str4u8[i]);

    strcpy(&renderer.charAt(8, 41), str4[0].c_str());
    strcpy(&renderer.charAt(9, 41), str4[1].c_str());
    strcpy(&renderer.charAt(10, 41), str4[2].c_str());

    for (int i = 0; i < 18; i++) {
        renderer.attrAt(5, i+12).as_byte = 0b00000001;
        renderer.attrAt(5, i+51).as_byte = 0b00000100;
        renderer.attrAt(9, i+12).as_byte = 0b00000010;
        renderer.attrAt(9, i+51).as_byte = 0b00001110;
    }

    string str5u8[] = {
        "        ",
        " PUMP 1 ",
        "        "
    };
    string str5[3];
    for (int i = 0; i < 3; i++) str5[i] = UTF8Convert(str5u8[i]);

    for (int i = 0; i < 3; i++) {
        strcpy(&renderer.charAt(13 + i, 20), str5[i].c_str());
        for (int j = 0; j < str5[i].length(); j++) {
            auto& attr = renderer.attrAt(13 + i, 20 + j);
            //attr.fgcolor = 0;
            //attr.bgcolor = 0b0111;
            attr.fgcolor = 0b0111;
            attr.bgcolor = 0;
        }
    }

    string str6u8[] = {
        "        ",
        " PUMP 2 ",
        "        "
    };
    string str6[3];
    for (int i = 0; i < 3; i++) str6[i] = UTF8Convert(str6u8[i]);

    for (int i = 0; i < 3; i++) {
        strcpy(&renderer.charAt(13 + i, 30), str6[i].c_str());
        for (int j = 0; j < str6[i].length(); j++) {
            auto& attr = renderer.attrAt(13 + i, 30 + j);
            attr.fgcolor = 0;
            attr.bgcolor = 0b1010;
        }
    }

    string str7u8[] = {
        " PWR  ",
        " GOOD "
    };
    renderStrArray<2>(renderer, 13, 40, str7u8, 0b11000000);

    string str8[] = {
        "┌───────┬─────┤ CPU TEMP ├─────┐",
        "│ 46 °C │ ███████              │",
        "└───────┴─┬───┬───┬───┬───┬───┬┘",
        "          20  35  50  65  80  95 ",
        "",
        "┌───────┬─────┤ GPU TEMP ├─────┐",
        "│ 80 °C │ █████████████████    │",
        "└───────┴─┬───┬───┬───┬───┬───┬┘",
        "          20  35  50  65  80  95 ",
    };
    renderStrArray<9>(renderer, 19, 2, str8, 0b00000111);
    for (int i = 12; i <= 31; i++) {
        renderer.attrAt(20, i).fgcolor = 0b0010;
        renderer.attrAt(25, i).fgcolor = 0b0100;
    }

    /*string str9[] = {
        "┌┤ CPU ├┬──────────────────────┐",
        "│ 46 °C │ ███████              │",
        "└───────┴─┬───┬───┬───┬───┬───┬┘",
        "         20  35  50  65  80  95",
        "┌┤ GPU ├┬─┴───┴───┴───┴───┴───┴┐",
        "│ 80 °C │ █████████████████    │",
        "└───────┴──────────────────────┘",
    };
    renderStrArray<7>(renderer, 19, 40, str9, TextRenderer::TextAttributes{.as_byte = 0b00000111});
    for (int i = 50; i <= 70; i++) {
        renderer.attrAt(20, i).fgcolor = 0b0010;
        renderer.attrAt(24, i).fgcolor = 0b0100;
    }*/

    Gauge(renderer, 19, 40, "CPU TEMP", "°C", 20, 95, 46, 0, 4, 0b0010);
    Gauge(renderer, 24, 40, "12V POWER", " V", 11, 13, 12.35, 0, 2, 0b0010);
    Indicator(renderer, 13, 48, "COMP\nON", 0b10100000);
}

bool g_hasUART = false;

void drawUART(TextRenderer &renderer) {
    using df = duration<float>;

    static auto time_start = steady_clock::now();
    static bool isHBLit = false;

    if (!g_hasUART) {
        isHBLit = false;
        renderer.print(-1, 0, UTF8Convert(" RS232 "), 0b01001111);
        renderer.print(-1, 7, UTF8Convert("■"), 0b00010111);
    } else {
        auto rs232pos = &renderer.attrAt(-1, 0);
        for (int i = 0; i < 7; i++) rs232pos[i].bgcolor = 0b0010;

        auto& hbpos = renderer.attrAt(-1, 7);
        auto now = steady_clock::now();
        df s = now - time_start;
        if (s.count() > 1) {
            time_start = now;
            if (isHBLit) hbpos.fgcolor = 0xf;
            else hbpos.fgcolor = 0x7;
            isHBLit = !isHBLit;
        }
    }
}

void drawUI(TextRenderer &renderer) {
    renderer.print(0, 0, fmt::format("{:<{}}", "", TextRenderer::N_COLS), 0b00011111);
    renderer.print(0, 0, UTF8Convert(" Základní obrazovka "), 0b10011111);
    renderer.print(0, 20, UTF8Convert("│ Napájení │ Zatížení PC │ Intel PCM"));

    {
        char* line1c = &renderer.charAt(0, 0);
        TextRenderer::TextAttributes* line1a = &renderer.attrAt(0, 0);
        char sep = UTF8Convert("│")[0];
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

    renderer.print(18, 0, UTF8Convert(fmt::format("{:<{}}", "", TextRenderer::N_COLS)), 0b00010111);

    Indicator(renderer, 20, 2, "PC\nZAP", 0b11000000);
    Indicator(renderer, 20, 10, "ČERP\n1", 0b10100000);
    Indicator(renderer, 20, 18, "ČERP\n2", 0b00000111/*0b01110000*/);
    Indicator(renderer, 20, 26, "PWR\nGOOD", 0b10100000);

    renderer.print(20, 40, "        ", 0b10100000);
    renderer.print(21, 40, UTF8Convert(" ČERP 1 "), 0b10100000);
    renderer.print(22, 40, "        ", 0b10100000);

    renderer.print(-1, 8, fmt::format("{:<{}}", "", TextRenderer::N_COLS-8), 0b00011111);

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

    drawUI(textRenderer);

    sdlRenderer.Clear();
    textRenderer.render();

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
                            break;
                    }
                    break;
            }
        }

        drawUI(textRenderer);
        textRenderer.render();
        //textRenderer.present();
        sdlRenderer.Present();

        auto stopTime = steady_clock::now();
        auto elapsedTime = duration_cast<milliseconds>(stopTime - startTime).count();
        if (elapsedTime < msPerFrame) SDL_Delay(msPerFrame - elapsedTime);
    }
    konec:
    return 0;
}

int main(int argc, char** argv) {
    try {
        return main_u(argc, argv);
    } catch (std::exception &ex) {
        std::cerr << ex.what() << std::endl;
        return -1;
    }
}

// iconv na konverzi <0-0xff> -> unicode
// https://unicode-org.github.io/icu/userguide/conversion/converters.html#modes-of-conversion
// lib icu

//cp 852
// ÇüéâäůćçłëŐőîŹÄĆÉĹĺôöĽľŚśÖÜŤťŁ×čáíóúĄąŽžĘę¬źČş«»░▒▓│┤ÁÂĚŞ╣║╗╝Żż┐└┴┬├─┼Ăă╚╔╩╦╠═╬¤đĐĎËďŇÍÎě┘┌█▄ŢŮ▀ÓßÔŃńňŠšŔÚŕŰýÝţ´
// <SHY>
// ˝˛ˇ˘§÷¸°¨˙űŘř■
// <NBSP>

//const char* cp852upper = "ÇüéâäůćçłëŐőîŹÄĆÉĹĺôöĽľŚśÖÜŤťŁ×čáíóúĄąŽžĘę¬źČş«»░▒▓│┤ÁÂĚŞ╣║╗╝Żż┐└┴┬├─┼Ăă╚╔╩╦╠═╬¤đĐĎËďŇÍÎě┘┌█▄ŢŮ▀ÓßÔŃńňŠšŔÚŕŰýÝţ´ ˝˛ˇ˘§÷¸°¨˙űŘř■ ";

/* https://en.wikipedia.org/wiki/Code_page_852
 * let tbl = document.querySelector("table.wikitable");
 * let chars = tbl.querySelectorAll("td > a");
 * let str = "";
 * chars.forEach(el=>{str+=el.innerHTML;});
 */