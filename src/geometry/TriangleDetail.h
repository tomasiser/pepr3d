#pragma once
/**
 *  Fix for CGAL bugs in polygon with holes verification
 *  Contained in CGAL PR #3688, likely to be in 4.14 version of CGAL
 *
 *  For now, just keep this custom patched include file first, to avoid using CGAL's version
 */
#include "CGAL-patched/Boolean_set_operations_2/Gps_traits_adaptor.h"
//---------------------------------------

#include "geometry/Triangle.h"

#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Exact_spherical_kernel_3.h>
#include <CGAL/General_polygon_2.h>
#include <CGAL/IO/io.h>
#include <CGAL/Lazy_exact_nt.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_set_2.h>
#include <CGAL/Polygon_triangulation_decomposition_2.h>
#include <CGAL/Polygon_with_holes_2.h>

#include <cereal/access.hpp>
#include <cereal/external/base64.hpp>

#include <cereal/types/set.hpp>
#include <cereal/types/vector.hpp>
#include <deque>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "GeometryUtils.h"
#include "geometry/GlmSerialization.h"

#ifdef PEPR3D_COLLECT_DEBUG_DATA
#include <boost/variant.hpp>
#include <cereal/types/boost_variant.hpp>
#endif

namespace pepr3d {
/**
 * Represents additional triangles that replace the original one. Allows "painting" over the original triangle
 * by creating a polygonal representation of the new shapes, which is then converted back to triangles.
 */
class TriangleDetail {
   public:
    using K = CGAL::Exact_spherical_kernel_3;
    using Polygon = CGAL::Polygon_2<K>;
    using PolygonWithHoles = CGAL::Polygon_with_holes_2<K>;
    using GeneralPolygon = CGAL::General_polygon_2<K>;
    using Circle = TriangleDetail::K::Circle_2;
    using Circle3 = TriangleDetail::K::Circle_3;
    using Sphere = TriangleDetail::K::Sphere_3;
    using Triangle2 = TriangleDetail::K::Triangle_2;

    using PolygonSet = CGAL::Polygon_set_2<K>;
    using Traits = TriangleDetail::PolygonSet::Traits_2;
    using PeprPoint2 = DataTriangle::K::Point_2;
    using PeprPoint3 = DataTriangle::K::Point_3;
    using PeprVector3 = DataTriangle::K::Vector_3;
    using Point2 = TriangleDetail::K::Point_2;
    using Point3 = TriangleDetail::K::Point_3;
    using Vector2 = TriangleDetail::K::Vector_2;
    using Vector3 = TriangleDetail::K::Vector_3;
    using Plane = TriangleDetail::K::Plane_3;
    using Line3 = TriangleDetail::K::Line_3;
    using Line2 = TriangleDetail::K::Line_2;
    using Segment3 = TriangleDetail::K::Segment_3;
    using Segment2 = TriangleDetail::K::Segment_2;

    using PeprTriangle = DataTriangle::Triangle;
    using PeprPlane = DataTriangle::K::Plane_3;
    using PeprSphere = DataTriangle::K::Sphere_3;

    /// Used in triangulation to distinguish filled faces from holes
    struct FaceInfo {
        int nestingLevel = -1;
    };
    using Fbb = CGAL::Triangulation_face_base_with_info_2<FaceInfo, K>;
    using Fb = CGAL::Constrained_triangulation_face_base_2<K, Fbb>;
    using Tds = CGAL::Triangulation_data_structure_2<CGAL::Triangulation_vertex_base_2<K>, Fb>;
    using ConstrainedTriangulation = CGAL::Constrained_triangulation_2<K, Tds, CGAL::No_intersection_tag>;

    /// Exact triangle with colour and polygon information
    struct ExactTriangle {
        ExactTriangle(Triangle2& tri, size_t color, size_t polygonIdx)
            : triangle(tri), color(color), polygonIdx(polygonIdx) {}
        ExactTriangle(Triangle2&& tri, size_t color, size_t polygonIdx)
            : triangle(tri), color(color), polygonIdx(polygonIdx) {}
        ExactTriangle() = default;
        ExactTriangle(const ExactTriangle&) = default;
        ExactTriangle(ExactTriangle&&) = default;

        /// Epeck representation of the triangle
        Triangle2 triangle;

        size_t color;

        /// Idx of the polygon this triangle belongs to
        size_t polygonIdx;

