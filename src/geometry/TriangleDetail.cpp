#include "geometry/TriangleDetail.h"

#include <CGAL/Boolean_set_operations_2.h>
#include <CGAL/Gps_traits_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_set_2.h>
#include <CGAL/Spherical_kernel_intersections.h>
#include <CGAL/partition_2.h>

#include <cinder/Filesystem.h>

#include <assert.h>
#include <cinder/Log.h>
#include <deque>
#include <glm/gtc/constants.inl>
#include <list>
#include <optional>
#include <type_traits>

namespace pepr3d {

#ifdef _DEBUG
#include <fstream>

class GnuplotDebug {
   public:
    GnuplotDebug() = default;
    GnuplotDebug(const GnuplotDebug&) = delete;

    void addPoly(const TriangleDetail::Polygon& poly, const std::string& rgbStr) {
        mPolysToDraw.push_back(poly);
        mRgbStrings.push_back(rgbStr);
    }

    void exportToFile() {
        std::ofstream oFileGnuplot("debugOut.gnuplot");
        if(oFileGnuplot.bad())
            return;

        oFileGnuplot << "set size ratio -1 \n";

        for(size_t idx = 0; idx < mRgbStrings.size(); ++idx) {
            oFileGnuplot << "set style line " << idx + 1 << " linecolor rgb \"" << mRgbStrings[idx]
                         << "\" linetype 1 linewidth 2\n";
        }

        const cinder::fs::path filePath("debugOut.data");

        oFileGnuplot << "plot '" << absolute(filePath) << "' index 0 with lines linestyle 1";

        for(size_t idx = 1; idx < mPolysToDraw.size(); ++idx) {
            oFileGnuplot << ", '" << absolute(filePath) << "' index " << idx << " with lines linestyle " << idx + 1;
        }

        oFileGnuplot << "\n";

        std::ofstream oFileData(absolute(filePath));
        if(oFileData.bad())
            return;

        for(const auto& poly : mPolysToDraw) {
            oFileData << "# X Y\n";
            for(TriangleDetail::Polygon::Vertex_const_iterator it = poly.vertices_begin(); it != poly.vertices_end();
                ++it) {
                oFileData << it->x() << " " << it->y() << "\n";
            }

            // add first point again to connect the poly
            oFileData << poly.vertices_begin()->x() << " " << poly.vertices_begin()->y() << "\n";

            oFileData << "\n";
            oFileData << "\n";
        }
    }

   private:
    std::vector<TriangleDetail::Polygon> mPolysToDraw;
    std::vector<std::string> mRgbStrings;
};

#endif

void TriangleDetail::addCircle(const Circle3& circle, size_t color) {
    const Polygon circlePoly = polygonFromCircle(circle);
    paintPolygon(circlePoly, color);
}

void TriangleDetail::paintSphere(const PeprSphere& peprSphere, size_t color) {
    // Vertices on the triangle boundaries must be the same across multiple triangle details!

    const Sphere sphere(toExactK(peprSphere.center()), peprSphere.squared_radius());
    auto intersection = CGAL::intersection(sphere, mOriginalPlane);

    if(!intersection) {
        return;
    }

    std::optional<Circle3> circleIntersection = boost::apply_visitor(SphereIntersectionVisitor{}, *intersection);

    // Continue only if the intersection is a circle (not a point or miss)
    if(circleIntersection) {
        auto poly = polygonFromCircle(*circleIntersection);
        paintPolygon(poly, color);
    }
}

TriangleDetail::Polygon pepr3d::TriangleDetail::polygonFromTriangle(const PeprTriangle& tri) const {
    const Point2 a = mOriginalPlane.to_2d(toExactK(tri.vertex(0)));
    const Point2 b = mOriginalPlane.to_2d(toExactK(tri.vertex(1)));
    const Point2 c = mOriginalPlane.to_2d(toExactK(tri.vertex(2)));

    Polygon pgn;
    pgn.push_back(a);
    pgn.push_back(b);
    pgn.push_back(c);

    assert(!pgn.is_empty());

    if(pgn.is_clockwise_oriented())
        pgn.reverse_orientation();

    return pgn;
}

TriangleDetail::Polygon TriangleDetail::polygonFromTriangle(const Triangle2& tri) const {
    Polygon pgn;
    pgn.push_back(tri.vertex(0));
    pgn.push_back(tri.vertex(1));
    pgn.push_back(tri.vertex(2));

    assert(!pgn.is_empty());

    if(pgn.is_clockwise_oriented())
        pgn.reverse_orientation();

    return pgn;
}

std::vector<std::pair<TriangleDetail::Point2, double>> TriangleDetail::getCircleSharedPoints(const Circle3& circle,
                                                                                             const Vector3& xBase,
                                                                                             const Vector3& yBase) {
    // We need shared verticies on the boundary of triangle details
    // This vertex needs to be the same for both neighbouring triangles
    // Thats why we calculate the intersection using original world-space data

    Sphere sphere(circle.center(), circle.squared_radius());
    std::vector<std::pair<Point2, double>> result;

    // Find all points that intersect triangle edge
    for(size_t i = 0; i < 3; i++) {
        Line3 triEdge(toExactK(mOriginal.getVertex(i)), toExactK(mOriginal.getVertex((i + 1) % 3)));

        std::vector<CGAL::Object> intersections;
        auto intersection = CGAL::intersection(sphere, triEdge, std::back_inserter(intersections));

        // Add both intersection points of this edge
        for(auto& obj : intersections) {
            std::pair<K::Circular_arc_point_3, unsigned> ptPair;
            if(CGAL::assign(ptPair, obj)) {
                K::Circular_arc_point_3& pt = ptPair.first;
                const Point3 worldPoint(CGAL::to_double(pt.x()), CGAL::to_double(pt.y()), CGAL::to_double(pt.z()));

                // Project the vector onto the bases of the circle
                const auto circleVector(worldPoint - circle.center());
                auto xCoords = circleVector * xBase;
                auto yCoords = circleVector * yBase;

                // Find the circle angle that matches this point, so that we know where it belongs
                // The angle is measured from the x-positive axis going counter clockwise. a \in (0, 2PI)
                double circleAngle = std::atan2(CGAL::to_double(yCoords), CGAL::to_double(xCoords));
                if(circleAngle < 0) {
                    circleAngle += 2 * glm::pi<double>();
                }

                result.emplace_back(std::make_pair(mOriginalPlane.to_2d(worldPoint), circleAngle));
            }
        }
    }

    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.second < b.second; });

    return result;
}

