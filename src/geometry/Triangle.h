#pragma once

#include <CGAL/Algebraic_kernel_for_spheres_2_3.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Spherical_kernel_3.h>
#include <cereal/access.hpp>
#include <cereal/cereal.hpp>
#include <glm/glm.hpp>

namespace pepr3d {

/// Custom Triangle type holding the CGAL::Triangle_3 and additional data
class DataTriangle {
   public:
    // Use spherical kernel otherwise some CGAL operations on spheres will assert fail
    // IMPORTANT:
    // Even though this kernel is based on Simple_cartesian, it still uses ref counting
    // Therefore even making copies of CGAL objects is not safe in multithreaded environment!
    using K = CGAL::Spherical_kernel_3<CGAL::Simple_cartesian<double>, CGAL::Algebraic_kernel_for_spheres_2_3<double>>;
    using Point = K::Point_3;
    using Triangle = K::Triangle_3;

   private:
    /// Geometry data in CGAL format to allow for usage in AABB tree
    Triangle mTriangleCgal;

    /// Color of the triangle in the final STL file
    size_t mColor;

    /// Normal of the triangle
    glm::vec3 mNormal;

    friend class cereal::access;

   public:
    DataTriangle() : mColor(0) {}

    DataTriangle(const glm::vec3 x, const glm::vec3 y, const glm::vec3 z, const glm::vec3 n, const size_t col = 0)
        : mTriangleCgal(Point(x.x, x.y, x.z), Point(y.x, y.y, y.z), Point(z.x, z.y, z.z)), mColor(col), mNormal(n) {}

    /// Method used by the AABB conversion functor to make DataTriangle searchable in AABB tree
    const Triangle& getTri() const {
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

   private:
    template <class Archive>
    void serialize(Archive& ar) {
        ar(mTriangleCgal, mColor, mNormal);
    }
};

}  // namespace pepr3d

namespace CGAL {

template <typename Archive>
void save(Archive& archive, const pepr3d::DataTriangle::Point& point) {
    archive(cereal::make_nvp("x", point[0]), cereal::make_nvp("y", point[1]), cereal::make_nvp("z", point[2]));
}

template <typename Archive>
void load(Archive& archive, pepr3d::DataTriangle::Point& point) {
    // Important!
    // Keep the same order for points, even if you use NVP. (Only JSON archives support out of order loading)
    std::array<pepr3d::DataTriangle::Point::FT, 3> values;
    archive(cereal::make_nvp("x", values[0]), cereal::make_nvp("y", values[1]), cereal::make_nvp("z", values[2]));
    point = pepr3d::DataTriangle::Point(values[0], values[1], values[2]);
}

template <typename Archive>
void save(Archive& archive, const pepr3d::DataTriangle::Triangle& tri) {
    archive(tri[0], tri[1], tri[2]);
}

template <typename Archive>
void load(Archive& archive, pepr3d::DataTriangle::Triangle& tri) {
    // Important!
    // Keep the same order for points, even if you use NVP. (Only JSON archives support out of order loading)
    std::array<pepr3d::DataTriangle::Point, 3> points;
    archive(points[0], points[1], points[2]);
    tri = pepr3d::DataTriangle::Triangle(points[0], points[1], points[2]);
}
}  // namespace CGAL