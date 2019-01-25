#pragma once

#include <CGAL/Surface_mesh.h>
#include "geometry/Triangle.h"

namespace pepr3d {

struct PolyhedronData {
    /// Vertex positions after joining all identical vertices.
    /// This is after removing all other componens and as such based only on the position property.
    std::vector<glm::vec3> vertices;

    /// Indices to triangles of the polyhedron. Indices are stored in a CCW order, as imported from Assimp.
    std::vector<std::array<size_t, 3>> indices;

    using Mesh = CGAL::Surface_mesh<DataTriangle::K::Point_3>;
    using vertex_descriptor = Mesh::Vertex_index;
    using face_descriptor = Mesh::Face_index;
    using edge_descriptor = Mesh::Edge_index;
    using halfedge_descriptor = Mesh::Halfedge_index;

    bool isSdfComputed = false;
    bool valid = false;
    bool sdfValuesValid = true;

    /// Map converting a face_descriptor into an ID, that corresponds to the mTriangles vector
    PolyhedronData::Mesh::Property_map<PolyhedronData::face_descriptor, size_t> mIdMap;

    /// Map converting a face_descriptor into the SDF value of the given face.
    /// Note: the SDF values are linearly normalized so min=0, max=1
    PolyhedronData::Mesh::Property_map<PolyhedronData::face_descriptor, double> sdf_property_map;

    /// A "map" converting the ID of each triangle (from mTriangles) into a face_descriptor
    std::vector<PolyhedronData::face_descriptor> mFaceDescs;

    /// The data-structure itself
    Mesh mMesh;
};

}  // namespace pepr3d