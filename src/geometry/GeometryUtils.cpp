#include "geometry/GeometryUtils.h"
#include "geometry/Geometry.h"
#include "geometry/TriangleDetail.h"

#include <CGAL/Aff_transformation_3.h>
#include <CGAL/Min_sphere_of_points_d_traits_3.h>
#include <CGAL/Min_sphere_of_spheres_d.h>

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
    using K = DataTriangle::K;
    using Point3 = Geometry::Point3;
    using Sphere = Geometry::Sphere;
    using Traits = CGAL::Min_sphere_of_points_d_traits_3<K, K::FT>;
    using MinSphere = CGAL::Min_sphere_of_spheres_d<Traits>;

    std::vector<Point3> spheres;
    for(const Point3 &point : shape) {
        // Explicitly construct by coordinates, to be safe in multithreaded environment
        spheres.emplace_back(point.x(), point.y(), point.z());
    }

    // Using min sphere of spheres is supposedly faster than min sphere
    MinSphere ms(spheres.begin(), spheres.end());
    Point3 centerPoint(*ms.center_cartesian_begin(), *(ms.center_cartesian_begin() + 1),
                       *(ms.center_cartesian_begin() + 2));

    return std::make_pair(centerPoint, ms.radius());
}

std::pair<DataTriangle::K::Point_3, double> GeometryUtils::getBoundingSphere(
    const std::vector<DataTriangle::Triangle> &triangles) {
    using K = DataTriangle::K;
    using Point3 = Geometry::Point3;
    using Sphere = Geometry::Sphere;
    using Traits = CGAL::Min_sphere_of_points_d_traits_3<K, K::FT>;
    using MinSphere = CGAL::Min_sphere_of_spheres_d<Traits>;

    std::vector<Point3> spheres;
    for(const DataTriangle::Triangle &tri : triangles) {
        // Explicitly construct by coordinates, to be safe in multithreaded environment
        spheres.emplace_back(tri.vertex(0).x(), tri.vertex(0).y(), tri.vertex(0).z());
        spheres.emplace_back(tri.vertex(1).x(), tri.vertex(1).y(), tri.vertex(1).z());
        spheres.emplace_back(tri.vertex(2).x(), tri.vertex(2).y(), tri.vertex(2).z());
    }

    // Using min sphere of spheres is supposedly faster than min sphere
    MinSphere ms(spheres.begin(), spheres.end());
    Point3 centerPoint(*ms.center_cartesian_begin(), *(ms.center_cartesian_begin() + 1),
                       *(ms.center_cartesian_begin() + 2));

    return std::make_pair(centerPoint, ms.radius());
}

std::pair<DataTriangle::K::Point_3, double> GeometryUtils::getBoundingSphere(const DataTriangle::Triangle &triangle) {
    using K = DataTriangle::K;
    using Point3 = Geometry::Point3;
    using Sphere = Geometry::Sphere;
    using Traits = CGAL::Min_sphere_of_points_d_traits_3<K, K::FT>;
    using MinSphere = CGAL::Min_sphere_of_spheres_d<Traits>;

    std::array<Point3, 3> spheres{Point3(triangle.vertex(0).x(), triangle.vertex(0).y(), triangle.vertex(0).z()),
                                  Point3(triangle.vertex(1).x(), triangle.vertex(1).y(), triangle.vertex(1).z()),
                                  Point3(triangle.vertex(2).x(), triangle.vertex(2).y(), triangle.vertex(2).z())};

    // Using min sphere of spheres is supposedly faster than min sphere
    MinSphere ms(spheres.begin(), spheres.end());
    Point3 centerPoint(*ms.center_cartesian_begin(), *(ms.center_cartesian_begin() + 1),
                       *(ms.center_cartesian_begin() + 2));

    return std::make_pair(centerPoint, ms.radius());
}

}  // namespace pepr3d