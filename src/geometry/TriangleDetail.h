#pragma once
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
#include <deque>
#include <map>
#include <optional>
#include <sstream>
#include <vector>

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
    using PeprPoint2 = DataTriangle::K::Point_2;
    using PeprPoint3 = DataTriangle::K::Point_3;
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

    // Used in triangulation to distinguish filled faces from holes
    struct FaceInfo {
        int nestingLevel = -1;
    };
    using Fbb = CGAL::Triangulation_face_base_with_info_2<FaceInfo, K>;
    using Fb = CGAL::Constrained_triangulation_face_base_2<K, Fbb>;
    using Tds = CGAL::Triangulation_data_structure_2<CGAL::Triangulation_vertex_base_2<K>, Fb>;
    using ConstrainedTriangulation = CGAL::Constrained_triangulation_2<K, Tds, CGAL::No_intersection_tag>;

    explicit TriangleDetail(const DataTriangle& original) : mOriginal(original) {
        const PeprTriangle& tri = mOriginal.getTri();
        mOriginalPlane = Plane(toExactK(tri.vertex(0)), toExactK(tri.vertex(1)), toExactK(tri.vertex(2)));
        mBounds = polygonFromTriangle(mOriginal.getTri());
        mTriangles.push_back(mOriginal);

        std::array<Point2, 3> exactPoints;
        for(int i = 0; i < 3; i++) {
            exactPoints[i] = mOriginalPlane.to_2d(toExactK(mOriginal.getTri().vertex(i)));
        }
        mTrianglesExact.emplace_back(exactPoints[0], exactPoints[1], exactPoints[2]);
        mColoredPolys.emplace(mOriginal.getColor(), polygonFromTriangle(mOriginal.getTri()));
    }

    // Cereal requires default constructor
    TriangleDetail() = default;

    TriangleDetail(const DataTriangle& original, std::vector<DataTriangle>&& detailTriangles)
        : TriangleDetail(original) {
        mTriangles = detailTriangles;
        updatePolysFromTriangles();
    }

    /// Paint sphere onto this detail
    void paintSphere(const PeprSphere& sphere, size_t color);

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
    void setColor(size_t detailIdx, size_t color) {
        assert(detailIdx < mTriangles.size());
        if(mTriangles[detailIdx].getColor() != color) {
            mTriangles[detailIdx].setColor(color);
            mColorChanged = true;
        }
    }

    template <class Archive>
    void save(Archive& archive) const {
        archive(mOriginal, mTriangles, mTrianglesExact);
    }

    template <class Archive>
    void load(Archive& archive) {
        archive(mOriginal, mTriangles, mTrianglesExact);
        const PeprTriangle& tri = mOriginal.getTri();
        mOriginalPlane = Plane(toExactK(tri.vertex(0)), toExactK(tri.vertex(1)), toExactK(tri.vertex(2)));
        mBounds = polygonFromTriangle(mOriginal.getTri());
        updatePolysFromTriangles();
    }

   private:
    /// Temporary storage for DataTriangles. This gets overwritten on every paint operation!
    std::vector<DataTriangle> mTriangles;

    /// Temporary storage for exact triangles. This gets overwritten on every paint operation!
    /// This is used for saving andpolygonset reconstruction
    std::vector<Triangle2> mTrianglesExact;

    std::map<size_t, PolygonSet> mColoredPolys;

    DataTriangle mOriginal;

    static const int VERTICES_PER_UNIT_CIRCLE = 100;
    static const int MIN_VERTICES_IN_CIRCLE = 24;

    /// Bounds of the original triangle
    Polygon mBounds;

    Plane mOriginalPlane;

    /// Did color of any detail triangle change since last triangulation?
    bool mColorChanged = false;

    /// Create a polygon from a PeprTriangle
    Polygon polygonFromTriangle(const PeprTriangle& tri) const;

    /// Create a polygon from 2D triangle in plane coordinates
    Polygon polygonFromTriangle(const Triangle2& tri) const;

    /// Get points of a circle that are shared with border triangles
    std::vector<std::pair<Point2, double>> getCircleSharedPoints(const Circle3& circle, const Vector3& xBase,
                                                                 const Vector3& yBase);
    // Construct a polygon from a circle.
    Polygon polygonFromCircle(const Circle3& circle);

    /// Add polygon to the detail
    void addPolygon(const Polygon& poly, size_t color);

    /// Paint a circle shape onto the plane of the triangle
    void addCircle(const Circle3& circle, size_t color);

    /// Simplify polygon, removing unnecessary vertices
    /// @return was simplified
    bool simplifyPolygon(PolygonWithHoles& poly);

    /// Find shared edge between triangles
    Segment3 findSharedEdge(const TriangleDetail& other);

    /// Find all points of polygons that are on the edge
    std::set<Point3> findPointsOnEdge(const Segment3& edge);

    /// Add points that are missing to our polygons
    /// @return true if any points were added
    bool addMissingPoints(const std::set<Point3>& myPoints, const std::set<Point3>& theirPoints,
                          const Segment3& sharedEdge);

    /// Simplify polygons, removing any unnecessary vertices
    void simplifyPolygons();

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
};

}  // namespace pepr3d

namespace CGAL {
template <typename Archive>
void save(Archive& archive, pepr3d::TriangleDetail::Triangle2 const& tri) {
    std::stringstream stream;
    stream << tri;
    archive(stream.str());
}

template <typename Archive>
void load(Archive& archive, pepr3d::TriangleDetail::Triangle2& tri) {
    std::string str;
    archive(str);
    std::stringstream stream(str);
    stream.seekg(stream.beg);
    stream >> tri;
}

}  // namespace CGAL