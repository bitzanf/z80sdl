#include <iostream>
#include <exception>

#define SDL_MAIN_HANDLED
#include "FTRenderer.hpp"
#include "TextRenderer.hpp"
#include "Z80Computer.hpp"

#include <chrono>
#include <fmt/format.h>

#include <ft2build.h>
#include FT_FREETYPE_H
//#include "SDL_syswm.h"

//#define FONT_FILE R"(c:\Users\Filip\AppData\Local\Microsoft\Windows\Fonts\PxPlus_IBM_MDA.ttf)"
#define FONT_FILE "../data/PxPlus_IBM_MDA.ttf"
//#define FONT_FILE R"(c:\Windows\Fonts\Consola.ttf)"
//#define FONT_FILE R"(c:\Windows\Fonts\seguiemj.ttf)"

//#include <windows.h>
/*#include <ShObjIdl.h>
LOGFONTA getFontDialog();
std::string callFileOpenDialog(const wchar_t* title, COMDLG_FILTERSPEC* fs = nullptr, int nFS = 0, const char* startPath = nullptr);*/

// c:\Users\Filip\AppData\Local\Microsoft\Windows\Fonts\

// https://github.com/libSDL2pp/libSDL2pp-tutorial
using namespace SDL2pp;
using namespace std::chrono;

const int FPS = 30;
const int msPerFrame = 1000 / FPS;

void render(Surface& surface, uint64_t frame) {
    //float progress = (frame % FPS) / ((float)FPS);
    float progress = abs(sin(frame / 90.0 * M_PI)) * 0.8;
    uint8_t color = 0xff * progress;
    surface.FillRect(Rect{0, 0, 640, 480}, color | (color << 8) | (color << 16));

    surface.FillRect(Rect{640 - 32 - 93, 480 - 64, 32, 32}, 0xff0000);
    surface.FillRect(Rect{640 - 32 - 64, 480 - 64, 32, 32}, 0x00ff00);
    surface.FillRect(Rect{640 - 32 - 32, 480 - 64, 32, 32}, 0x0000ff);
}

int main_u2(int argc, char** argv) {
    SDL sdl(SDL_INIT_VIDEO);
    SDLTTF ttf;

    Window window(
        "z80sdl",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_RESIZABLE
    );
    Renderer renderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );
    Surface surface{0, 640, 480, 24, 0, 0, 0, 0};
    Texture texture{renderer, surface};

    //Font font("c:/windows/fonts/arial.ttf", 14);
    //font.SetStyle(TTF_STYLE_BOLD);
    Font font(FONT_FILE, 16);

    //LOGFONTA fontSpec = getFontDialog();
    //auto fontPath = callFileOpenDialog(L"Choose a font file", nullptr, 0, R"( c:\Users\Filip\AppData\Local\Microsoft\Windows\Fonts\ )");

    uint64_t frameCounter = 0;
    auto loopStartTime = steady_clock::now();
    surface.FillRect(NullOpt, 0);
    surface.SetBlendMode(SDL_BLENDMODE_NONE);

    while (true) {
        auto startTime = steady_clock::now();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                goto konec;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                    case SDLK_q:
                        goto konec;
                }
            }
        }

        auto stopTime = steady_clock::now();
        auto elapsedTime = duration_cast<milliseconds>(stopTime - startTime).count();
        if (elapsedTime < msPerFrame) SDL_Delay(msPerFrame - elapsedTime);

        renderer.Clear();
        render(surface, frameCounter++);
        texture.Update(Rect{0, 0, 640, 480}, surface);
        renderer.Copy(texture);

        Texture texts[2] = {
        Texture{ renderer, font.RenderText_Shaded(
            /*fmt::format(" frame: {} ", frameCounter),*/ " frame: ((linker error))",
            SDL_Color{ 0xff, 0xff, 0x00, 0xff},
            SDL_Color{ 0, 0, 0, 0xff})
            },
        Texture{
            renderer,font.RenderText_Shaded(
                /*fmt::format(" elapsed time: {:.2f} ",duration_cast<duration<float>>(stopTime - loopStartTime).count()),*/ " elapsed time: ((linker error))",
                SDL_Color{ 0xff, 0xff, 0x00, 0xff},
                SDL_Color{ 0, 0, 0, 0xff})
            }
        };

        int totalHeight = 0, currentHeight = 0;
        for (const auto& tex : texts) totalHeight += tex.GetHeight();

        for (auto& tex : texts) {
            renderer.Copy(
                tex,
                NullOpt,
                Rect{
                    (640 - tex.GetWidth()) / 2,
                    (480 - totalHeight - tex.GetHeight()) / 2 + currentHeight,
                    tex.GetWidth(),
                    tex.GetHeight()
                }
            );
            currentHeight += tex.GetHeight();
        }

        renderer.Present();
    }
    konec:
    return 0;
}

