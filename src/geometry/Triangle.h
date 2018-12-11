#pragma once

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Spherical_kernel_3.h>
#include <cinder/Color.h>

#include <CGAL/Algebraic_kernel_for_spheres_2_3.h>
#include <vector>

namespace pepr3d {

/// Custom Triangle type holding the CGAL::Triangle_3 and additional data
class DataTriangle {
   public:
    // using K = CGAL::Spherical_kernel_3<CGAL::Simple_cartesian<double>,
    // CGAL::Algebraic_kernel_for_spheres_2_3<double>>;
    using K = CGAL::Simple_cartesian<double>;
    using Point = K::Point_3;
    using Triangle = K::Triangle_3;

   private:
    /// Geometry data in CGAL format to allow for usage in AABB tree
    Triangle mTriangleCgal;

    /// Color of the triangle in the final STL file
    size_t mColor;

    /// Normal of the triangle
    glm::vec3 mNormal;

   public:
    DataTriangle() : mColor(0) {}

    DataTriangle(const glm::vec3 x, const glm::vec3 y, const glm::vec3 z, const glm::vec3 n, const size_t col = 0)
        : mTriangleCgal(Point(x.x, x.y, x.z), Point(y.x, y.y, y.z), Point(z.x, z.y, z.z)), mColor(col), mNormal(n) {}

    /// Method used by the AABB conversion functor to make DataTriangle searchable in AABB tree
    const Triangle &getTri() const {
        return mTriangleCgal;
    }

    glm::vec3 getVertex(const size_t i) const {
        auto v = mTriangleCgal.vertex(static_cast<int>(i));
        return glm::vec3(v.x(), v.y(), v.z());
    }

    void setColor(const size_t newColor) {
        mColor = newColor;
    }

    size_t getColor() const {
        return mColor;
    }

    glm::vec3 getNormal() const {
        return mNormal;
    }
};

using Iterator = std::vector<DataTriangle>::const_iterator;

/// Provides the conversion facilities between the custom triangle DataTriangle and the CGAL
/// Triangle_3 class. Taken from CGAL/examples/AABB_tree/custom_example.cpp, modified.
struct DataTriangleAABBPrimitive {
   public:
    // this is the type of data that the queries returns
    using Id = std::vector<DataTriangle>::const_iterator;

    // CGAL types returned
    using Point = pepr3d::DataTriangle::K::Point_3;     // CGAL 3D point type
    using Datum = pepr3d::DataTriangle::K::Triangle_3;  // CGAL 3D triangle type
    using Datum_reference = const pepr3d::DataTriangle::K::Triangle_3 &;

   private:
    Id tri;  // this is what the AABB tree stores internally

   public:
    DataTriangleAABBPrimitive() = default;  // default constructor needed

    // the following constructor is the one that receives the iterators from the
    // iterator range given as input to the AABB_tree
    explicit DataTriangleAABBPrimitive(Iterator it) : tri(std::move(it)) {}

    const Id &id() const {
        return tri;
    }

    // on the fly conversion from the internal data to the CGAL types
    Datum_reference datum() const {
        return tri->getTri();
    }

    // returns a reference point which must be on the primitive
    Point reference_point() const {
        return tri->getTri().vertex(0);
    }
};
}  // namespace pepr3d