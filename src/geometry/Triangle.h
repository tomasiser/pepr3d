#pragma once

#include <CGAL/Algebraic_kernel_for_spheres_2_3.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Spherical_kernel_3.h>
#include <cereal/cereal.hpp>
#include <glm/glm.hpp>

namespace pepr3d {

/// Custom Triangle type holding the CGAL::Triangle_3 and additional data
class DataTriangle {
   public:
    // Use spherical kernel otherwise some CGAL operations on spheres will assert fail
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

}

namespace CGAL {
template <typename Archive>
void save(Archive& archive, pepr3d::DataTriangle::Triangle const& tri) {
    archive(cereal::make_nvp("x1", tri[0][0]), cereal::make_nvp("y1", tri[0][1]), cereal::make_nvp("z1", tri[0][2]));
    archive(cereal::make_nvp("x2", tri[1][0]), cereal::make_nvp("y2", tri[1][1]), cereal::make_nvp("z2", tri[1][2]));
    archive(cereal::make_nvp("x3", tri[2][0]), cereal::make_nvp("y3", tri[2][1]), cereal::make_nvp("z3", tri[2][2]));
}

template <typename Archive>
void load(Archive& archive, pepr3d::DataTriangle::Triangle& tri) {
    std::array<double, 3> x;
    std::array<double, 3> y;
    std::array<double, 3> z;
    archive(cereal::make_nvp("x1", x[0]), cereal::make_nvp("x2", x[1]), cereal::make_nvp("x3", x[2]));
    archive(cereal::make_nvp("y1", y[0]), cereal::make_nvp("y2", y[1]), cereal::make_nvp("y3", y[2]));
    archive(cereal::make_nvp("z1", z[0]), cereal::make_nvp("z2", z[1]), cereal::make_nvp("z3", z[2]));
    tri = pepr3d::DataTriangle::Triangle(pepr3d::DataTriangle::K::Point_3(x[0], x[1], x[2]),
                                         pepr3d::DataTriangle::K::Point_3(y[0], y[1], y[2]),
                                         pepr3d::DataTriangle::K::Point_3(z[0], z[1], z[2]));
}
}  // namespace CGAL