struct RedrawContext {
    Renderer *sdlRenderer;
    FTRenderer *ftRenderer;
    Window *window;
};

int redrawHandler(void *data, SDL_Event *event) {
    if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_EXPOSED) {
        auto ctx = (RedrawContext*)data;
        if (ctx->window->Get() == SDL_GetWindowFromID(event->window.windowID)) {
            ctx->sdlRenderer->SetDrawColor();
            ctx->sdlRenderer->Clear();
            ctx->ftRenderer->render();
            ctx->sdlRenderer->Present();
        }
    }

    return 0;
}

int main_u3(int argc, char** argv) {
    SDL sdl(SDL_INIT_VIDEO);

    Window window(
        "z80sdl",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_SHOWN
    );
    Renderer sdlRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );
    sdlRenderer.SetDrawColor(0, 0, 0);
    int fontSize = 16;

    ftpp::FTLib ftLib;
    ftpp::FTFace ftFace{ ftLib, FONT_FILE };
    ftFace.setPixelSizes(fontSize);
    FTRenderer ftRenderer(sdlRenderer, ftFace, 30, 80);
    ftRenderer.mkMapForCharset("cp852");
    memset(ftRenderer.textBufferPosition(0, 0), 0, ftRenderer.bufferSize());
    strcpy(ftRenderer.textBufferPosition(1, 1), "ahoj svete");
    strcpy(ftRenderer.textBufferPosition(0, 0), "Microsoft Windows [Version 10.0.19044.2846]");
    memset(ftRenderer.attributeBufferPosition(0, 0), 0b111, ftRenderer.bufferSize());
    ftRenderer.attrAt(1, 1).fgcolor = 0b1111;
    ftRenderer.attrAt(1, 2).as_byte = 0b01001101;
    ftRenderer.attrAt(1, 4).as_byte = 0b10101111;
    strcpy(ftRenderer.textBufferPosition(29, 0), "meow meow");
    ftRenderer.attrAt(29, 79).bgcolor = 6;
    ftRenderer.attrAt(0, 0).bgcolor = 6;

    auto bfr = ftRenderer.textBufferPosition(10, 0);
    for (int i = 0; i < 256; i++) bfr[i] = i;

    bfr = ftRenderer.textBufferPosition(20, 0);
    auto abfr = ftRenderer.attributeBufferPosition(20, 0);
    for (int i = 0; i < 256; i++) {
        bfr[i] = i;
        abfr[i].bgcolor = 0b1100;
        abfr[i].fgcolor = 0b1111;
    }

    for (int i = 0; i < 8; i++) {
        strcpy(ftRenderer.textBufferPosition(2, i * 10), "012345678 ");
    }

    /*RedrawContext ctx {
        .sdlRenderer = &sdlRenderer,
        .ftRenderer = &ftRenderer,
        .window = &window
    };
    SDL_AddEventWatch(redrawHandler, &ctx);*/
    /*HMENU menuBar = CreateMenu();
    AppendMenuA(menuBar, MF_STRING, 1, "Menu Item1");
    AppendMenuA(menuBar, MF_STRING, 2, "Menu Item2");
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window.Get(), &wmInfo);
    SetMenu(wmInfo.info.win.window, menuBar);
    SDL_EventState(SDL_SYSWMEVENT, 1);
    window.SetSize(640, 480);*/

    bool RctrlPressed = false, LctrlPressed = false, LshiftPressed = false, RshiftPressed = false;
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
                            window.SetSize(640, 480);
                            ftRenderer.resize(640, 480);
                            fontSize = 16;
                            break;
                        case SDLK_LCTRL:
                            LctrlPressed = true;
                            break;
                        case SDLK_RCTRL:
                            RctrlPressed = true;
                            break;
                        case SDLK_LSHIFT:
                            LshiftPressed = true;
                            break;
                        case SDLK_RSHIFT:
                            RshiftPressed = true;
                            break;
                    }
                    break;
                case SDL_KEYUP:
                    switch (event.key.keysym.sym) {
                        case SDLK_LCTRL:
                            LctrlPressed = false;
                            break;
                        case SDLK_RCTRL:
                            RctrlPressed = false;
                            break;
                        case SDLK_LSHIFT:
                            LshiftPressed = false;
                            break;
                        case SDLK_RSHIFT:
                            RshiftPressed = false;
                            break;
                    }
                case SDL_MOUSEWHEEL:
                    if (LctrlPressed || RctrlPressed) {
                        Point pt;
                        if (event.wheel.y < 0 && fontSize > 1) {
                            if (LshiftPressed || RshiftPressed) fontSize -= 2;
                            else fontSize--;
                            pt = ftRenderer.resizeText(fontSize);
                            window.SetSize(pt);
                        } else if (event.wheel.y > 0) {
                            if (LshiftPressed || RshiftPressed) fontSize += 2;
                            else fontSize++;
                            pt = ftRenderer.resizeText(fontSize);
                            window.SetSize(pt);
                        }
                    }
                    break;
                /*case SDL_SYSWMEVENT:
                    SDL_SysWMmsg *msg = event.syswm.msg;
                    if (msg->msg.win.msg == WM_COMMAND) {
                        int id = LOWORD(msg->msg.win.wParam);
                        if (id == 1) MessageBoxA(NULL, "Clicked 1", "menu click", MB_OK);
                        else if (id == 2) MessageBoxA(NULL, "Clicked 2", "menu click", MB_OK);
                    }
                    break;*/
                /*case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        ftRenderer.resize(event.window.data1, event.window.data2);
                    }*/
            }
        }

        sdlRenderer.SetDrawColor(0, 0, 0);
        sdlRenderer.Clear();
        ftRenderer.render();
        ftRenderer.present();
        sdlRenderer.Present();

        auto stopTime = steady_clock::now();
        auto elapsedTime = duration_cast<milliseconds>(stopTime - startTime).count();
        if (elapsedTime < msPerFrame) SDL_Delay(msPerFrame - elapsedTime);
    }
    konec:
    return 0;
}

