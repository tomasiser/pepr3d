#pragma once
/**
 *  Fix for CGAL bugs in polygon with holes verification
 *  Contained in CGAL PR #3688, likely to be in 4.14 version of CGAL
 *
 *  For now, just keep this custom patched include file first, to avoid using CGAL's version
 */
#include "CGAL-patched/Boolean_set_operations_2/Gps_traits_adaptor.h"
//---------------------------------------

#include "Triangle.h"
#include "cinder/Ray.h"

#include <CGAL/Nef_polyhedron_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_set_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/Simple_cartesian.h>
#include <optional>
#include <vector>

namespace pepr3d {
class DataTriangle;

/// Static utility functions that are not closely tied to Geometry
class GeometryUtils {
   private:
    // Prevent this util class from being constructed
    GeometryUtils() {}

    class TriRayIntVisitor : public boost::static_visitor<glm::vec3> {
       public:
        inline glm::vec3 operator()(const DataTriangle::K::Point_3& pt) const {
            return glm::vec3(static_cast<float>(pt.x()), static_cast<float>(pt.y()), static_cast<float>(pt.z()));
        }

        inline glm::vec3 operator()(const DataTriangle::K::Segment_3& seg) const {
            const auto pt = seg.start();
            return glm::vec3(static_cast<float>(pt.x()), static_cast<float>(pt.y()), static_cast<float>(pt.z()));
        }
    };

   public:
    /// Simplify polygon, removing unnecessary vertices
    /// @return true Polygon was simplified
    template <typename K>
    static bool simplifyPolygon(CGAL::Polygon_2<K>& poly) {
        using Segment = CGAL::Segment_2<K>;

        std::vector<size_t> verticesToRemove;
        const size_t vertexCount = poly.vertices_end() - poly.vertices_begin();
        for(size_t i = 0; i < vertexCount; i++) {
            auto currentVertex = std::next(poly.vertices_circulator(), i);

            const typename K::Line_2 neighbourLine(*std::prev(currentVertex), *std::next(currentVertex));
            if(neighbourLine.has_on(*currentVertex)) {
                verticesToRemove.emplace_back(i);
            }
        }

        // Remove from the back so that we dont have to move that much data
        for(auto it = verticesToRemove.rbegin(); it != verticesToRemove.rend(); ++it) {
            size_t vertexId = *it;
            poly.erase(poly.vertices_begin() + vertexId);
        }

        return !verticesToRemove.empty();
    }

    /// Get equidistant points on circle
    static std::vector<DataTriangle::K::Point_3> pointsOnCircle(const DataTriangle::K::Circle_3& circle, int segments) {
        using Point3 = DataTriangle::K::Point_3;
        assert(segments >= 3);
        assert(!circle.is_degenerate());

        auto plane = circle.supporting_plane();
        const double radius = sqrt(CGAL::to_double(circle.squared_radius()));
        const auto xBase = plane.base1() / CGAL::sqrt(CGAL::to_double(plane.base1().squared_length()));
        const auto yBase = plane.base2() / CGAL::sqrt(CGAL::to_double(plane.base2().squared_length()));

        std::vector<DataTriangle::K::Point_3> points;

        for(size_t i = 0; i < segments; i++) {
            const double circleCoord = (static_cast<double>(i) / segments) * 2 * glm::pi<double>();
            const Point3 pt = circle.center() + xBase * cos(circleCoord) * radius + yBase * sin(circleCoord) * radius;

            points.push_back(pt);
        }

        return points;
    }

