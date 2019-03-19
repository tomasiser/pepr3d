#include "peprassert.h"  //Must be above everything else, or Cinder will eat asserts

#include "geometry/GeometryUtils.h"
#include "geometry/TriangleDetail.h"

#include <CGAL/Boolean_set_operations_2.h>
#include <CGAL/Exact_spherical_kernel_3.h>
#include <CGAL/Gps_traits_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_set_2.h>
#include <CGAL/Spherical_kernel_intersections.h>
#include <CGAL/partition_2.h>

#ifdef PEPR3D_COLLECT_DEBUG_DATA
#include <cereal/archives/json.hpp>
#include <cereal/types/map.hpp>
#include <fstream>
#endif

#include <cinder/Log.h>
#include <deque>
#include <list>
#include <optional>
#include <stdexcept>
#include <type_traits>

#ifndef NDEBUG
#include "geometry/GnuplotDebugHelper.h"
#endif

namespace pepr3d {

void TriangleDetail::paintSphere(const PeprSphere& peprSphere, int minSegments, size_t color) {
    // Vertices on the triangle boundaries must be the same across multiple triangle details!

    const Sphere sphere(toExactK(peprSphere.center()), peprSphere.squared_radius());
    auto intersection = CGAL::intersection(sphere, mOriginalPlane);

    if(!intersection) {
        return;
    }

    std::optional<Circle3> circleIntersection = boost::apply_visitor(SphereIntersectionVisitor{}, *intersection);

    // Continue only if the intersection is a circle (not a point or miss)
    if(circleIntersection) {
        auto poly = polygonFromCircle(*circleIntersection, minSegments);
        addPolygon(poly, color);
    }
}
TriangleDetail::Polygon TriangleDetail::projectShapeToPolygon(const std::vector<PeprPoint3>& shape,
                                                              const PeprVector3& direction) {
    P_ASSERT(shape.size() >= 3);

    // Create a polygon of the shape
    Polygon pgn;
    for(size_t i = 0; i < shape.size(); ++i) {
        auto lineFromPoint = Line3(toExactK(shape[i]), toExactK(direction));
        auto intersection = CGAL::intersection(lineFromPoint, mOriginalPlane);

        if(intersection) {
            Point3* intersectionPoint = boost::get<Point3>(&(*intersection));
            if(intersectionPoint) {
                pgn.push_back(mOriginalPlane.to_2d(*intersectionPoint));
            }
        }
    }

    if(pgn.size() < 3) {
        return {};
    }

    if(!pgn.is_simple()) {
        CI_LOG_E("Polygon not simple!");
        return {};
    }

    if(pgn.is_clockwise_oriented()) {
        pgn.reverse_orientation();
    }

    P_ASSERT(CGAL::is_valid_polygon(pgn, Traits()));

    return pgn;
}
void TriangleDetail::paintShape(const std::vector<PeprPoint3>& shape, const PeprVector3& direction, size_t color) {
    addPolygon(projectShapeToPolygon(shape, direction), color);
}

void TriangleDetail::paintShape(const std::vector<PeprTriangle>& triangles, const PeprVector3& direction,
                                size_t color) {
    std::vector<Polygon> polygons;
    polygons.reserve(triangles.size());
    for(const auto& tri : triangles) {
        // Construct new points instead of a copy (to be safe in multithreaded environment)
        std::vector<PeprPoint3> points = {PeprPoint3(tri.vertex(0).x(), tri.vertex(0).y(), tri.vertex(0).z()),
                                          PeprPoint3(tri.vertex(1).x(), tri.vertex(1).y(), tri.vertex(1).z()),
                                          PeprPoint3(tri.vertex(2).x(), tri.vertex(2).y(), tri.vertex(2).z())};
        Polygon pgn = projectShapeToPolygon(points, direction);
        // Add only if intersects the bounds
        if(trianglePolygonsDoIntersect(pgn, mBounds)) {
            polygons.emplace_back(std::move(pgn));
        }
    }
    PolygonSet pSet{};
    pSet.join(polygons.begin(), polygons.end());

    addPolygonSet(pSet, color);
}

std::pair<bool, bool> TriangleDetail::correctSharedVertices(TriangleDetail& other) {
    const Segment3 sharedEdge = findSharedEdge(other);
    if(mColorChanged) {
        updatePolysFromTriangles();
    }
    if(other.mColorChanged) {
        other.updatePolysFromTriangles();
    }

    std::set<Point3> myPoints = findPointsOnEdge(sharedEdge);
    P_ASSERT(myPoints.size() >= 2);
    std::set<Point3> theirPoints = other.findPointsOnEdge(sharedEdge);
    P_ASSERT(theirPoints.size() >= 2);

    const bool myPointsAdded = addMissingPoints(myPoints, theirPoints, sharedEdge);
    const bool otherPointsAdded = other.addMissingPoints(theirPoints, myPoints, sharedEdge);
    return std::make_pair(myPointsAdded, otherPointsAdded);
}

bool TriangleDetail::addMissingPoints(const std::set<Point3>& myPoints, const std::set<Point3>& theirPoints,
                                      const Segment3& sharedEdge) {
#ifdef PEPR3D_COLLECT_DEBUG_DATA
    history.emplace_back(PointEntry{myPoints, theirPoints, sharedEdge});
#endif

    if(mColorChanged) {
        updatePolysFromTriangles();
    }

    // Find missing points
    std::set<Point3> missingPoints;
    std::set_difference(theirPoints.begin(), theirPoints.end(), myPoints.begin(), myPoints.end(),
                        std::inserter(missingPoints, missingPoints.begin()));

    if(missingPoints.empty()) {
        return false;
    }

    for(const auto& pt : missingPoints) {
        if(!sharedEdge.has_on(pt)) {
            CI_LOG_E("3D Point is not on 3D shared edge. Possibly invalid input data.");
            throw std::logic_error("3D Point is not on 3D shared edge. Possibly invalid input data.");
        }
    }

    // Bring the 3d points to our plane
    std::vector<Point2> points2D;
    std::transform(missingPoints.begin(), missingPoints.end(), std::back_inserter(points2D),
                   [this](auto& e) { return mOriginalPlane.to_2d(e); });

    const Line2 sharedEdge2D(mOriginalPlane.to_2d(sharedEdge.vertex(0)), mOriginalPlane.to_2d(sharedEdge.vertex(1)));

    // Find edges that contain any of the points
    for(auto& colorSetIt : mColoredPolys) {
        std::vector<PolygonWithHoles> polys(colorSetIt.second.number_of_polygons_with_holes());
        colorSetIt.second.polygons_with_holes(polys.begin());

        for(PolygonWithHoles& polyWithHoles : polys) {
            Polygon& poly = polyWithHoles.outer_boundary();
            P_ASSERT(GeometryUtils::is_valid_polygon_with_holes(polyWithHoles, Traits()));

            for(size_t vertIdx = 0; vertIdx < poly.size();) {
                auto vertIt = poly.vertices_circulator() + vertIdx;
                auto nextVertIt = std::next(vertIt);

                if(points2D.empty()) {
                    break;
                }

                // Is this an edge of the whole triangle?
                if(sharedEdge2D.has_on(*vertIt) && sharedEdge2D.has_on(*nextVertIt)) {
                    const Segment2 edgeSegment(*vertIt, *nextVertIt);
                    // Test this segment against all points to see if we split
                    auto pointIt = std::find_if(points2D.begin(), points2D.end(),
                                                [edgeSegment](Point2& pt) { return edgeSegment.has_on(pt); });

                    if(pointIt != points2D.end()) {
                        if(nextVertIt == poly.vertices_circulator()) {
                            // Put a new vertex at the end of the polygon instead of the start
                            poly.push_back(*pointIt);
                        } else {
                            poly.insert(nextVertIt, *pointIt);
                        }

                        P_ASSERT(GeometryUtils::is_valid_polygon_with_holes(polyWithHoles, Traits()));

                        std::swap(*pointIt, points2D.back());
                        points2D.pop_back();  // Remove the point from array

                        continue;  // stay at this vertex
                    }
                }

                vertIdx++;
            }
        }

        // Insert the polygon back to color set
        colorSetIt.second.clear();
        colorSetIt.second.join(polys.begin(), polys.end());
    }

    if(!points2D.empty()) {
        CI_LOG_E("Some shared points could not be added!");

#ifdef PEPR3D_COLLECT_DEBUG_DATA
        CI_LOG_E("Bounds:");
        {
            std::stringstream sstream;
            sstream << mOriginal.getTri();
            CI_LOG_E(sstream.str());
        }
        CI_LOG_E("History:");
        std::stringstream sstream;
        {
            cereal::JSONOutputArchive jsonArchive(sstream);
            jsonArchive(history);
        }
        CI_LOG_E(sstream.str());
#endif
        throw std::logic_error(
            "Could not add matching vertex to a shared triangle edge. This was likely caused by corrupted internal "
            "state.");
    }
    return true;
}

void TriangleDetail::simplifyPolygons() {
    P_ASSERT(!mColorChanged);  // Did you forget to updatePolygons first?

    for(auto& colorSetIt : mColoredPolys) {
        if(colorSetIt.second.is_empty())
            continue;

        bool updateNeeded = false;
        std::vector<PolygonWithHoles> polys(colorSetIt.second.number_of_polygons_with_holes());
        colorSetIt.second.polygons_with_holes(polys.begin());
        for(PolygonWithHoles& poly : polys) {
            P_ASSERT(GeometryUtils::is_valid_polygon_with_holes(poly, Traits()));
            updateNeeded |= GeometryUtils::simplifyPolygon(poly.outer_boundary());
            P_ASSERT(GeometryUtils::is_valid_polygon_with_holes(poly, Traits()));
        }

        // Update this polygon set with simplified representation
        if(updateNeeded) {
            colorSetIt.second.clear();
            colorSetIt.second.join(polys.begin(), polys.end());
        }
    }
}

std::set<TriangleDetail::Point3> TriangleDetail::findPointsOnEdge(const TriangleDetail::Segment3& edge) {
    Line2 edgeLine(mOriginalPlane.to_2d(edge.point(0)), mOriginalPlane.to_2d(edge.point(1)));
    std::set<Point3> result;

    for(auto& colorSetIt : mColoredPolys) {
        std::vector<PolygonWithHoles> polys(colorSetIt.second.number_of_polygons_with_holes());
        colorSetIt.second.polygons_with_holes(polys.begin());
        for(PolygonWithHoles& polyWithHoles : polys) {
            Polygon& poly = polyWithHoles.outer_boundary();
            for(auto vertexIt = poly.vertices_begin(); vertexIt != poly.vertices_end(); vertexIt++) {
                if(edgeLine.has_on(*vertexIt)) {
                    result.insert(mOriginalPlane.to_3d(*vertexIt));
                }
            }
        }
    }

    return result;
}

TriangleDetail::Polygon pepr3d::TriangleDetail::polygonFromTriangle(const PeprTriangle& tri) const {
    const Point2 a = mOriginalPlane.to_2d(toExactK(tri.vertex(0)));
    const Point2 b = mOriginalPlane.to_2d(toExactK(tri.vertex(1)));
    const Point2 c = mOriginalPlane.to_2d(toExactK(tri.vertex(2)));

    Polygon pgn;
    pgn.push_back(a);
    pgn.push_back(b);
    pgn.push_back(c);

    P_ASSERT(!pgn.is_empty());

    if(pgn.is_clockwise_oriented())
        pgn.reverse_orientation();

    P_ASSERT(pgn.is_counterclockwise_oriented());
    P_ASSERT(CGAL::is_valid_polygon(pgn, Traits()));

    return pgn;
}

TriangleDetail::Polygon TriangleDetail::polygonFromTriangle(const Triangle2& tri) {
    Polygon pgn;
    pgn.push_back(tri.vertex(0));
    pgn.push_back(tri.vertex(1));
    pgn.push_back(tri.vertex(2));

    P_ASSERT(!pgn.is_empty());

    if(pgn.is_clockwise_oriented())
        pgn.reverse_orientation();

    P_ASSERT(pgn.is_counterclockwise_oriented());
    P_ASSERT(CGAL::is_valid_polygon(pgn, Traits()));
    return pgn;
}

std::vector<std::pair<TriangleDetail::Point2, double>> TriangleDetail::getCircleSharedPoints(
    const Circle3& circle, const Vector3& xBase, const Vector3& yBase) const {
    // We need shared verticies on the boundary of triangle details
    // This vertex needs to be the same for both neighbouring triangles
    // Thats why we calculate the intersection using original world-space data

    Sphere sphere(circle.center(), circle.squared_radius());
    std::vector<std::pair<Point2, double>> result;

    // Find all points that intersect triangle edge
    for(size_t i = 0; i < 3; i++) {
        std::array<Point3, 2> vertices{toExactK(mOriginal.getVertex(i)), toExactK(mOriginal.getVertex((i + 1) % 3))};
        if(vertices[0] >= vertices[1]) {
            std::swap(vertices[0],
                      vertices[1]);  // Makes sure the result of method calculation is same for both triangles
        }

        Line3 triEdge(vertices[0], vertices[1]);

        std::vector<CGAL::Object> intersections;
        auto intersection = CGAL::intersection(sphere, triEdge, std::back_inserter(intersections));

        // Add both intersection points of this edge
        for(auto& obj : intersections) {
            std::pair<K::Circular_arc_point_3, unsigned> ptPair;
            if(CGAL::assign(ptPair, obj)) {
                K::Circular_arc_point_3& pt = ptPair.first;
                Point3 worldPoint(CGAL::to_double(pt.x()), CGAL::to_double(pt.y()),
                                  CGAL::to_double(pt.z()));  // Cannot get exact

                // Make sure the point is exactly on the line
                worldPoint = triEdge.projection(worldPoint);

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

TriangleDetail::Polygon TriangleDetail::polygonFromCircle(const Circle3& circle, int minSegments) const {
    P_ASSERT(minSegments >= 3);

    // Scale the vertex count based on the size of the circle
    const double radius = sqrt(CGAL::to_double(circle.squared_radius()));

    // Bases for the points of the circle (cannot be exact, because Epeck does not support sqrt)
    const auto xBase = mOriginalPlane.base1() / CGAL::sqrt(CGAL::to_double(mOriginalPlane.base1().squared_length()));
    const auto yBase = mOriginalPlane.base2() / CGAL::sqrt(CGAL::to_double(mOriginalPlane.base2().squared_length()));
    P_ASSERT(xBase * yBase == 0);

    // We need a shared vertex on the boundary of triangle details
    // This vertex does not need to be exact, but needs to be the same from both triangles
    std::vector<std::pair<Point2, double>> sharedPoints = getCircleSharedPoints(circle, xBase, yBase);
    auto sharedPointIt = sharedPoints.begin();

    // Construct the polygon.
    Polygon pgn;
    for(size_t i = 0; i < minSegments; i++) {
        const double circleCoord = (static_cast<double>(i) / minSegments) * 2 * glm::pi<double>();
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

    P_ASSERT(pgn.is_simple());
    P_ASSERT(pgn.is_counterclockwise_oriented());
    P_ASSERT(pgn.is_convex());
    P_ASSERT(CGAL::is_valid_polygon(pgn, Traits()));

    return pgn;
}

void TriangleDetail::addPolygon(const Polygon& poly, size_t color) {
#ifdef PEPR3D_COLLECT_DEBUG_DATA
    history.emplace_back(PolygonEntry{poly, color});
#endif

    if(poly.is_empty()) {
        return;
    }

    P_ASSERT(CGAL::is_valid_polygon(poly, Traits()));
    PolygonSet addedShape(poly);
    addPolygonSet(addedShape, color);
}

void TriangleDetail::addPolygonSet(PolygonSet& polySet, size_t color) {
#ifdef PEPR3D_COLLECT_DEBUG_DATA
    history.emplace_back(PolygonSetEntry{polySet, color});
#endif

    polySet.intersection(mBounds);

    if(mColorChanged) {
        updatePolysFromTriangles();
    }

    // Add the shape to its color layer
    mColoredPolys[color].join(polySet);

    // Remove the new shape from other colors
    for(auto& it : mColoredPolys) {
        if(it.first != color) {
            debugOnlyVerifyPolygonSet(it.second);

            it.second.difference(polySet);

            debugOnlyVerifyPolygonSet(it.second);
        }
    }

    simplifyPolygons();
    updateTrianglesFromPolygons();
}

TriangleDetail::Segment3 TriangleDetail::findSharedEdge(const TriangleDetail& other) {
    std::array<PeprPoint3, 2> commonPoints;
    size_t pointsFound = 0;

    // Find the two triangle vertices that are the same for both triangles
    for(int i = 0; i < 3; i++) {
        const PeprPoint3 myPoint = mOriginal.getTri().vertex(i);
        for(int j = 0; j < 3; j++) {
            const PeprPoint3 otherPoint = other.mOriginal.getTri().vertex(j);
            if(myPoint == otherPoint) {
                commonPoints[pointsFound++] = myPoint;
            }
        }
    }

    P_ASSERT(pointsFound == 2);
    return Segment3(toExactK(commonPoints[0]), toExactK(commonPoints[1]));
}

void TriangleDetail::updatePolysFromTriangles() {
    debugEdgeConsistencyCheck();

    mColoredPolys = createPolygonSetsFromTriangles(mTrianglesExact);
    mColorChanged = false;
    debugEdgeConsistencyCheck();

    simplifyPolygons();
}

std::map<size_t, TriangleDetail::PolygonSet> TriangleDetail::createPolygonSetsFromTriangles(
    const std::vector<ExactTriangle>& trianglesExact) {
    std::map<size_t, PolygonSet> coloredPolygonSets;

    // Create polygons from triangles
    std::map<size_t, std::vector<Polygon>> polygonsByColor;
    for(const ExactTriangle& exactTri : trianglesExact) {
        polygonsByColor[exactTri.color].emplace_back(polygonFromTriangle(exactTri.triangle));
    }

    for(const auto& it : polygonsByColor) {
        const std::vector<Polygon>& polygons = it.second;

        P_ASSERT(std::all_of(polygons.begin(), polygons.end(), [](const auto& poly) {
            return CGAL::is_valid_polygon(poly, Traits()) && poly.is_counterclockwise_oriented();
        }));

        // Must be joined all at the same time
        // Otherwise cgal creates PolygonSet with invalid holes (vertices of higher degree)
        PolygonSet pSet;
        pSet.join(polygons.begin(), polygons.end());
        TriangleDetail::debugOnlyVerifyPolygonSet(pSet);

        coloredPolygonSets.emplace(std::make_pair(it.first, std::move(pSet)));
    }

    return coloredPolygonSets;
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
    std::vector<Triangle2> newTriangles = triangulatePolygon(poly);

    // Create new array for degenerate triangles
    const size_t polygonId = mPolygonDegenerateTriangles.size();
    mPolygonDegenerateTriangles.push_back({});

    // Store all triangles
    for(Triangle2& exactTri : newTriangles) {
        const size_t idxOfExactTri = mTrianglesExact.size();

        const glm::vec3 a = toGlmVec(mOriginalPlane.to_3d(exactTri.vertex(0)));
        const glm::vec3 b = toGlmVec(mOriginalPlane.to_3d(exactTri.vertex(1)));
        const glm::vec3 c = toGlmVec(mOriginalPlane.to_3d(exactTri.vertex(2)));
        DataTriangle tri(a, b, c, mOriginal.getNormal());
        if(!tri.getTri().is_degenerate()) {
            // Triangle is good
            tri.setColor(color);
            mTriangles.emplace_back(std::move(tri));
            mTrianglesToExactIdx.push_back(mTrianglesExact.size());
        } else {
            // Triangle degenerates
            mPolygonDegenerateTriangles[polygonId].push_back(idxOfExactTri);
        }

        mTrianglesExact.emplace_back(std::move(exactTri), color, polygonId);
    }

    P_ASSERT(mTriangles.size() == mTrianglesToExactIdx.size());
}

std::vector<TriangleDetail::Triangle2> TriangleDetail::triangulatePolygon(const PolygonWithHoles& poly) {
    P_ASSERT(GeometryUtils::is_valid_polygon_with_holes(poly, Traits()));

    std::vector<Triangle2> triangles;

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
            Triangle2 tri(faceIt->vertex(0)->point(), faceIt->vertex(1)->point(), faceIt->vertex(2)->point());
            if(!tri.is_degenerate()) {
                triangles.emplace_back(tri);
            }
        }
    }

    return triangles;
}

void TriangleDetail::updateTrianglesFromPolygons() {
    mTriangles.clear();
    mTrianglesToExactIdx.clear();
    mTrianglesExact.clear();
    mPolygonDegenerateTriangles.clear();

    debugEdgeConsistencyCheck();

    for(auto& colorSetIt : mColoredPolys) {
        if(colorSetIt.second.is_empty())
            continue;

        std::vector<PolygonWithHoles> polys(colorSetIt.second.number_of_polygons_with_holes());
        colorSetIt.second.polygons_with_holes(polys.begin());
        for(PolygonWithHoles& poly : polys) {
            addTrianglesFromPolygon(poly, colorSetIt.first);
        }
    }

    P_ASSERT(mTriangles.size() == mTrianglesToExactIdx.size());
}

void TriangleDetail::setColor(size_t detailIdx, size_t color) {
    P_ASSERT(detailIdx < mTriangles.size());
    P_ASSERT(mTriangles.size() == mTrianglesToExactIdx.size());

    if(mTriangles[detailIdx].getColor() != color) {
        // Update the inexact representation
        mTriangles[detailIdx].setColor(color);

        // Update the exact representation
        const size_t exactTriIdx = mTrianglesToExactIdx[detailIdx];
        P_ASSERT(exactTriIdx < mTrianglesExact.size());
        mTrianglesExact[exactTriIdx].color = color;

        // Also changle all degenerate triangles of this polygon to this colour
        // These are not otherwise accessible by DetailedTriangleId, but not coloring these
        // would prevent simplification in case of fill.
        const size_t polygonIdx = mTrianglesExact[exactTriIdx].polygonIdx;
        for(size_t exactDegTriIdx : mPolygonDegenerateTriangles[polygonIdx]) {
            mTrianglesExact[exactDegTriIdx].color = color;
        }

        mColorChanged = true;

#ifdef PEPR3D_COLLECT_DEBUG_DATA
        history.emplace_back(ColorChangeEntry{detailIdx, color});
#endif
    }
}
}  // namespace pepr3d