        template <typename Archive>
        void serialize(Archive& archive) {
            archive(cereal::make_nvp("triangle", triangle), cereal::make_nvp("color", color),
                    cereal::make_nvp("polygonIdx", polygonIdx));
        }
    };

#ifdef PEPR3D_COLLECT_DEBUG_DATA
    struct PolygonEntry {
        Polygon polygon;
        size_t color;

        template <typename Archive>
        void serialize(Archive& archive) {
            archive(cereal::make_nvp("polygon", polygon), cereal::make_nvp("color", color));
        }
    };

    struct PointEntry {
        std::set<Point3> myPoints;
        std::set<Point3> theirPoints;
        Segment3 sharedEdge;

        template <typename Archive>
        void serialize(Archive& archive) {
            archive(cereal::make_nvp("myPoints", myPoints), cereal::make_nvp("theirPoints", theirPoints),
                    cereal::make_nvp("sharedEdge", sharedEdge));
        }
    };

    struct ColorChangeEntry {
        size_t detailIdx;
        size_t color;
        template <typename Archive>
        void serialize(Archive& archive) {
            archive(cereal::make_nvp("detailIdx", detailIdx), cereal::make_nvp("color", color));
        }
    };

    using HistoryEntry = boost::variant<PolygonEntry, PointEntry, ColorChangeEntry>;

#endif

    explicit TriangleDetail(const DataTriangle& original) : mOriginal(original) {
        const PeprTriangle& tri = mOriginal.getTri();
        mOriginalPlane = Plane(toExactK(tri.vertex(0)), toExactK(tri.vertex(1)), toExactK(tri.vertex(2)));
        mBounds = polygonFromTriangle(mOriginal.getTri());
        mTriangles.push_back(mOriginal);
        mTrianglesToExactIdx.push_back(0);

        std::array<Point2, 3> exactPoints;
        for(int i = 0; i < 3; i++) {
            exactPoints[i] = mOriginalPlane.to_2d(toExactK(mOriginal.getTri().vertex(i)));
        }
        mTrianglesExact.emplace_back(Triangle2(exactPoints[0], exactPoints[1], exactPoints[2]), original.getColor(), 0);
        mColoredPolys.emplace(mOriginal.getColor(), PolygonSet(mBounds));
        mPolygonDegenerateTriangles.push_back({});
    }

    // Cereal requires default constructor
    TriangleDetail() = default;

    /// Paint sphere onto this detail
    /// @param minSegments Minimum number of segments of each sphere/plane intersection. Additional points may be added
    /// on boundaries.
    void paintSphere(const PeprSphere& sphere, int minSegments, size_t color);

    /// Paint a shape to triangle detail
    /// @param shape Collection of points that form a polygon, that is going to be projected onto the TriangleDetail
    /// @param direction Direction vector of the projection
    void paintShape(const std::vector<PeprPoint3>& shape, const PeprVector3& direction, size_t color);

    /// Makes sure all vertices on the common edge between these two triangles are matched
    /// Creates new vertices for both triangles if there are missing
    /// You will need to updateTrianglesFromPolygons() after calling this method!
    /// @return <bool,bool> true if points were added to a triangle
    std::pair<bool, bool> correctSharedVertices(TriangleDetail& other);

    const std::vector<DataTriangle>& getTriangles() const {
        return mTriangles;
    }

    const DataTriangle& getOriginal() const {
        return mOriginal;
    }

    /// Create new triangles from a set of colored polygons
    /// Tries to simplify the polygons in the process
    void updateTrianglesFromPolygons();

    /// Set color of a detail triangle
    void setColor(size_t detailIdx, size_t color);

    /// Add polygon to the detail
    /// @param poly Polygon in the plane-space of this detail
    void addPolygon(const Polygon& poly, size_t color);

    /// Find all points of polygons that are on the edge
    std::set<Point3> findPointsOnEdge(const Segment3& edge);

    template <class Archive>
    void save(Archive& archive) const {
        if(mColorChanged) {
            // Update polygonal representation so that we can save it
            TriangleDetail* mutableThis = const_cast<TriangleDetail*>(this);
            mutableThis->updatePolysFromTriangles();
        }

        archive(mOriginal, mColoredPolys);
    }

    template <class Archive>
    void load(Archive& archive) {
        archive(mOriginal, mColoredPolys);
        const PeprTriangle& tri = mOriginal.getTri();
        mOriginalPlane = Plane(toExactK(tri.vertex(0)), toExactK(tri.vertex(1)), toExactK(tri.vertex(2)));
        mBounds = polygonFromTriangle(mOriginal.getTri());

        updateTrianglesFromPolygons();
    }

