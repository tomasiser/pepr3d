#pragma once

#include <vector>

#include "commands/Command.h"
#include "geometry/Geometry.h"
#include "tools/Brush.h"

namespace pepr3d {

class CmdPaintBrush : public CommandBase<Geometry> {
   public:
    std::string_view getDescription() const override {
        return "Paint with a brush";
    }

    CmdPaintBrush(ci::Ray ray, const BrushSettings settings)
        : CommandBase(false, true), mRays{ray}, mSettings(settings) {}

   protected:
    void run(Geometry& target) const override {
        for(const ci::Ray& ray : mRays) {
            target.paintArea(ray, mSettings);
        }
    }

    bool joinCommand(const CommandBase& otherBase) override {
        const auto* other = dynamic_cast<const CmdPaintBrush*>(&otherBase);
        if(other && other->mSettings == mSettings) {
            mRays.insert(mRays.end(), other->mRays.begin(), other->mRays.end());
            return true;
        } else {
            return false;
        }
    }

    std::vector<ci::Ray> mRays;
    BrushSettings mSettings;
};
}  // namespace pepr3d