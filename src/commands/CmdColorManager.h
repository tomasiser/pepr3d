#pragma once

#include <vector>
#include "glm/glm.hpp"

#include "commands/Command.h"
#include "geometry/Geometry.h"

namespace pepr3d {

class CmdColorManagerChangeColor : public CommandBase<Geometry> {
   public:
    CmdColorManagerChangeColor(size_t colorIdx, glm::vec4 color)
        : CommandBase(false, true), mColorIdx(colorIdx), mColor(color) {}

    std::string_view getDescription() const override {
        return "Change a color in the palette";
    }

   protected:
    void run(Geometry& target) const override {
        target.getColorManager().setColor(mColorIdx, mColor);
    }

    bool joinCommand(const CommandBase& otherBase) override {
        const auto* other = dynamic_cast<const CmdColorManagerChangeColor*>(&otherBase);
        if(other) {
            // Keep the newer color if we change the same slot
            if(mColorIdx == other->mColorIdx) {
                mColor = other->mColor;
                return true;
            } else {
                return false;
            }

        } else {
            return false;
        }
    }

    size_t mColorIdx;
    glm::vec4 mColor;
};

class CmdColorManagerSwapColors : public CommandBase<Geometry> {
   public:
    CmdColorManagerSwapColors(size_t color1Idx, size_t color2Idx)
        : CommandBase(false, false), mColor1Idx(color1Idx), mColor2Idx(color2Idx) {}

    std::string_view getDescription() const override {
        return "Swap 2 colors in the palette";
    }

   protected:
    void run(Geometry& target) const override {
        ColorManager& colorManager = target.getColorManager();
        const glm::vec4 temp = colorManager.getColor(mColor1Idx);
        colorManager.setColor(mColor1Idx, colorManager.getColor(mColor2Idx));
        colorManager.setColor(mColor2Idx, temp);
    }

    size_t mColor1Idx;
    size_t mColor2Idx;
};

class CmdColorManagerReorderColors : public CommandBase<Geometry> {
   public:
    CmdColorManagerReorderColors(size_t color1Idx, size_t color2Idx)
        : CommandBase(false, false), mColor1Idx(color1Idx), mColor2Idx(color2Idx) {}

    std::string_view getDescription() const override {
        return "Reorder 2 colors in the palette";
    }

   protected:
    void run(Geometry& target) const override {
        // the beginning is the same as in color swap:
        ColorManager& colorManager = target.getColorManager();
        const glm::vec4 temp = colorManager.getColor(mColor1Idx);
        colorManager.setColor(mColor1Idx, colorManager.getColor(mColor2Idx));
        colorManager.setColor(mColor2Idx, temp);
        // unlike in color swap, we need to preserve model coloring, i.e., swap color IDs in the model:
        for(size_t i = 0; i < target.getTriangleCount(); ++i) {
            const auto& triangle = target.getTriangle(i);
            if(triangle.getColor() == mColor1Idx) {
                target.setTriangleColor(i, mColor2Idx);
            } else if(triangle.getColor() == mColor2Idx) {
                target.setTriangleColor(i, mColor1Idx);
            }
        }
    }

    size_t mColor1Idx;
    size_t mColor2Idx;
};

class CmdColorManagerRemoveColor : public CommandBase<Geometry> {
   public:
    CmdColorManagerRemoveColor(size_t colorIdx) : CommandBase(false, false), mColorIdx(colorIdx) {}

    std::string_view getDescription() const override {
        return "Remove a color from the palette";
    }

   protected:
    void run(Geometry& target) const override {
        ColorManager& colorManager = target.getColorManager();
        assert(mColorIdx + 1 <= colorManager.size());
        ColorManager::ColorMap& colorMap = colorManager.getColorMap();
        colorMap.erase(colorMap.cbegin() + mColorIdx);
        // replace the erased color in the model and shift remaining colors:
        assert(colorManager.size() > 0);
        for(size_t i = 0; i < target.getTriangleCount(); ++i) {
            const auto& triangle = target.getTriangle(i);
            const size_t triangleColor = triangle.getColor();
            if(triangleColor == mColorIdx) {
                target.setTriangleColor(i, 0);
            } else if(triangleColor > mColorIdx) {
                target.setTriangleColor(i, triangleColor - 1);
            }
        }
        // set new active (user selected) color:
        if(colorManager.getActiveColorIndex() == mColorIdx) {
            colorManager.setActiveColorIndex(0);
        }
    }

    size_t mColorIdx;
};

class CmdColorManagerAddColor : public CommandBase<Geometry> {
   public:
    CmdColorManagerAddColor() : CommandBase(false, false) {}

    std::string_view getDescription() const override {
        return "Add a new color to the palette";
    }

   protected:
    void run(Geometry& target) const override {
        ColorManager& colorManager = target.getColorManager();
        assert(colorManager.size() > 0);
        std::random_device rd;   // Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
        uniform_real_distribution<> dis(0.0, 1.0);
        colorManager.addColor(glm::vec4(dis(gen), dis(gen), dis(gen), 1.0f));
    }
};

class CmdColorManagerResetColors : public CommandBase<Geometry> {
   public:
    CmdColorManagerResetColors() : CommandBase(false, false) {}

    std::string_view getDescription() const override {
        return "Reset all colors to default";
    }

   protected:
    void run(Geometry& target) const override {
        ColorManager& colorManager = target.getColorManager();
        colorManager = ColorManager();
        for(size_t i = 0; i < target.getTriangleCount(); ++i) {
            const auto& triangle = target.getTriangle(i);
            const size_t triangleColor = triangle.getColor();
            if(triangleColor >= colorManager.size()) {
                target.setTriangleColor(i, colorManager.size() - 1);
            }
        }
    }
};

}  // namespace pepr3d
