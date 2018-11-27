#include "geometry/TriangleDetail.h"

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Gps_circle_segment_traits_2.h>
#include <CGAL/Gps_traits_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_set_2.h>
#include <CGAL/Polygon_triangulation_decomposition_2.h>
#include <CGAL/partition_2.h>

#include <assert.h>
#include <list>

// Loss of precision between conversions may move verticies? Needs testing

namespace pepr3d {
struct FaceInfo {
    int nestingLevel = -1;
};

using K = TriangleDetail::K;
using Fbb = CGAL::Triangulation_face_base_with_info_2<FaceInfo, K>;
using Fb = CGAL::Constrained_triangulation_face_base_2<K, Fbb>;
using Tds = CGAL::Triangulation_data_structure_2<CGAL::Triangulation_vertex_base_2<K>, Fb>;
using ConstrainedTriangulation = CGAL::Constrained_triangulation_2<K, Tds>;

using XMonotoneCurve = TriangleDetail::CircleSegmentTraits::X_monotone_curve_2;

void TriangleDetail::addCircle(const PeprPoint2& circleOrigin, const PeprPoint2& circleEdge, size_t color) {
    const Polygon circlePoly = polygonFromCircle(circleOrigin, circleEdge);
    PolygonSet addedShape;
    addedShape.insert(circlePoly);
    addedShape.intersection(mBounds);

    auto coloredPolys = gatherTrianglesIntoPolys();
    for(auto it : coloredPolys) {
        // Join the new shape with PolygonSet of the same color
        // Remove the new shape from other colors
        if(it.first == color) {
            it.second.join(addedShape);
        } else {
            it.second.difference(addedShape);
        }
    }

    createNewTriangles(coloredPolys);
}

TriangleDetail::Polygon pepr3d::TriangleDetail::polygonFromTriangle(const PeprTriangle& tri) const {
    const Point2 a = convertPoint(mOriginalPlane.to_2d(tri.vertex(0)));
    const Point2 b = convertPoint(mOriginalPlane.to_2d(tri.vertex(1)));
    const Point2 c = convertPoint(mOriginalPlane.to_2d(tri.vertex(2)));

    Polygon pgn;
    pgn.push_back(a);
    pgn.push_back(b);
    pgn.push_back(c);
    return pgn;
}

TriangleDetail::Polygon TriangleDetail::polygonFromCircle(const PeprPoint2& circleOrigin,
                                                          const PeprPoint2& circleEdge) {
    const Circle circle(convertPoint(circleOrigin), convertPoint(circleEdge));
    const Vector2 circleDirA{circleOrigin, circleEdge};
    const Vector2 circleDirB = circleDirA.perpendicular(CGAL::Orientation::COUNTERCLOCKWISE);

    // Scale the vertex count based on the size of the circle
    size_t vertexCount =
        static_cast<size_t>(CGAL::sqrt(circleDirA.squared_length()) * glm::pi<double>() * 2 * VERTICES_PER_UNIT_CIRCLE);
    vertexCount = std::max(vertexCount, static_cast<size_t>(3));

    // Construct the polygon.
    Polygon pgn;
    for(size_t i = 0; i < vertexCount; i++) {
        const double circleCoord = (static_cast<double>(i) / vertexCount) * 2 * glm::pi<double>();
        Point2 pt = circleOrigin + circleDirA * cos(circleCoord) + circleDirB * sin(circleCoord);

        pgn.push_back(std::move(pt));
    }

    return pgn;
}
std::map<size_t, TriangleDetail::PolygonSet> TriangleDetail::gatherTrianglesIntoPolys() {
    std::map<size_t, PolygonSet> result;

    for(const DataTriangle& tri : mTriangles) {
        result[tri.getColor()].join(polygonFromTriangle(tri.getTri()));
    }

    return result;
}

void markDomains(ConstrainedTriangulation& ct, ConstrainedTriangulation::Face_handle start, int index,
                 std::list<ConstrainedTriangulation::Edge>& border) {
    if(start->info().nestingLevel != -1) {
        return;
    }
    std::list<ConstrainedTriangulation::Face_handle> queue;
    queue.push_back(start);
    while(!queue.empty()) {
        ConstrainedTriangulation::Face_handle fh = queue.front();
        queue.pop_front();
        if(fh->info().nestingLevel == -1) {
            fh->info().nestingLevel = index;
            for(int i = 0; i < 3; i++) {
                ConstrainedTriangulation::Edge e(fh, i);
                ConstrainedTriangulation::Face_handle n = fh->neighbor(i);
                if(n->info().nestingLevel == -1) {
                    if(ct.is_constrained(e))
                        border.push_back(e);
                    else
                        queue.push_back(n);
                }
            }
        }
    }
}

// ( From CGAL User Manual )
// ---------------------------
// explore set of facets connected with non constrained edges,
// and attribute to each such set a nesting level.
// We start from facets incident to the infinite vertex, with a nesting
// level of 0. Then we recursively consider the non-explored facets incident
// to constrained edges bounding the former set and increase the nesting level by 1.
// Facets in the domain are those with an odd nesting level.
void markDomains(ConstrainedTriangulation& ct) {
    for(auto it = ct.all_faces_begin(); it != ct.all_faces_end(); ++it) {
        it->info().nestingLevel = -1;
    }
    std::list<ConstrainedTriangulation::Edge> border;
    markDomains(ct, ct.infinite_face(), 0, border);
    while(!border.empty()) {
        ConstrainedTriangulation::Edge e = border.front();
        border.pop_front();
        ConstrainedTriangulation::Face_handle n = e.first->neighbor(e.second);
        if(n->info().nestingLevel == -1) {
            markDomains(ct, n, e.first->info().nestingLevel + 1, border);
        }
    }
}

void TriangleDetail::addTrianglesFromPolygon(const PolygonWithHoles& poly, size_t color) {
    ConstrainedTriangulation ct;
    ct.insert_constraint(poly.outer_boundary().vertices_begin(), poly.outer_boundary().vertices_end());

    for(auto holeIt = poly.holes_begin(); holeIt != poly.holes_end(); holeIt++) {
        ct.insert_constraint(holeIt->vertices_begin(), holeIt->vertices_end());
    }

    markDomains(ct);

    for(auto faceIt = ct.finite_faces_begin(); faceIt != ct.finite_faces_end(); ++faceIt) {

		// Keep only faces with odd nesting level, those are inside the polygon and not in the hole
        if(faceIt->info().nestingLevel % 2 > 0) {
            const glm::vec3 a = convertPoint(mOriginalPlane.to_3d(faceIt->vertex(0)->point()));
            const glm::vec3 b = convertPoint(mOriginalPlane.to_3d(faceIt->vertex(1)->point()));
            const glm::vec3 c = convertPoint(mOriginalPlane.to_3d(faceIt->vertex(2)->point()));

            DataTriangle tri(a,b,c, mOriginal.getNormal());
            tri.setColor(color);
            mTriangles.emplace_back(std::move(tri));
        }
    }
}

void TriangleDetail::createNewTriangles(const std::map<size_t, PolygonSet>& coloredPolys) {
    mTriangles.clear();

    for(const auto it : coloredPolys) {
        if(it.second.is_empty())
            continue;

        std::vector<PolygonWithHoles> polys(it.second.number_of_polygons_with_holes());
        it.second.polygons_with_holes(polys.begin());
        for(PolygonWithHoles& poly : polys) {
            addTrianglesFromPolygon(poly, it.first);
        }
    }
}
}  // namespace pepr3d