    /// Convert Point_2 from Pepr3d kernel to Exact kernel
    inline static K::Point_2 toExactK(const PeprPoint2& point) {
        return K::Point_2(point.x(), point.y());
    }

    /// Convert Point_3 from Pepr3d kernel to Exact kernel
    inline static K::Point_3 toExactK(const PeprPoint3& point) {
        return K::Point_3(point.x(), point.y(), point.z());
    }

    /// Convert glm::vec3 to Exact kernel
    inline static K::Point_3 toExactK(const glm::vec3& vec) {
        return Point3(vec.x, vec.y, vec.z);
    }

    /// Convert Point_3 from Pepr3d kernel to Exact kernel
    inline static K::Vector_3 toExactK(const PeprVector3& vec) {
        return K::Vector_3(vec.x(), vec.y(), vec.z());
    }

    /// Convert Exact kernel Point_2 to normal Pepr3d kernel
    inline static PeprPoint2 toNormalK(const K::Point_2& point) {
        return PeprPoint2(exactToDbl(point.x()), exactToDbl(point.y()));
    }

    /// Convert Exact kernel Point_3 to normal Pepr3d kernel
    inline static PeprPoint3 toNormalK(const K::Point_3& point) {
        return PeprPoint3(exactToDbl(point.x()), exactToDbl(point.y()), exactToDbl(point.z()));
    }

    /// Convert Pepr Point_3 to vec3
    inline static glm::vec3 toGlmVec(const PeprPoint3& pt) {
        return glm::vec3(pt.x(), pt.y(), pt.z());
    }

    /// Convert Exact kernel Point_3 to vec3
    inline static glm::vec3 toGlmVec(const K::Point_3& point) {
        return toGlmVec(toNormalK(point));
    }

    static double exactToDbl(const CGAL::Gmpq& num) {
        return num.to_double();
    }

    static double exactToDbl(const CGAL::Gmpz& num) {
        return num.to_double();
    }

    /// Creates a map of [ColorID, PolygonSet] of polygon sets made of provided triangles
    /// @triangles array of DataTriangles, only used to get color information
    /// @trianglesExact array of Epeck Triangles, used to get exact bounds of each triangle
    static std::map<size_t, PolygonSet> createPolygonSetsFromTriangles(
        const std::vector<ExactTriangle>& trianglesExact);

    /// Break down a polygon into an array of triangles
    /// @return vector of exact triangles that make up the polygon
    static std::vector<Triangle2> triangulatePolygon(const PolygonWithHoles& poly);

    /// Change color IDs of this detail
    /// @param ColorFunc functor of type size_t func(size_t originalColor), that returns the new color ID
    template <typename ColorFunc>
    void changeColorIds(const ColorFunc& colorFunc) {
        if(!mColorChanged) {
            std::map<size_t, PolygonSet> coloredPolygonSets;

            for(auto& coloredSetIt : mColoredPolys) {
                coloredPolygonSets[colorFunc(coloredSetIt.first)].join(coloredSetIt.second);
            }
            mColoredPolys = std::move(coloredPolygonSets);
        }

        // Color of some triangles has been changed, polygon representation is old
        for(ExactTriangle& exactTri : mTrianglesExact) {
            exactTri.color = colorFunc(exactTri.color);
        }

        for(DataTriangle& dataTri : mTriangles) {
            dataTri.setColor(colorFunc(dataTri.getColor()));
        }
    }

   private:
    /// Temporary storage for DataTriangles.  This gets overwritten on every time updateTrianglesFromPolygons() is run.
    /// Triangles stored here are non-degenerate triangles that roughly make up the original triangle.
    /// Becasue DataTriangle is using a limited-precission, these triangles cannot be used to reconstruct the surface.
    std::vector<DataTriangle> mTriangles;

    /// Stores index of exact triangle to every DataTriangle of this detail (mTriangles.size() ==
    /// mTrianglesToExactIdx.size()) Every DataTriangle in this detail has matching exact triangle. But not all exact
    /// triangles have a DataTriange - some degenerate.
    std::vector<size_t> mTrianglesToExactIdx;

    /// Stores epeck triangles. This gets overwritten on every time updateTrianglesFromPolygons() is run.
    /// This is used for saving andpolygonset reconstruction
    std::vector<ExactTriangle> mTrianglesExact;

    /// Stores index into mTrianglesExact of every degenerate triangle(when represented as DataTriangle), grouped by the
    /// polygon it belongs to.
    std::vector<std::vector<size_t>> mPolygonDegenerateTriangles;

    std::map<size_t, PolygonSet> mColoredPolys;

    DataTriangle mOriginal;

