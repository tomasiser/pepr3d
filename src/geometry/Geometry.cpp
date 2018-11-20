#include "geometry/Geometry.h"

namespace pepr3d {

/* -------------------- Commands -------------------- */

Geometry::GeometryState Geometry::saveState() const {
    // Save only necessary data to keep snapshot size low
    return GeometryState{mTriangles, ColorManager::ColorMap(mColorManager.getColorMap())};
}

void Geometry::loadState(const GeometryState& state) {
    mTriangles = state.triangles;

    mColorManager.replaceColors(state.colorMap.begin(), state.colorMap.end());
    assert(!mColorManager.empty());

    // TODO Improve: Can we reuse something?

    // Generate new buffers from the new triangles
    generateVertexBuffer();
    generateColorBuffer();
    generateNormalBuffer();
    generateIndexBuffer();

    // Tree needs to be rebuilt to match loaded triangles
    mTree->rebuild(mTriangles.begin(), mTriangles.end());
    assert(mTree->size() == mTriangles.size());
}

/* -------------------- Mesh loading -------------------- */

void Geometry::loadNewGeometry(const std::string& fileName) {
    /// Load into mTriangles
    ModelImporter modelImporter(fileName);  // only first mesh [0]

    if(modelImporter.isModelLoaded()) {
        mTriangles = modelImporter.getTriangles();

        mPolyhedronData.vertices.clear();
        mPolyhedronData.indices.clear();
        mPolyhedronData.vertices = modelImporter.getVertexBuffer();
        mPolyhedronData.indices = modelImporter.getIndexBuffer();

        /// Generate new vertex buffer
        generateVertexBuffer();

        /// Generate new index buffer
        generateIndexBuffer();

        /// Generate new color buffer from triangle color data
        generateColorBuffer();

        /// Generate new normal buffer, copying the triangle normal to each vertex
        generateNormalBuffer();

        /// Rebuild the AABB tree
        mTree->rebuild(mTriangles.begin(), mTriangles.end());
        assert(mTree->size() == mTriangles.size());

        /// Get the new bounding box
        if(!mTree->empty()) {
            mBoundingBox = std::make_unique<BoundingBox>(mTree->bbox());
        }

        /// Build the polyhedron data structure
        buildPolyhedron();

        /// Get the generated color palette of the model, replace the current one
        mColorManager = modelImporter.getColorManager();
        assert(!mColorManager.empty());
    } else {
        CI_LOG_E("Model not loaded --> write out message for user");
    }
}

void Geometry::exportGeometry(const std::string filePath, const std::string fileName, const std::string fileType) {
    ModelExporter modelExporter(mTriangles, filePath, fileName, fileType);
}

void Geometry::generateVertexBuffer() {
    mVertexBuffer.clear();
    mVertexBuffer.reserve(3 * mTriangles.size());

    for(const auto& mTriangle : mTriangles) {
        mVertexBuffer.push_back(mTriangle.getVertex(0));
        mVertexBuffer.push_back(mTriangle.getVertex(1));
        mVertexBuffer.push_back(mTriangle.getVertex(2));
    }
}

void Geometry::generateIndexBuffer() {
    mIndexBuffer.clear();
    mIndexBuffer.reserve(mVertexBuffer.size());

    for(uint32_t i = 0; i < mVertexBuffer.size(); ++i) {
        mIndexBuffer.push_back(i);
    }
}

void Geometry::generateColorBuffer() {
    mColorBuffer.clear();
    mColorBuffer.reserve(mVertexBuffer.size());

    for(const auto& mTriangle : mTriangles) {
        const ColorIndex triColorIndex = static_cast<ColorIndex>(mTriangle.getColor());
        mColorBuffer.push_back(triColorIndex);
        mColorBuffer.push_back(triColorIndex);
        mColorBuffer.push_back(triColorIndex);
    }
    assert(mColorBuffer.size() == mVertexBuffer.size());
}

void Geometry::generateNormalBuffer() {
    mNormalBuffer.clear();
    mNormalBuffer.reserve(mVertexBuffer.size());
    for(const auto& mTriangle : mTriangles) {
        mNormalBuffer.push_back(mTriangle.getNormal());
        mNormalBuffer.push_back(mTriangle.getNormal());
        mNormalBuffer.push_back(mTriangle.getNormal());
    }
    assert(mNormalBuffer.size() == mVertexBuffer.size());
}

/* -------------------- Tool support -------------------- */

std::optional<size_t> Geometry::intersectMesh(const ci::Ray& ray) const {
    if(mTree->empty()) {
        return {};
    }

    const glm::vec3 source = ray.getOrigin();
    const glm::vec3 direction = ray.getDirection();

    const pepr3d::Geometry::Ray rayQuery(pepr3d::DataTriangle::Point(source.x, source.y, source.z),
                                         pepr3d::Geometry::Direction(direction.x, direction.y, direction.z));

    // Find the two intersection parameters - place and triangle
    Ray_intersection intersection = mTree->first_intersection(rayQuery);
    if(intersection) {
        // The intersected triangle
        if(boost::get<DataTriangleAABBPrimitive::Id>(intersection->second) != mTriangles.end()) {
            const DataTriangleAABBPrimitive::Id intersectedTriIter =
                boost::get<DataTriangleAABBPrimitive::Id>(intersection->second);
            assert(intersectedTriIter != mTriangles.end());
            const size_t retValue = intersectedTriIter - mTriangles.begin();
            assert(retValue < mTriangles.size());
            return retValue;  // convert the iterator into an index
        }
    }

    /// No intersection detected.
    return {};
}

void Geometry::setTriangleColor(const size_t triangleIndex, const size_t newColor) {
    /// Change it in the buffer
    // Color buffer has 1 ColorA for each vertex, each triangle has 3 vertices
    const size_t vertexPosition = triangleIndex * 3;

    // Change all vertices of the triangle to the same new color
    assert(vertexPosition + 2 < mColorBuffer.size());

    ColorIndex newColorIndex = static_cast<ColorIndex>(newColor);
    mColorBuffer[vertexPosition] = newColorIndex;
    mColorBuffer[vertexPosition + 1] = newColorIndex;
    mColorBuffer[vertexPosition + 2] = newColorIndex;

    /// Change it in the triangle soup
    assert(triangleIndex < mTriangles.size());
    mTriangles[triangleIndex].setColor(newColor);
}

void Geometry::buildPolyhedron() {
    mPolyhedronData.mMesh.clear();
    mPolyhedronData.mMesh.remove_property_map(mPolyhedronData.mIdMap);
    mPolyhedronData.mFaceDescs.clear();

    std::cout << "\nSTART BUILD \n mPoly vertices, indices:" << mPolyhedronData.vertices.size() << " , "
              << mPolyhedronData.indices.size() << "\n";
    std::cout << "Mesh building: \n";

    std::vector<PolyhedronData::vertex_descriptor> vertDescs;
    vertDescs.reserve(mPolyhedronData.vertices.size());
    mPolyhedronData.mMesh.reserve(mPolyhedronData.vertices.size(), mPolyhedronData.indices.size() * 3 / 2,
                                  mPolyhedronData.indices.size());

    for(const auto& vertex : mPolyhedronData.vertices) {
        PolyhedronData::vertex_descriptor v =
            mPolyhedronData.mMesh.add_vertex(DataTriangle::Point(vertex.x, vertex.y, vertex.z));
        vertDescs.push_back(v);
    }

    mPolyhedronData.mFaceDescs.reserve(mPolyhedronData.indices.size());
    for(const auto& tri : mPolyhedronData.indices) {
        auto f = mPolyhedronData.mMesh.add_face(vertDescs[tri[0]], vertDescs[tri[1]], vertDescs[tri[2]]);
        assert(f != PolyhedronData::Mesh::null_face());
        mPolyhedronData.mFaceDescs.push_back(f);
    }

    bool created;
    boost::tie(mPolyhedronData.mIdMap, created) =
        mPolyhedronData.mMesh.add_property_map<PolyhedronData::face_descriptor, size_t>(
            "f:idOfEachTriangle", std::numeric_limits<size_t>::max() - 1);
    assert(created);

    // Add the IDs
    size_t i = 0;
    for(const PolyhedronData::face_descriptor& face : mPolyhedronData.mFaceDescs) {
        /*std::cout << i << " ";
        if (i == 49) {
            std::cout << "danger\n";
        }*/
        mPolyhedronData.mIdMap[face] = i;
        ++i;
    }

    std::cout << "Mesh is valid : " << mPolyhedronData.mMesh.is_valid()
              << ", vertices: " << mPolyhedronData.mMesh.number_of_vertices()
              << ", faces: " << mPolyhedronData.mMesh.number_of_faces() << "\n";

    std::cout << "\nPolyhedron building: \n";
    PolyhedronBuilder<HalfedgeDS> triangle(mPolyhedronData.indices, mPolyhedronData.vertices);
    mPolyhedronData.P.clear();
    try {
        mPolyhedronData.P.delegate(triangle);
        mPolyhedronData.faceHandles = triangle.getFacetArray();
    } catch(CGAL::Assertion_exception* assertExcept) {
        mPolyhedronData.P.clear();
        CI_LOG_E("Polyhedron not loaded. " + assertExcept->message());
        return;
    }

    // The exception does not get thrown in Release
    if(!mPolyhedronData.P.is_valid() || mPolyhedronData.P.is_empty()) {
        mPolyhedronData.P.clear();
        CI_LOG_E("Polyhedron loaded empty or invalid.");
        return;
    }

    assert(mPolyhedronData.P.size_of_facets() == mPolyhedronData.indices.size());
    assert(mPolyhedronData.P.size_of_vertices() == mPolyhedronData.vertices.size());

    // Use the facetsCreated from the incremental builder, set the ids linearly
    for(size_t facetId = 0; facetId < mPolyhedronData.faceHandles.size(); ++facetId) {
        mPolyhedronData.faceHandles[facetId]->id() = facetId;
    }

    mPolyhedronData.closeCheck = mPolyhedronData.P.is_closed();
    std::cout << "Polyhedron built OK. " << mPolyhedronData.P.size_of_vertices() << " vertices, "
              << mPolyhedronData.P.size_of_facets() << " facets."
              << "\n";
}

std::array<int, 3> Geometry::gatherNeighboursSurface(const size_t triIndex) const {
    const auto& faceDescriptors = mPolyhedronData.mFaceDescs;
    const auto& mesh = mPolyhedronData.mMesh;
    assert(triIndex < faceDescriptors.size());
    std::array<int, 3> returnValue = {-1, -1, -1};

    const PolyhedronData::face_descriptor face = faceDescriptors[triIndex];
    const auto edge = mesh.halfedge(face);
    auto itEdge = edge;

    for(int i = 0; i < 3; ++i) {
        auto oppositeEdge = mesh.opposite(itEdge);
        if(oppositeEdge.is_valid()) {
            const PolyhedronData::Mesh::Face_index neighbourFace = mesh.face(oppositeEdge);
            const size_t neighbourFaceId = mPolyhedronData.mIdMap[neighbourFace];
            assert(neighbourFaceId < mTriangles.size());
            returnValue[i] = neighbourFaceId;
        }

        itEdge = mesh.next(itEdge);
    }
    assert(edge == itEdge);

    return returnValue;
}

std::array<int, 3> Geometry::gatherNeighbours(
    const size_t triIndex,
    const std::vector<CGAL::Polyhedron_incremental_builder_3<HalfedgeDS>::Face_handle>& faceHandles) const {
    assert(triIndex < faceHandles.size());
    const Polyhedron::Facet_iterator& facet = faceHandles[triIndex];
    std::array<int, 3> returnValue = {-1, -1, -1};
    assert(facet->is_triangle());

    const auto edgeIteratorStart = facet->facet_begin();
    auto edgeIter = edgeIteratorStart;

    for(int i = 0; i < 3; ++i) {
        const auto eFace = edgeIter->facet();
        if(edgeIter->opposite()->facet() != nullptr) {
            const size_t triId = edgeIter->opposite()->facet()->id();
            // Asserting in int, because int is the return value
            assert(static_cast<int>(triId) < static_cast<int>(mTriangles.size()));
            returnValue[i] = static_cast<int>(triId);
        }
        ++edgeIter;
    }
    assert(edgeIter == edgeIteratorStart);

    auto meshRet = gatherNeighboursSurface(triIndex);

    for(int i = 0; i < 3; ++i) {
        // std::cout << " poly> " << returnValue[i] << " - " << meshRet[i] << " <mesh\n";
        assert(returnValue[i] == meshRet[i]);
    }

    return returnValue;
}

void Geometry::computeSdf() {
    mPolyhedronData.sdfComputed = false;
    mPolyhedronData.mMesh.remove_property_map(mPolyhedronData.sdf_property_map);
    bool created;
    boost::tie(mPolyhedronData.sdf_property_map, created) =
        mPolyhedronData.mMesh.add_property_map<PolyhedronData::face_descriptor, double>("f:sdf");
    assert(created);

    CGAL::sdf_values(mPolyhedronData.mMesh, mPolyhedronData.sdf_property_map, 2.0 / 3.0 * CGAL_PI, 25, true);
    mPolyhedronData.sdfComputed = true;
    CI_LOG_I("SDF values computed.");
}

size_t Geometry::segment(const int numberOfClusters, const float smoothingLambda,
                         std::unordered_map<size_t, std::vector<size_t>>& segmentToTriangleIds) {
    if(!mPolyhedronData.sdfComputed) {
        return 0;
    }
    bool created;
    PolyhedronData::Mesh::Property_map<PolyhedronData::face_descriptor, std::size_t> segment_property_map;
    boost::tie(segment_property_map, created) =
        mPolyhedronData.mMesh.add_property_map<PolyhedronData::face_descriptor, std::size_t>("f:sid");
    assert(created);

    std::size_t number_of_segments =
        CGAL::segmentation_from_sdf_values(mPolyhedronData.mMesh, mPolyhedronData.sdf_property_map,
                                           segment_property_map, numberOfClusters, smoothingLambda);

    // Fill up the palette to the new number of colors
    if(number_of_segments > PEPR3D_MAX_PALETTE_COLORS) {
        mPolyhedronData.mMesh.remove_property_map(segment_property_map);
        return 0;
    }
    glm::vec4 segmentColors[4] = {glm::vec4(0.1, 1, 0.1, 1), glm::vec4(1, 0, 1, 1), glm::vec4(0.4, 0.4, 0.4, 1),
                                  glm::vec4(0, 1, 1, 1)};
    const int colorsToAdd = static_cast<int>(number_of_segments) - static_cast<int>(mColorManager.size());
    for(int i = 0; i < colorsToAdd; ++i) {
        mColorManager.addColor(segmentColors[i]);
        CI_LOG_I("Added a color");
    }
    assert(mColorManager.size() >= number_of_segments);

    for(size_t seg = 0; seg < number_of_segments; ++seg) {
        segmentToTriangleIds.insert({seg, {}});
    }

    // Assign the colors to the triangles
    for(const auto& face : mPolyhedronData.mFaceDescs) {
        const size_t id = mPolyhedronData.mIdMap[face];
        const size_t color = segment_property_map[face];
        assert(id < mTriangles.size());
        assert(color < mColorManager.size());

        segmentToTriangleIds[color].push_back(id);
        // setTriangleColor(id, color);
    }

    CI_LOG_I("Segmentation finished. Number of segments: " + std::to_string(number_of_segments));

    // End, clean up
    mPolyhedronData.mMesh.remove_property_map(segment_property_map);

    return number_of_segments;
}

}  // namespace pepr3d
