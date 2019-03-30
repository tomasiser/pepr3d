#pragma once

#include <vector>

#include "commands/Command.h"
#include "geometry/Geometry.h"

namespace pepr3d {

/// Command that sets a (same) color to a batch of triangles
class CmdPaintSingleColor : public CommandBase<Geometry> {
   public:
    std::string_view getDescription() const override {
        return "Set the same color to a batch of triangles";
    }

    CmdPaintSingleColor(size_t triangleId, const size_t colorId)
        : CmdPaintSingleColor(DetailedTriangleId(triangleId), colorId) {}

    CmdPaintSingleColor(DetailedTriangleId triangleId, const size_t colorId)
        : CommandBase(false, true), mTriangleIds{triangleId}, mColorId(colorId) {}

    CmdPaintSingleColor(std::vector<DetailedTriangleId>&& triangleIds, const size_t colorId)
        : CommandBase(false, true), mTriangleIds(triangleIds), mColorId(colorId) {}

    CmdPaintSingleColor(std::vector<size_t>&& triangleIds, const size_t colorId)
        : CommandBase(false, true), mColorId(colorId) {
        mTriangleIds.reserve(triangleIds.size());
        for(size_t triIdx : triangleIds) {
            mTriangleIds.emplace_back(triIdx);
        }
    }

   protected:
    void run(Geometry& target) const override {
        for(DetailedTriangleId triangleId : mTriangleIds) {
            target.setTriangleColor(triangleId, mColorId);
        }
    }

    bool joinCommand(const CommandBase& otherBase) override {
        const auto* other = dynamic_cast<const CmdPaintSingleColor*>(&otherBase);
        if(other && other->mColorId == mColorId) {
            mTriangleIds.insert(mTriangleIds.end(), other->mTriangleIds.begin(), other->mTriangleIds.end());
            return true;
        } else {
            return false;
        }
    }

    std::vector<DetailedTriangleId> mTriangleIds;
    size_t mColorId;
};
}  // namespace pepr3d