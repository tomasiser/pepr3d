#pragma once
#include "geometry/Geometry.h"

namespace pepr3d {

/// Command that paints a text using orthogonal projection
class CmdPaintText : public CommandBase<Geometry> {
   public:
    using Triangle = DataTriangle::Triangle;
    using Point = DataTriangle::Point;

    std::string_view getDescription() const override {
        return "Paint a text string";
    }

    /// Paint a text string, using orthogonal projection.
    /// @param ray Direction of text projection
    /// @param text Text mesh in world space
    /// @param color Color to paint with
    CmdPaintText(ci::Ray ray, const std::vector<std::vector<FontRasterizer::Tri>>& text, size_t color)
        : CommandBase(true, false), mText{}, mRay{ray}, mColor(color) {
        mText.resize(text.size());
        for(size_t i = 0; i < text.size(); ++i) {
            const std::vector<FontRasterizer::Tri>& letter = text[i];

            mText[i].reserve(letter.size());
            for(size_t j = 0; j < letter.size(); ++j) {
                const FontRasterizer::Tri& tri = letter[j];
                mText[i].emplace_back(Point(tri.a.x, tri.a.y, tri.a.z), Point(tri.b.x, tri.b.y, tri.b.z),
                                      Point(tri.c.x, tri.c.y, tri.c.z));
            }
        }
    }

   protected:
    void run(Geometry& target) const override {
        const auto start = std::chrono::high_resolution_clock::now();

        const size_t textLength = mText.size();
        size_t progress = 0;

        for(const std::vector<Triangle>& letter : mText) {
            target.getProgress().paintTextPercentage = static_cast<float>(progress) / textLength;
            target.paintWithShape(mRay, letter, mColor);
            ++progress;
        }

        const auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> timeMs = end - start;

        target.getProgress().paintTextPercentage = 1.0f;

        CI_LOG_I("Text paint took " + std::to_string(timeMs.count()) + " ms");
    }

    std::vector<std::vector<Triangle>> mText;
    ci::Ray mRay;
    size_t mColor;
};
}  // namespace pepr3d