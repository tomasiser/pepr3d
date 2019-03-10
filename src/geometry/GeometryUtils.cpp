#include "geometry/GeometryUtils.h"
#include <CGAL/Aff_transformation_3.h>
#include "geometry/Geometry.h"
#include "geometry/TriangleDetail.h"

namespace pepr3d {

float GeometryUtils::segmentPointDistanceSquared(const glm::vec3 &start, const glm::vec3 &end, const glm::vec3 &point) {
    using K = CGAL::Simple_cartesian<double>;
    using Point = K::Point_3;
    using Segment = K::Segment_3;

    const Segment segment(Point(start.x, start.y, start.z), Point(end.x, end.y, end.z));
    const Point cgPoint(point.x, point.y, point.z);

    return static_cast<float>(CGAL::squared_distance(segment, cgPoint));
}

std::optional<glm::vec3> GeometryUtils::triangleRayIntersection(const DataTriangle &tri, ci::Ray ray) {
    const glm::vec3 source = ray.getOrigin();
    const glm::vec3 direction = ray.getDirection();
    const pepr3d::Geometry::Ray rayQuery(pepr3d::DataTriangle::Point(source.x, source.y, source.z),
                                         pepr3d::Geometry::Direction(direction.x, direction.y, direction.z));

    auto intersectionPointOrSeg = CGAL::intersection(rayQuery, tri.getTri());
    if(intersectionPointOrSeg) {
        glm::vec3 result = boost::apply_visitor(TriRayIntVisitor{}, *intersectionPointOrSeg);
        return result;
    } else {
        return {};
    }
}

bool GeometryUtils::isFullyInsideASphere(const DataTriangle::K::Triangle_3 &tri, const DataTriangle::K::Point_3 &origin,
                                         double radius) {
    // All three points must be inside the sphere (closer than the radius)
    using Vector = DataTriangle::K::Vector_3;
    const double radiusSquared = radius * radius;
    const double dist0 = Vector(tri.vertex(0), origin).squared_length();
    const double dist1 = Vector(tri.vertex(1), origin).squared_length();
    const double dist2 = Vector(tri.vertex(2), origin).squared_length();

    return dist0 <= radiusSquared && dist1 <= radiusSquared && dist2 <= radiusSquared;
}

std::pair<DataTriangle::K::Point_3, double> GeometryUtils::getBoundingSphere(
    const std::vector<DataTriangle::K::Point_3> &shape) {
    using Vector3 = Geometry::Vector3;
    using Point3 = Geometry::Point3;

    const double pointCount = static_cast<double>(shape.size());

    // Find center
    Vector3 center(0, 0, 0);
    for(const Point3 &pt : shape) {
        center += Vector3(pt.x() / pointCount, pt.y() / pointCount, pt.z() / pointCount);
    }

    const Point3 centerPoint(center.x(), center.y(), center.z());
    // Find max squared distance
    double maxDistSquared = 0;
    for(const Point3 &pt : shape) {
        maxDistSquared = std::max(maxDistSquared, CGAL::squared_distance(pt, centerPoint));
    }

    return std::make_pair(centerPoint, CGAL::sqrt(maxDistSquared));
}

std::pair<DataTriangle::K::Point_3, double> GeometryUtils::getBoundingSphere(
    const std::vector<DataTriangle::Triangle> &triangles) {
    using Vector3 = Geometry::Vector3;
    using Point3 = Geometry::Point3;

    const double pointCount = static_cast<double>(triangles.size() * 3);

    // Find center
    Vector3 center(0, 0, 0);
    for(auto &tri : triangles) {
        for(int i = 0; i < 3; i++) {
            auto &pt = tri.vertex(0);
            center += Vector3(pt.x() / pointCount, pt.y() / pointCount, pt.z() / pointCount);
        }
    }

    const Point3 centerPoint(center.x(), center.y(), center.z());

    // Find max squared distance
    double maxDistSquared = 0;
    for(auto &tri : triangles) {
        for(int i = 0; i < 3; i++) {
            auto &pt = tri.vertex(0);
            maxDistSquared = std::max(maxDistSquared, CGAL::squared_distance(pt, centerPoint));
        }
    }

    return std::make_pair(centerPoint, CGAL::sqrt(maxDistSquared));
}

}  // namespace pepr3d