int main_u(int argc, char** argv) {
    SDL sdl(SDL_INIT_VIDEO);

    Window window(
        "z80sdl",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_SHOWN
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
    memset(&textRenderer.charAt(0, 0), 0, bufferSize);
    strcpy(&textRenderer.charAt(1, 1), "ahoj svete");
    strcpy(&textRenderer.charAt(0, 0), "Microsoft Windows [Version 10.0.19044.2846]");
    memset(&textRenderer.attrAt(0, 0), 0b111, bufferSize);
    textRenderer.attrAt(1, 1).fgcolor = 0b1111;
    textRenderer.attrAt(1, 2).as_byte = 0b01001101;
    textRenderer.attrAt(1, 4).as_byte = 0b10101111;
    strcpy(&textRenderer.charAt(29, 0), "meow meow");
    textRenderer.attrAt(29, 79).bgcolor = 6;
    textRenderer.attrAt(0, 0).bgcolor = 6;

    auto bfr = &textRenderer.charAt(10, 0);
    for (int i = 0; i < 256; i++) bfr[i] = i;

    bfr = &textRenderer.charAt(20, 0);
    auto abfr = &textRenderer.attrAt(20, 0);
    for (int i = 0; i < 256; i++) {
        bfr[i] = i;
        abfr[i].bgcolor = 0b1100;
        abfr[i].fgcolor = 0b1111;
    }

    for (int i = 0; i < 8; i++) {
        strcpy(&textRenderer.charAt(2, i * 10), "012345678 ");
    }

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
                            window.SetSize(640, 480);
                            break;
                    }
                    break;
            }
        }

        sdlRenderer.SetDrawColor(0, 0, 0);
        sdlRenderer.Clear();
        textRenderer.render();

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