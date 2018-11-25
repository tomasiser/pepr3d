#pragma once
#include <CGAL/Simple_cartesian.h>
#include <glm/glm.hpp>
#include <optional>
#include "CGAL/squared_distance_2.h"
#include "cinder/Ray.h"


namespace pepr3d {
class DataTriangle;

class GeometryUtils {
   private:
    // Prevent this util class from being constructed
    GeometryUtils() {}

    class TriRayIntVisitor : public boost::static_visitor<glm::vec3> {
       public:
        inline glm::vec3 operator()(const CGAL::Simple_cartesian<double>::Point_3& pt) const {
            return glm::vec3(static_cast<float>(pt.x()), static_cast<float>(pt.y()), static_cast<float>(pt.z()));
        }

        inline glm::vec3 operator()(const CGAL::Simple_cartesian<double>::Segment_3& seg) const {
            const auto pt = seg.start();
            return glm::vec3(static_cast<float>(pt.x()), static_cast<float>(pt.y()), static_cast<float>(pt.z()));
        }
    };

   public:
    /// Find squared distance between a line segment and a point in 3D space
    static float segmentPointDistanceSquared(const glm::vec3 &start, const glm::vec3 &end, const glm::vec3 &point);

    /// Find intersection point of a ray and a single triangle
    /// If the intersection is a segment return one of the edge points
    static std::optional<glm::vec3> triangleRayIntersection(const class DataTriangle& tri, ci::Ray ray);
};


}  // namespace pepr3d
