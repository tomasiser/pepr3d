#pragma once

#include <cinder/Color.h>
#include <vector>

namespace pepr3d {

class ColorManager {
    /// A vector containing all the colors that the application supports at this time
    std::vector<ci::ColorA> mColorMap;

   public:
    ColorManager() {
        mColorMap.emplace_back(1, 0, 0, 1);
        mColorMap.emplace_back(0, 1, 0, 1);
        mColorMap.emplace_back(0, 0, 1, 1);
        mColorMap.emplace_back(0.2, 0.2, 0.2, 1);
    }

    ColorManager(const std::vector<ci::ColorA>::const_iterator start,
                 const std::vector<ci::ColorA>::const_iterator end) {
        replaceColors(start, end);
    }

    /// Return the i-th color
    ci::ColorA getColor(const size_t i) const {
        assert(i < mColorMap.size());
        return mColorMap[i];
    }

    /// Returns the number of currently used colors
    size_t size() const {
        return mColorMap.size();
    }

    /// Set the i-th color to a new color
    void setColor(const size_t i, const ci::ColorA newColor) {
        assert(i < mColorMap.size());
        mColorMap[i] = newColor;
    }

    /// Replace the current colors with this list
    void replaceColors(const std::vector<ci::ColorA>::const_iterator start,
                       const std::vector<ci::ColorA>::const_iterator end) {
        auto it = start;
        mColorMap.clear();
        mColorMap.reserve(end - start);
        while(it != end) {
            mColorMap.push_back(*it);
            ++it;
        }
    }

    /// Replace the current colors with this new vector of colors
    void replaceColors(std::vector<ci::ColorA>&& newColors) {
        mColorMap = std::move(newColors);
    }
};

}  // namespace pepr3d