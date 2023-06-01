//
// Created by Filip on 19.4.2023.
//

#ifndef Z80SDL_FTWRAPPER_HPP
#define Z80SDL_FTWRAPPER_HPP

#include <memory>
#include <system_error>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace ftpp {
    void FTErrorCheck(FT_Error err);
    void FTErrorCheck(FT_Error err, const std::string &message);

    struct FTError {
        explicit inline FTError(FT_Error error) : err(error) {}

        [[nodiscard]] const char* getErrorMessage() const;

        FT_Error err = 0;
    };

    struct FTLib {
        FTLib() {
            FTErrorCheck(FT_Init_FreeType(&lib), "Error initializing library");
        }

        FTLib(const FTLib &other) = delete;
        FTLib(const FTLib &&other) = delete;

        ~FTLib() {
            if (lib) {
                FT_Done_FreeType(lib);
                lib = nullptr;
            }
        }

        inline FT_Library get() { return lib; }
        inline operator FT_Library() { return lib; }
        
    private:
        FT_Library lib = nullptr;
    };

    
    struct FTFace {
        FTFace(ftpp::FTLib &lib, const char* filePath, FT_Long faceIndex = 0) {
            FTErrorCheck(FT_New_Face(lib.get(), filePath, faceIndex, &face), "Error opening font face");
        }

        FTFace(FTFace &other) {
            other.incRef();
            face = other.face;
        }

        FTFace(FTFace &&other) noexcept {
            face = other.face;
            other.face = nullptr;
        }

        ~FTFace() {
            if (face) try {
                decRef();
                face = nullptr;
            } catch (...) { }
        }
        
        inline void loadGlyph(FT_UInt glyphIndex, FT_Int32 loadFlags) {
            FTErrorCheck(FT_Load_Glyph(face, glyphIndex, loadFlags), "Error loading glyph");
        }
        
        inline void renderGlyph(FT_Render_Mode_ renderMode) {
            FTErrorCheck(FT_Render_Glyph(face->glyph, renderMode), "Error rendering glyph");
        }

        inline void loadChar(FT_ULong charCode, FT_Int32 loadFlags) {
            FTErrorCheck(FT_Load_Char(face, charCode, loadFlags), "Error loading character");
        }

        inline void selectCharMap(FT_Encoding encoding) {
            FTErrorCheck(FT_Select_Charmap(face, encoding), "Error selecting CharMap");
        }

        inline void setPixelSizes(FT_UInt width, FT_UInt height) {
            FTErrorCheck(FT_Set_Pixel_Sizes(face, width, height), "Error setting pixel sizes");
        }

        inline void setPixelSizes(FT_UInt height) { setPixelSizes(0, height); }
        inline FT_UInt getCharIndex(FT_ULong charCode) { return FT_Get_Char_Index(face, charCode); }
        inline void incRef() { FTErrorCheck(FT_Reference_Face(face)); }
        inline void decRef() { FTErrorCheck(FT_Done_Face(face)); }
        
        inline FT_Face get() { return face; }
        inline operator FT_Face() { return face; }
        inline FT_Face operator ->() { return face; }

    private:
        FT_Face face = nullptr;
    };

    struct sFTErrorCategory : std::error_category {
        [[nodiscard]] const char* name() const noexcept override { return "FreeType"; }
        [[nodiscard]] std::string message(int ev) const noexcept override {
            return ftpp::FTError(ev).getErrorMessage();
        }
    };
    const sFTErrorCategory FTErrorCategory{};
}

// https://akrzemi1.wordpress.com/2017/07/12/your-own-error-code/

namespace std {
    template <>
    struct is_error_code_enum<ftpp::FTError> : true_type {};

    std::error_code make_error_code(ftpp::FTError err);
}

#endif //Z80SDL_FTWRAPPER_HPP
