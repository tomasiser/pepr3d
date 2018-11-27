#pragma once
#include "geometry/Triangle.h"

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Gps_circle_segment_traits_2.h>
#include <CGAL/Lazy_exact_nt.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_set_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <map>
#include <vector>

namespace pepr3d {
/**
 * Represents additional triangles that replace the original one. Allows "painting" over the original triangle
 * by creating a polygonal representation of the new shapes, which is then converted back to triangles.
 */
class TriangleDetail {
   public:
    // using K = CGAL::Exact_predicates_exact_constructions_kernel;
    using K = DataTriangle::K;  // Lets try using our normal kernel if its possible. (Should be way faster)
    using CircleSegmentTraits = CGAL::Gps_circle_segment_traits_2<K>;
    using Polygon = CGAL::Polygon_2<K>;
    using PolygonWithHoles = CGAL::Polygon_with_holes_2<K>;
    using Circle = TriangleDetail::K::Circle_2;
    using Curve = CircleSegmentTraits::Curve_2;

    using PolygonSet = CGAL::Polygon_set_2<K>;
    using PeprPoint2 = DataTriangle::K::Point_2;
    using Point2 = TriangleDetail::K::Point_2;
    using Vector2 = TriangleDetail::K::Vector_2;

    using PeprTriangle = DataTriangle::Triangle;
    using Plane = DataTriangle::K::Plane_3;

    explicit TriangleDetail(const DataTriangle& original, size_t colorIdx)
        : mOriginal(original), mOriginalPlane(original.getTri().supporting_plane()) {
        mBounds = polygonFromTriangle(mOriginal.getTri());
    }

    bool isSingleColor() const {
        return mTriangles.size() <= 1;
    }

    /// Paint a circle shape onto the plane of the triangle
    void addCircle(const PeprPoint2& circleOrigin, const PeprPoint2& circleEdge, size_t color);

    const std::vector<DataTriangle>& getTriangles() const {
        return mTriangles;
    }

    const DataTriangle& getOriginal() const {
        return mOriginal;
    }

   private:
    std::vector<DataTriangle> mTriangles;

    const DataTriangle mOriginal;

    static const int VERTICES_PER_UNIT_CIRCLE = 100;

    /// Bounds of the original triangle
    Polygon mBounds;

    const Plane mOriginalPlane;

    Polygon polygonFromTriangle(const PeprTriangle& tri) const;

    // Construct a polygon from a circle.
    Polygon polygonFromCircle(const PeprPoint2& circleOrigin, const PeprPoint2& circleEdge);

    inline static K::Point_2 convertPoint(const DataTriangle::K::Point_2& point) {
        return K::Point_2(point.x(), point.y());
    }

    inline static glm::vec3 convertPoint(const K::Point_3& pt) {
        return glm::vec3(pt.x(), pt.y(), pt.z());
    }

    /// Generate one colored polygon set for each color inside the triangle
    std::map<size_t, PolygonSet> gatherTrianglesIntoPolys();

    /// Add triangles from this polygon to our triangles
    void addTrianglesFromPolygon(const PolygonWithHoles& poly, size_t color);

    /// Create new triangles from a set of colored polygons
    void createNewTriangles(const std::map<size_t, PolygonSet>& coloredPolys);
};
}  // namespace pepr3d