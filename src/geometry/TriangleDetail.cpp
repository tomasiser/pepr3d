#include "geometry/TriangleDetail.h"

#include <CGAL/Gps_traits_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_set_2.h>

#include <CGAL/partition_2.h>

#include <cinder/Filesystem.h>

#include <assert.h>
#include <cinder/Log.h>
#include <deque>
#include <list>
#include <optional>

// Loss of precision between conversions may move verticies? Needs testing

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

    PolygonSet addedShape(circlePoly);
    addedShape.intersection(mBounds);

    /*GnuplotDebug dbg;
    dbg.addPoly(circlePoly, "#FF0000");
    dbg.addPoly(mBounds, "#00FF00");
    dbg.exportToFile();*/

    // std::map<size_t, PolygonSet> coloredPolys = gatherTrianglesIntoPolys();
    bool joined = false;
    for(auto& it : mColoredPolys) {
        // Join the new shape with PolygonSet of the same color
        // Remove the new shape from other colors
        if(it.first == color) {
            it.second.join(addedShape);
            joined = true;
        } else {
            it.second.difference(addedShape);
        }
    }

    // If there is no other polygon of the same color create a new one
    if(!joined) {
        mColoredPolys.emplace(color, std::move(addedShape));
    }

    createNewTriangles(mColoredPolys);
}

void TriangleDetail::paintSphere(const PeprSphere& peprSphere, size_t color) {
    // Need to calculate everything using exact kernels

    const Sphere sphere(toExactK(peprSphere.center()), peprSphere.squared_radius());

    const Plane plane(mOriginalPlane.a(), mOriginalPlane.b(), mOriginalPlane.c(), mOriginalPlane.d());
    auto intersection = CGAL::intersection(sphere, plane);

    if(!intersection) {
        return;
    }

    std::optional<Circle3> circleIntersection = boost::apply_visitor(SphereIntersectionVisitor{}, *intersection);
    if(circleIntersection) {
        addCircle(*circleIntersection, color);
    }
}

TriangleDetail::Polygon pepr3d::TriangleDetail::polygonFromTriangle(const PeprTriangle& tri) const {
    const Point2 a = toExactK(mOriginalPlane.to_2d(tri.vertex(0)));
    Point2 b = toExactK(mOriginalPlane.to_2d(tri.vertex(1)));
    Point2 c = toExactK(mOriginalPlane.to_2d(tri.vertex(2)));

    Polygon pgn;
    pgn.push_back(a);
    pgn.push_back(b);
    pgn.push_back(c);

    assert(!pgn.is_empty());

    if(pgn.is_clockwise_oriented())
        pgn.reverse_orientation();

    return pgn;
}

TriangleDetail::Polygon TriangleDetail::polygonFromCircle(const Circle3& circle) {
    // We don't care about exactness here, so we do all the math using standard kernel

    // Scale the vertex count based on the size of the circle
    const double radius = sqrt(CGAL::to_double(circle.squared_radius()));

    size_t vertexCount = static_cast<size_t>(radius * VERTICES_PER_UNIT_CIRCLE);
    vertexCount = std::max(vertexCount, static_cast<size_t>(3));
    const auto base1 = mOriginalPlane.base1() / CGAL::sqrt(mOriginalPlane.base1().squared_length());
    const auto base2 = mOriginalPlane.base2() / CGAL::sqrt(mOriginalPlane.base2().squared_length());

    assert(base1 * base2 == 0);
    assert(base1.squared_length() == 1);
    assert(base2.squared_length() == 1);

    const PeprPoint3 circleOrigin = toNormalK(circle.center());
    // Construct the polygon.
    Polygon pgn;
    for(size_t i = 0; i < vertexCount; i++) {
        const double circleCoord = (static_cast<double>(i) / vertexCount) * 2 * glm::pi<double>();
        const PeprPoint3 pt = circleOrigin + base1 * cos(circleCoord) * radius + base2 * sin(circleCoord) * radius;

        pgn.push_back(toExactK(mOriginalPlane.to_2d(pt)));
    }

    return pgn;
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

std::map<size_t, TriangleDetail::PolygonSet> TriangleDetail::gatherTrianglesIntoPolys() {
    std::map<size_t, PolygonSet> result;

    for(const DataTriangle& tri : mTriangles) {
        assert(tri.getTri().squared_area() > 0);
        result[tri.getColor()].join(polygonFromTriangle(tri.getTri()));
    }

    return result;
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
            const glm::vec3 a = toGlmVec(mOriginalPlane.to_3d(toNormalK(faceIt->vertex(0)->point())));
            const glm::vec3 b = toGlmVec(mOriginalPlane.to_3d(toNormalK(faceIt->vertex(1)->point())));
            const glm::vec3 c = toGlmVec(mOriginalPlane.to_3d(toNormalK(faceIt->vertex(2)->point())));

            DataTriangle tri(a, b, c, mOriginal.getNormal());
            if(tri.getTri().squared_area() > 0) {
                tri.setColor(color);

                // Make sure that the original counter-clockwise order is preserved
                if(glm::dot(mOriginal.getNormal(), glm::cross((b - a), (c - a))) < 0) {
                    tri = DataTriangle(a, c, b, mOriginal.getNormal());
                }

                mTriangles.emplace_back(std::move(tri));
            }
        }
    }
}

void TriangleDetail::createNewTriangles(std::map<size_t, PolygonSet>& coloredPolys) {
    mTriangles.clear();

    for(auto& it : coloredPolys) {
        if(it.second.is_empty())
            continue;

        bool simplified = false;
        std::vector<PolygonWithHoles> polys(it.second.number_of_polygons_with_holes());
        it.second.polygons_with_holes(polys.begin());
        for(PolygonWithHoles& poly : polys) {
            simplified |= simplifyPolygon(poly);
            addTrianglesFromPolygon(poly, it.first);
        }

        // Update this polygon set with simplified representation
        if(simplified) {
            it.second.clear();
            for(PolygonWithHoles& poly : polys) {
                it.second.join(poly);
            }
        }
    }
}
}  // namespace pepr3d