TriangleDetail::Polygon TriangleDetail::polygonFromCircle(const Circle3& circle) {
    // Scale the vertex count based on the size of the circle
    const double radius = sqrt(CGAL::to_double(circle.squared_radius()));
    size_t vertexCount = static_cast<size_t>(radius * VERTICES_PER_UNIT_CIRCLE);
    vertexCount = std::max(vertexCount, static_cast<size_t>(MIN_VERTICES_IN_CIRCLE));

    // Bases for the points of the circle (cannot be exact, because Epeck does not support sqrt)
    const auto xBase = mOriginalPlane.base1() / CGAL::sqrt(CGAL::to_double(mOriginalPlane.base1().squared_length()));
    const auto yBase = mOriginalPlane.base2() / CGAL::sqrt(CGAL::to_double(mOriginalPlane.base2().squared_length()));
    assert(xBase * yBase == 0);

    // We need a shared vertex on the boundary of triangle details
    // This vertex does not need to be exact, but needs to be the same from both triangles
    std::vector<std::pair<Point2, double>> sharedPoints = getCircleSharedPoints(circle, xBase, yBase);
    auto sharedPointIt = sharedPoints.begin();

    // Construct the polygon.
    Polygon pgn;
    for(size_t i = 0; i < vertexCount; i++) {
        const double circleCoord = (static_cast<double>(i) / vertexCount) * 2 * glm::pi<double>();
        const Point3 pt = circle.center() + xBase * cos(circleCoord) * radius + yBase * sin(circleCoord) * radius;

        // Add all shared points that are before this point
        while(sharedPointIt != sharedPoints.end() && sharedPointIt->second <= circleCoord) {
            if(sharedPointIt->second != circleCoord) {
                pgn.push_back(sharedPointIt->first);
            }

            sharedPointIt++;
        }

        pgn.push_back(mOriginalPlane.to_2d(pt));
    }

    // Add the remaining shared points
    while(sharedPointIt != sharedPoints.end()) {
        pgn.push_back(sharedPointIt->first);
        sharedPointIt++;
    }

    assert(pgn.is_simple());
    assert(pgn.is_counterclockwise_oriented());
    assert(pgn.is_convex());

    return pgn;
}

void TriangleDetail::paintPolygon(const Polygon& poly, size_t color) {
    PolygonSet addedShape(poly);
    addedShape.intersection(mBounds);

    if(colorChanged) {
        updatePolysFromTriangles();
    }

    // Add the shape to its color layer
    mColoredPolys[color].join(addedShape);

    // Remove the new shape from other colors
    for(auto& it : mColoredPolys) {
        if(it.first != color) {
            it.second.difference(addedShape);
        }
    }

    createNewTriangles(mColoredPolys);
}

