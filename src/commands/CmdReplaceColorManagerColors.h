#pragma once
#include <vector>
#include "glm/glm.hpp"

#include "commands/Command.h"
#include "geometry/Geometry.h"

namespace pepr3d {
class CmdReplaceColorManagerColors : public CommandBase<Geometry> {
   public:
    explicit CmdReplaceColorManagerColors(std::vector<glm::vec4>&& colors)
        : CommandBase(false, false), mColors(colors) {}

    std::string_view getDescription() const override {
        return "Change all colors";
    }

   protected:
    void run(Geometry& target) const override {
        target.getColorManager().replaceColors(mColors);
    }

    std::vector<glm::vec4> mColors;
};

}  // namespace pepr3d
