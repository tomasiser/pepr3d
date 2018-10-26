#pragma once

// #include <CGAL/Simple_cartesian.h>
#include <cinder/Color.h>
#include <vector>

namespace pepr3d {

struct DataTriangle  // Will be changed for DataTriangle from Triangle.h after CGAL is usable.
{
    glm::vec3 vertices[3];
    glm::vec3 normal;
    cinder::ColorA color;

    void setColor(const cinder::ColorA col) {
        color = col;
    }

    DataTriangle(const glm::vec3 x, const glm::vec3 y, const glm::vec3 z, const glm::vec3 n, const cinder::ColorA col) {
        vertices[0] = x;
        vertices[1] = y;
        vertices[2] = z;
        normal = n;
        color = col;
    }
};

// using K = CGAL::Simple_cartesian<double>;
// using Point = K::Point_3;
// using Triangle = K::Triangle_3;
//
///// Custom Triangle type holding the CGAL::Triangle_3 and additional data
// class DataTriangle {
//    /// Geometry data in CGAL format to allow for usage in AABB tree
//    Triangle mTriangleCgal;
//
//    /// Color of the triangle in the final STL file
//    int mColor;
//
//   public:
//    DataTriangle() : mColor(0) {}
//
//    DataTriangle(const Point a, const Point b, const Point c, const int col = 0)
//        : mTriangleCgal(a, b, c), mColor(col) {}
//
//    /// Method used by the AABB conversion functor to make DataTriangle searchable in AABB tree
//    const Triangle &getTri() const {
//        return mTriangleCgal;
//    }
//
//    void setColor(const int newColor) {
//        mColor = newColor;
//    }
//};

// using Iterator = std::vector<DataTriangle>::const_iterator;
//
///// Provides the conversion facilities between the custom triangle DataTriangle and the CGAL
///// Triangle_3 class. Taken from CGAL/examples/AABB_tree/custom_example.cpp, modified.
// struct DataTriangleAABBPrimitive {
//   public:
//    // this is the type of data that the queries returns
//    using Id = std::vector<DataTriangle>::const_iterator;
//
//    // CGAL types returned
//    using Point = K::Point_3;     // CGAL 3D point type
//    using Datum = K::Triangle_3;  // CGAL 3D triangle type
//    using Datum_reference = const K::Triangle_3 &;
//
//   private:
//    Id tri;  // this is what the AABB tree stores internally
//
//   public:
//    DataTriangleAABBPrimitive() = default;  // default constructor needed
//
//    // the following constructor is the one that receives the iterators from the
//    // iterator range given as input to the AABB_tree
//    DataTriangleAABBPrimitive(Iterator it) : tri(std::move(it)) {}
//
//    const Id &id() const {
//        return tri;
//    }
//
//    // on the fly conversion from the internal data to the CGAL types
//    Datum_reference datum() const {
//        return tri->getTri();
//    }
//
//    // returns a reference point which must be on the primitive
//    Point reference_point() const {
//        return tri->getTri().vertex(0);
//    }
//};

}  // namespace pepr3d