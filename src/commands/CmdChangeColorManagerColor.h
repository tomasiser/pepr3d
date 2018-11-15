#pragma once
#include "commands/Command.h"
#include "geometry/Geometry.h"

namespace pepr3d {
class CmdChangeColorManagerColor : public CommandBase<Geometry> {
   public:
    CmdChangeColorManagerColor(size_t colorIdx, glm::vec4 color)
        : CommandBase(false, true), mColorIdx(colorIdx), mColor(color) {}

    std::string_view getDescription() const override {
        return "Change one of the colors";
    }

   protected:
    void run(Geometry& target) const override {
        target.getColorManager().setColor(mColorIdx, mColor);
    }

    bool joinCommand(const CommandBase& otherBase) override {
        const auto* other = dynamic_cast<const CmdChangeColorManagerColor*>(&otherBase);
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

}  // namespace pepr3d