bool TriangleDetail::simplifyPolygon(PolygonWithHoles& poly) {
    using Segment = CGAL::Segment_2<K>;
    Polygon& boundary = poly.outer_boundary();

    std::vector<size_t> verticesToRemove;
    size_t edgeCount = boundary.edges_end() - boundary.edges_begin();
    for(size_t i = 1; i < edgeCount; i++) {
        Segment lastEdge = boundary.edge(i - 1);
        Segment edge = boundary.edge(i);
        if(lastEdge.supporting_line() == edge.supporting_line()) {
            // Edges are on the same line
            // Remove their shared vertex to connect them
            verticesToRemove.push_back(i);
        }
    }

    // Remove from the back so that we dont have to move that much data
    for(auto it = verticesToRemove.rbegin(); it != verticesToRemove.rend(); ++it) {
        size_t vertexId = *it;
        boundary.erase(boundary.vertices_begin() + vertexId);
    }

    return !verticesToRemove.empty();
}
void TriangleDetail::updatePolysFromTriangles() {
    assert(mTriangles.size() == mTrianglesExact.size());

    // Create polygons from triangles
    std::map<size_t, std::vector<Polygon>> polygonsByColor;
    for(size_t i = 0; i < mTriangles.size(); i++) {
        const DataTriangle& tri = mTriangles[i];
        polygonsByColor[tri.getColor()].emplace_back(polygonFromTriangle(mTrianglesExact[i]));
    }

    // Create polygon set for each color
    mColoredPolys.clear();
    for(const auto& it : polygonsByColor) {
        const std::vector<Polygon>& polygons = it.second;
        PolygonSet pSet;
        pSet.join(polygons.begin(), polygons.end());
        mColoredPolys.emplace(std::make_pair(it.first, std::move(pSet)));
    }

    colorChanged = false;
}

void TriangleDetail::markDomains(ConstrainedTriangulation& ct, ConstrainedTriangulation::Face_handle start, int index,
                                 std::deque<ConstrainedTriangulation::Edge>& border) {
    if(start->info().nestingLevel != -1) {
        return;
    }
    std::deque<ConstrainedTriangulation::Face_handle> queue;
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
void TriangleDetail::markDomains(ConstrainedTriangulation& ct) {
    for(auto it = ct.all_faces_begin(); it != ct.all_faces_end(); ++it) {
        it->info().nestingLevel = -1;
    }
    std::deque<ConstrainedTriangulation::Edge> border;
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

    // Add outer edge
    for(auto edgeIt = poly.outer_boundary().edges_begin(); edgeIt != poly.outer_boundary().edges_end(); ++edgeIt) {
        ct.insert_constraint(edgeIt->source(), edgeIt->target());
    }

    // Add edges of each hole
    for(auto holeIt = poly.holes_begin(); holeIt != poly.holes_end(); holeIt++) {
        for(auto edgeIt = holeIt->edges_begin(); edgeIt != holeIt->edges_end(); ++edgeIt) {
            ct.insert_constraint(edgeIt->source(), edgeIt->target());
        }
    }

    markDomains(ct);

    for(auto faceIt = ct.finite_faces_begin(); faceIt != ct.finite_faces_end(); ++faceIt) {
        // Keep only faces with odd nesting level, those are inside the polygon and not in the hole

        if(faceIt->info().nestingLevel % 2 > 0) {
            const glm::vec3 a = toGlmVec(mOriginalPlane.to_3d(faceIt->vertex(0)->point()));
            const glm::vec3 b = toGlmVec(mOriginalPlane.to_3d(faceIt->vertex(1)->point()));
            const glm::vec3 c = toGlmVec(mOriginalPlane.to_3d(faceIt->vertex(2)->point()));

            DataTriangle tri(a, b, c, mOriginal.getNormal());
            if(tri.getTri().squared_area() > 0) {
                tri.setColor(color);

                // Make sure that the original counter-clockwise order is preserved
                if(glm::dot(mOriginal.getNormal(), glm::cross((b - a), (c - a))) < 0) {
                    tri = DataTriangle(a, c, b, mOriginal.getNormal());
                }

                mTriangles.emplace_back(std::move(tri));
                mTrianglesExact.emplace_back(faceIt->vertex(0)->point(), faceIt->vertex(1)->point(),
                                             faceIt->vertex(2)->point());
            }
        }
    }
}

void TriangleDetail::createNewTriangles(std::map<size_t, PolygonSet>& coloredPolys) {
    mTriangles.clear();
    mTrianglesExact.clear();

    for(auto& colorSetIt : coloredPolys) {
        if(colorSetIt.second.is_empty())
            continue;

        bool simplified = false;
        std::vector<PolygonWithHoles> polys(colorSetIt.second.number_of_polygons_with_holes());
        colorSetIt.second.polygons_with_holes(polys.begin());
        for(PolygonWithHoles& poly : polys) {
            simplified |= simplifyPolygon(poly);
            addTrianglesFromPolygon(poly, colorSetIt.first);
        }

        // Update this polygon set with simplified representation
        if(simplified) {
            colorSetIt.second.clear();
            for(PolygonWithHoles& poly : polys) {
                colorSetIt.second.join(poly);
            }
        }
    }

    assert(mTriangles.size() == mTrianglesExact.size());
}
}  // namespace pepr3d
