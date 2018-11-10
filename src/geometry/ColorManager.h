#pragma once

#include <glm/glm.hpp>
#include <random>
#include <vector>

#define PEPR3D_MAX_PALETTE_COLORS 4

namespace pepr3d {

class ColorManager {
   public:
    using ColorMap = std::vector<glm::vec4>;

   private:
    /// A vector containing all the colors that the application supports at this time
    ColorMap mColorMap;

   public:
    ColorManager() {
        mColorMap.emplace_back(1, 0, 0, 1);
        mColorMap.emplace_back(0, 1, 0, 1);
        mColorMap.emplace_back(0, 0, 1, 1);
        mColorMap.emplace_back(0.2, 0.2, 0.2, 1);
        assert(mColorMap.size() <= PEPR3D_MAX_PALETTE_COLORS);
    }

    ColorManager(const ColorMap::const_iterator start, const ColorMap::const_iterator end) {
        replaceColors(start, end);
    }

    ColorManager(const size_t number) {
        std::random_device rd;   // Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
        std::uniform_real_distribution<> randomGen(0.0, 1.0);

        for(int i = 0; i < number; ++i) {
            mColorMap.emplace_back(randomGen(gen), randomGen(gen), randomGen(gen), 1);
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

    /// Set the i-th color to a new color
    void setColor(const size_t i, const glm::vec4 newColor) {
        assert(i < mColorMap.size());
        mColorMap[i] = newColor;
    }

    /// Replace the current colors with this list
    void replaceColors(const ColorMap::const_iterator start, const ColorMap::const_iterator end) {
        auto it = start;
        mColorMap.clear();
        mColorMap.reserve(std::min<std::size_t>(end - start, PEPR3D_MAX_PALETTE_COLORS));
        while(it != end && mColorMap.size() < PEPR3D_MAX_PALETTE_COLORS) {
            mColorMap.push_back(*it);
            ++it;
        }
    }

    /// Replace the current colors with this new vector of colors
    void replaceColors(ColorMap&& newColors) {
        mColorMap = std::move(newColors);
        if(mColorMap.size() > PEPR3D_MAX_PALETTE_COLORS) {
            mColorMap.resize(PEPR3D_MAX_PALETTE_COLORS);
        }
    }

    ColorMap& getColorMap() {
        return mColorMap;
    }
};

}  // namespace pepr3d