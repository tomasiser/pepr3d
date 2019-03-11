#pragma once

#include <vector>

#include "commands/Command.h"
#include "geometry/Geometry.h"
#include "tools/Brush.h"

#include <chrono>

namespace pepr3d {

/// Command that paints a stroke with a brush
class CmdPaintBrush : public CommandBase<Geometry> {
    using Point3 = Geometry::Point3;
    using Vector3 = Geometry::Vector3;
    using Circle = Geometry::Circle;

   public:
    std::string_view getDescription() const override {
        return "Paint with a brush";
    }

    CmdPaintBrush(ci::Ray ray, const BrushSettings settings)
        : CommandBase(true, true), mRays{ray}, mSettings(settings) {}

   protected:
    void run(Geometry& target) const override {
        const auto start = std::chrono::high_resolution_clock::now();

        for(const ci::Ray& ray : mRays) {
            if(mSettings.spherical) {
                target.paintAreaWithSphere(ray, mSettings);
            } else {
                glm::vec3 ro = ray.getOrigin();
                glm::vec3 rd = ray.getDirection();
                if(mSettings.alignToNormal) {
                    auto intersection = target.intersectMesh(ray, ro);
                    if(!intersection) {
                        continue;
                    }

                    rd = -target.getTriangle(*intersection).getNormal();
                }

                // Create a shape to paint with
                const Vector3 rayDirectionVector(rd.x, rd.y, rd.z);
                const Circle circle(Point3(ro.x, ro.y, ro.z), mSettings.size * mSettings.size, rayDirectionVector);
                const std::vector<Point3> circlePoints = GeometryUtils::pointsOnCircle(circle, mSettings.segments);

                target.paintWithShape(ray, circlePoints, mSettings.color, mSettings.paintBackfaces);
            }
        }

        const auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> timeMs = end - start;

        CI_LOG_I("Brush paint took " + std::to_string(timeMs.count()) + " ms");
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