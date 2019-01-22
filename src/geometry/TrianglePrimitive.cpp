#include "geometry/TrianglePrimitive.h"
#include "geometry/Geometry.h"

namespace pepr3d {
DataTriangleAABBPrimitive::Datum_reference DataTriangleAABBPrimitive::datum() const {
    const Geometry* geometry = idPair.first;
    return geometry->getTriangle(idPair.second).getTri();
}

DataTriangleAABBPrimitive::Point DataTriangleAABBPrimitive::reference_point() const {
    return datum().vertex(0);
}
}  // namespace pepr3d