    static const int VERTICES_PER_UNIT_CIRCLE = 50;
    static const int MIN_VERTICES_IN_CIRCLE = 12;

#ifdef PEPR3D_COLLECT_DEBUG_DATA
    std::vector<HistoryEntry> history;
#endif

   private:
    /// Bounds of the original triangle
    Polygon mBounds;

    Plane mOriginalPlane;

    /// Did color of any detail triangle change since last triangulation?
    bool mColorChanged = false;

    /// Get points of a circle that are shared with border triangles
    std::vector<std::pair<Point2, double>> getCircleSharedPoints(const Circle3& circle, const Vector3& xBase,
                                                                 const Vector3& yBase) const;

   private:
    /// Find shared edge between triangles
    Segment3 findSharedEdge(const TriangleDetail& other);

    Polygon projectShapeToPolygon(const std::vector<PeprPoint3>& shape, const PeprVector3& direction);

#ifdef _TEST_
   public:  // Testing requires access to these methods
#endif
            /// Add points that are missing to our polygons
    /// @return true if any points were added
    bool addMissingPoints(const std::set<Point3>& myPoints, const std::set<Point3>& theirPoints,
                          const Segment3& sharedEdge);

    /// Construct a polygon from a circle.
    Polygon polygonFromCircle(const Circle3& circle, int segments) const;

    /// Create a polygon from a PeprTriangle
    Polygon polygonFromTriangle(const PeprTriangle& tri) const;

    /// Create a polygon from 2D triangle in plane coordinates
    static Polygon polygonFromTriangle(const Triangle2& tri);

   private:
    /// Simplify polygons, removing any vertices that are collinear
    void simplifyPolygons();

    /// Generate one colored polygon set for each color inside the triangle
    /// This is a slow operation
    void updatePolysFromTriangles();

    /// Add triangles from this polygon to our triangles
    void addTrianglesFromPolygon(const PolygonWithHoles& poly, size_t color);

    /// ( From CGAL User Manual )
    /// ---------------------------
    /// explore set of facets connected with non constrained edges,
    /// and attribute to each such set a nesting level.
    static void markDomains(ConstrainedTriangulation& ct);

    static void markDomains(ConstrainedTriangulation& ct, ConstrainedTriangulation::Face_handle start, int index,
                            std::deque<ConstrainedTriangulation::Edge>& border);

    struct SphereIntersectionVisitor : public boost::static_visitor<std::optional<Circle3>> {
        std::optional<Circle3> operator()(const Circle3& circle) const {
            return circle;
        }

        std::optional<Circle3> operator()(const Point3) const {
            return {};
        }
    };

    /// Verify that polygon set is made of valid polygons with holes
    static void debugOnlyVerifyPolygonSet(const PolygonSet& pSet) {
#ifndef NDEBUG
        // This creates a copy of the data, so run it only in debug

        std::vector<PolygonWithHoles> polys(pSet.number_of_polygons_with_holes());
        pSet.polygons_with_holes(polys.begin());
        for(PolygonWithHoles& poly : polys) {
            assert(GeometryUtils::is_valid_polygon_with_holes(poly, Traits()));
        }
#endif
    }

    /// Verify that the bound vertices are connected via a string of edges
    void debugEdgeConsistencyCheck() {
#ifndef NDEBUG
#ifdef PEPR3D_EDGE_CONSISTENCY_CHECK
        for(int i = 0; i < 3; i++) {
            const Point2 firstPoint(mBounds.vertex(i));
            const Point2 secondPoint(mBounds.vertex((i + 1) % 3));

            if(!isEdgeTraversable(firstPoint, secondPoint, mColoredPolys)) {
                throw std::logic_error("Only one outgoing edge should exist on this line");
            }
        }
#endif
#endif
    }