    /// Determine orientation of a polygon using shoelace formula
    /// Replacement for potentionally buggy CGAL implementation
    template <typename K>
    static CGAL::Orientation shoelaceOrientationTest(const CGAL::Polygon_2<K>& poly) {
        typename K::FT shoelaceSum = 0;

        const size_t vertexCount = poly.vertices_end() - poly.vertices_begin();
        for(size_t i = 0; i < vertexCount; i++) {
            auto currentVertexIt = poly.vertices_circulator() + i;
            auto nextVertexIt = currentVertexIt + 1;

            shoelaceSum += currentVertexIt->x() * nextVertexIt->y();
            shoelaceSum -= currentVertexIt->y() * nextVertexIt->x();
        }

        if(shoelaceSum > 0)
            return CGAL::Orientation::COUNTERCLOCKWISE;
        if(shoelaceSum < 0)
            return CGAL::Orientation::CLOCKWISE;

        return CGAL::Orientation::COLLINEAR;
    }

    /// Check validity of a polygon with holes using shoelace formula instead of a CGAL orientation test
    template <typename Traits_2>
    static bool is_valid_polygon_with_holes(const typename Traits_2::Polygon_with_holes_2& pgn,
                                            const Traits_2& traits) {
        bool closed = CGAL::is_closed_polygon_with_holes(pgn, traits);
        CGAL_warning_msg(closed, "The polygon's boundary or one of its holes is not closed.");
        if(!closed)
            return false;

        bool relatively_simple = CGAL::is_relatively_simple_polygon_with_holes(pgn, traits);
        CGAL_warning_msg(relatively_simple, "The polygon is not relatively simple.");
        if(!relatively_simple)
            return false;

        bool no_cross = CGAL::is_crossover_outer_boundary(pgn, traits);
        CGAL_warning_msg(no_cross, "The polygon has a crossover.");
        if(!no_cross)
            return false;

        bool valid_orientation = GeometryUtils::has_valid_orientation_polygon_with_holes(pgn, traits);
        CGAL_warning_msg(valid_orientation, "The polygon has a wrong orientation.");
        if(!valid_orientation)
            return false;

        bool holes_disjoint = CGAL::are_holes_and_boundary_pairwise_disjoint(pgn, traits);
        CGAL_warning_msg(holes_disjoint, "Holes of the PWH intersect amongst themselves or with outer boundary");
        if(!holes_disjoint)
            return false;

        return true;
    }

    template <typename Traits_2>
    static bool has_valid_orientation_polygon_with_holes(const typename Traits_2::Polygon_with_holes_2& pgn,
                                                         const Traits_2& traits) {
        /// Outer boundary must be counterclockwise
        if(shoelaceOrientationTest(pgn.outer_boundary()) != CGAL::Orientation::COUNTERCLOCKWISE) {
            return false;
        }

        /// Holes must be clockwise
        for(auto holeIt = pgn.holes_begin(); holeIt != pgn.holes_end(); ++holeIt) {
            if(shoelaceOrientationTest(*holeIt) != CGAL::Orientation::CLOCKWISE) {
                return false;
            }
        }

        return true;
    }

    /// Get bounding sphere of a shape
    /// @param shape Polygonal shape represented by a set of points
    static std::pair<DataTriangle::K::Point_3, double> getBoundingSphere(
        const std::vector<DataTriangle::K::Point_3>& shape);

    /// Get bounding sphere of a shape
    /// @param shape Polygonal shape represented by a set of points
    static std::pair<DataTriangle::K::Point_3, double> getBoundingSphere(
        const std::vector<DataTriangle::Triangle>& triangles);

    /// Find squared distance between a line segment and a point in 3D space
    static float segmentPointDistanceSquared(const glm::vec3& start, const glm::vec3& end, const glm::vec3& point);

    /// Find intersection point of a ray and a single triangle
    /// If the intersection is a segment return one of the edge points
    static std::optional<glm::vec3> triangleRayIntersection(const class DataTriangle& tri, ci::Ray ray);

    static bool isFullyInsideASphere(const DataTriangle::K::Triangle_3& tri, const DataTriangle::K::Point_3& origin,
                                     double radius);

    static bool isFullyInsideASphere(const DataTriangle::K::Triangle_3& tri, const glm::vec3& origin, double radius) {
        return isFullyInsideASphere(tri, DataTriangle::K::Point_3(origin.x, origin.y, origin.z), radius);
    }
};

}  // namespace pepr3d
