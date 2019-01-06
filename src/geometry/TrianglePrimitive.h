#pragma once

#include <limits>
#include <optional>
#include "geometry/Triangle.h"

namespace pepr3d {
/// Triangle ID in all detailed triangles
/// Face index in mMeshDetailed faces;
struct DetailedTriangleId {
    DetailedTriangleId() : mBaseId(std::numeric_limits<size_t>::max()), mDetailId() {}

    explicit DetailedTriangleId(size_t baseId, std::optional<size_t> detailId = {})
        : mBaseId(baseId), mDetailId(detailId) {}

    size_t getBaseId() const {
        return mBaseId;
    }

    std::optional<size_t> getDetailId() const {
        return mDetailId;
    }

   private:
    size_t mBaseId;
    std::optional<size_t> mDetailId;
};

/// Provides the conversion facilities between the custom triangle DataTriangle and the CGAL
/// Triangle_3 class. Taken from CGAL/examples/AABB_tree/custom_example.cpp, modified.
struct DataTriangleAABBPrimitive {
   public:
    using Id = std::pair<const class Geometry*, DetailedTriangleId>;

    // CGAL types returned
    using Point = DataTriangle::K::Point_3;     // CGAL 3D point type
    using Datum = DataTriangle::K::Triangle_3;  // CGAL 3D triangle type
    using Datum_reference = const DataTriangle::K::Triangle_3&;

   private:
    Id idPair;

   public:
    DataTriangleAABBPrimitive() = default;  // default constructor needed

    DataTriangleAABBPrimitive(const class Geometry* geometry, DetailedTriangleId triangleId)
        : idPair(geometry, triangleId) {}
    explicit DataTriangleAABBPrimitive(Id id) : idPair(id) {}

    // the following constructor is the one that receives the iterators from the
    // iterator range given as input to the AABB_tree
    /*explicit DataTriangleAABBPrimitive(Iterator it)
        : tri(std::move(it)) {}*/

    const Id& id() const {
        return idPair;
    }

    // on the fly conversion from the internal data to the CGAL types
    Datum_reference datum() const;

    // returns a reference point which must be on the primitive
    Point reference_point() const;
};

}  // namespace pepr3d