   public:
    // Defined only for debug environments, to avoid accidentaly leaving this expensive check in
#if !defined(NDEBUG) || defined(PEPR3D_EDGE_CONSISTENCY_CHECK)
    /// Tests for an existance of a path between the two points. All edges of this path must lie on an edge between the
    /// original points.
    /// @return true A path exists
    static bool isEdgeTraversable(const Point2& form, const Point2& to,
                                  const std::map<size_t, PolygonSet>& coloredPolys) {
        const Segment2 sharedEdge = form < to ? Segment2(form, to) : Segment2(to, form);

        // Map a lower sorted point of the edge to a higher sorted point of the edge
        std::map<Point2, Point2> pointMap;

        for(auto& colorSetIt : coloredPolys) {
            std::vector<PolygonWithHoles> polys(colorSetIt.second.number_of_polygons_with_holes());
            colorSetIt.second.polygons_with_holes(polys.begin());

            for(PolygonWithHoles& polyWithHoles : polys) {
                Polygon& poly = polyWithHoles.outer_boundary();
                for(size_t vertIdx = 0; vertIdx < poly.size(); vertIdx++) {
                    auto vertIt = poly.vertices_circulator() + vertIdx;
                    auto nextVertIt = std::next(vertIt);

                    // If this edge is on the edge of bounds add it to the map
                    if(sharedEdge.has_on(*vertIt) && sharedEdge.has_on(*nextVertIt) && *vertIt != *nextVertIt) {
                        if(*vertIt < *nextVertIt) {
                            if(pointMap.find(*vertIt) != pointMap.end()) {
                                return false;
                            }
                            pointMap[*vertIt] = *nextVertIt;
                        } else {
                            if(pointMap.find(*nextVertIt) != pointMap.end()) {
                                return false;
                            }
                            pointMap[*nextVertIt] = *vertIt;
                        }
                    }
                }
            }
        }

        // Now test that the boundary edge is traversable with the gathered edges
        Point2 currentPt = sharedEdge.min();
        const Point2 endPt = sharedEdge.max();
        while(currentPt != endPt) {
            auto nextPtIt = pointMap.find(currentPt);
            if(nextPtIt == pointMap.end()) {
                return false;
            }

            currentPt = nextPtIt->second;
        }

        return true;
    }
#endif
};

}  // namespace pepr3d

/**
 *  CGAL Serialization functions
 */
// Note: Keep them tightly specialized, so that they don't try to serialize any other types
namespace CGAL {
template <typename Archive, typename CGALType,
          typename std::enable_if<std::is_same<CGALType, pepr3d::TriangleDetail::Point2>::value ||
                                  std::is_same<CGALType, pepr3d::TriangleDetail::Point3>::value ||
                                  std::is_same<CGALType, pepr3d::TriangleDetail::Triangle2>::value ||
                                  std::is_same<CGALType, pepr3d::TriangleDetail::Polygon>::value ||
                                  std::is_same<CGALType, pepr3d::TriangleDetail::PolygonWithHoles>::value ||
                                  std::is_same<CGALType, pepr3d::TriangleDetail::Segment3>::value ||
                                  std::is_same<CGALType, pepr3d::TriangleDetail::Segment2>::value>::type* = nullptr>
void save(Archive& archive, const CGALType& val) {
    std::stringstream stream;
    stream << val;
    archive(stream.str());
}
template <typename Archive, typename CGALType,
          typename std::enable_if<std::is_same<CGALType, pepr3d::TriangleDetail::Point2>::value ||
                                  std::is_same<CGALType, pepr3d::TriangleDetail::Point3>::value ||
                                  std::is_same<CGALType, pepr3d::TriangleDetail::Triangle2>::value ||
                                  std::is_same<CGALType, pepr3d::TriangleDetail::Polygon>::value ||
                                  std::is_same<CGALType, pepr3d::TriangleDetail::PolygonWithHoles>::value ||
                                  std::is_same<CGALType, pepr3d::TriangleDetail::Segment3>::value ||
                                  std::is_same<CGALType, pepr3d::TriangleDetail::Segment2>::value>::type* = nullptr>
void load(Archive& archive, CGALType& val) {
    std::string str;
    archive(str);
    std::stringstream stream(str);
    stream.seekg(stream.beg);
    stream >> val;
}

template <typename Archive>
void save(Archive& archive, const pepr3d::TriangleDetail::PolygonSet& pset) {
    std::vector<pepr3d::TriangleDetail::PolygonWithHoles> polys(pset.number_of_polygons_with_holes());
    pset.polygons_with_holes(polys.begin());

    std::stringstream sstream;
    sstream << pset.number_of_polygons_with_holes();
    for(auto& poly : polys) {
        sstream << " " << poly;
    }

    archive(sstream.str());
}

template <typename Archive>
void load(Archive& archive, pepr3d::TriangleDetail::PolygonSet& pset) {
    std::string str;
    archive(str);
    std::stringstream sstream(str);
    size_t numPolys;
    sstream >> numPolys;

    std::vector<pepr3d::TriangleDetail::PolygonWithHoles> polys(numPolys);
    for(size_t i = 0; i < numPolys; ++i) {
        sstream >> polys[i];
    }

    pset.clear();
    pset.join(polys.begin(), polys.end());
}

}  // namespace CGAL

namespace std {}