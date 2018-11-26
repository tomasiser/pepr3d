#pragma once

#include <random>
#include <vector>
#include "cinder/Color.h"
#include "glm/glm.hpp"

#define PEPR3D_MAX_PALETTE_COLORS 16

namespace pepr3d {

class ColorManager {
   public:
    using ColorMap = std::vector<glm::vec4>;

   private:
    /// A vector containing all the colors that the application supports at this time
    ColorMap mColorMap;

    /// Index of a currently selected / active color
    size_t mActiveColorIndex = 0;

   public:
    ColorManager() {
        mColorMap.push_back(static_cast<glm::vec4>(ci::ColorA::hex(0x017BDA)));
        mColorMap.push_back(static_cast<glm::vec4>(ci::ColorA::hex(0xEB5757)));
        mColorMap.push_back(static_cast<glm::vec4>(ci::ColorA::hex(0xF2994A)));
        mColorMap.push_back(static_cast<glm::vec4>(ci::ColorA::hex(0x292E33)));
        assert(mColorMap.size() <= PEPR3D_MAX_PALETTE_COLORS);
    }

    ColorManager(const ColorMap::const_iterator start, const ColorMap::const_iterator end) {
        replaceColors(start, end);
    }

    explicit ColorManager(const size_t number) {
        std::random_device rd;   // Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
        std::uniform_real_distribution<> randomGenHue(0.0, 1.0);
        std::uniform_real_distribution<> randomGenValue(0.6, 1.0);

        const float start = static_cast<float>(randomGenHue(gen));
        for(size_t i = 0; i < number; ++i) {
            float added = start + static_cast<float>(i) / static_cast<float>(number) * 0.7f;
            float whole, fractional;
            fractional = std::modf(added, &whole);
            ci::ColorA r = cinder::hsvToRgb(glm::vec4(fractional, 1, randomGenValue(gen), 1));
            mColorMap.emplace_back(r.r, r.g, r.b, 1);
        }
    }

    /// Return the i-th color
    glm::vec4 getColor(const size_t i) const {
        assert(i < mColorMap.size());
        return mColorMap[i];
    }

    /// Returns the number of currently used colors
    size_t size() const {
        return mColorMap.size();
    }

    /// Returns true iff no colors are currently used
    bool empty() const {
        return mColorMap.empty();
    }

    /// Clears all colors and becomes empty
    void clear() {
        mColorMap.clear();
    }

    /// Adds a new color if not above limit
    void addColor(const glm::vec4 newColor) {
        if(size() < PEPR3D_MAX_PALETTE_COLORS) {
            mColorMap.push_back(newColor);
        }
        assert(mColorMap.size() <= PEPR3D_MAX_PALETTE_COLORS);
    }

    /// Set the i-th color to a new color
    void setColor(const size_t i, const glm::vec4 newColor) {
        assert(i < mColorMap.size());
        mColorMap[i] = newColor;
    }

    /// Replace the current colors with this list, trims if above limit
    void replaceColors(const ColorMap::const_iterator start, const ColorMap::const_iterator end) {
        auto it = start;
        mColorMap.clear();
        mColorMap.reserve(std::min<std::size_t>(end - start, PEPR3D_MAX_PALETTE_COLORS));
        while(it != end && mColorMap.size() < PEPR3D_MAX_PALETTE_COLORS) {
            mColorMap.push_back(*it);
            ++it;
        }
        if(mActiveColorIndex >= size()) {
            mActiveColorIndex = size() - 1;
        }
    }

    /// Replace the current colors with this new vector of colors, trims if above limit
    void replaceColors(const ColorMap& newColors) {
        mColorMap = newColors;
        if(mColorMap.size() > PEPR3D_MAX_PALETTE_COLORS) {
            mColorMap.resize(PEPR3D_MAX_PALETTE_COLORS);
        }
        if(mActiveColorIndex >= size()) {
            mActiveColorIndex = size() - 1;
        }
    }

    /// Gets index of the currently selected / active color
    size_t getActiveColorIndex() const {
        return mActiveColorIndex;
    }

    /// Sets index of the currently selected / active color, safely checks boundaries
    void setActiveColorIndex(size_t index) {
        mActiveColorIndex = std::min<size_t>(std::max<size_t>(index, 0), size() - 1);
    }

    ColorMap& getColorMap() {
        return mColorMap;
    }

    const ColorMap& getColorMap() const {
        return mColorMap;
    }
};

}  // namespace pepr3d
