#pragma once

#include <ft2build.h>

#include <freetype/freetype.h>
#include <freetype/ftglyph.h>

#include "ftgl/Point.h"
#include "ftgl/Vectoriser.h"

#pragma warning(push, 0)
#include "poly2tri/poly2tri.h"
#pragma warning(pop)

#include <exception>
#include <memory>
#include <string>

#include <cinder/gl/gl.h>
#include "cinder/Log.h"

namespace pepr3d {

class FontRasterizer {
   public:
    struct Tri {
        glm::vec3 a, b, c;
    };

   private:
    std::string mFontFile;
    bool mFontLoaded = false;

    FT_Library mLibrary;
    FT_Face mFace;

    FT_UInt mPrevCharIndex = 0, mCurCharIndex = 0;
    FT_Pos mPrev_rsb_delta = 0;

   public:
    FontRasterizer(const std::string fontFile) : mFontFile(fontFile) {
        mFontLoaded = true;

        if(FT_Init_FreeType(&mLibrary)) {
            CI_LOG_E("FT_Init_FreeType failed");
            mFontLoaded = false;
        }

        if(FT_New_Face(mLibrary, mFontFile.c_str(), 0, &mFace)) {
            CI_LOG_E("FT_New_Face failed (there is probably a problem with your font file\n");
            mFontLoaded = false;
        }
    };

    std::string getCurrentFont() const {
        return mFontFile;
    }

    bool isValid() const {
        return mFontLoaded;
    }

    std::vector<std::vector<FontRasterizer::Tri>> rasterizeText(const std::string textString, const size_t fontHeight,
                                                                const size_t bezierSteps);

   private:
    double addOneCharacter(const char ch, const size_t bezierSteps, double offset, std::vector<Tri>& outTriangles);

    void outlinePostprocess(std::vector<std::vector<Tri>>& trianglesPerLetter) const;

    static std::vector<p2t::Point*> triangulateContour(Vectoriser* vectoriser, int c, float offset);
};

}  // namespace pepr3d