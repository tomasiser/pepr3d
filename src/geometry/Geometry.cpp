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

        /// Build the polyhedron data structure
        buildPolyhedron();

        /// Get the generated color palette of the model, replace the current one
        mColorManager = modelImporter.getColorManager();
        assert(!mColorManager.empty());
    } else {
        CI_LOG_E("Model not loaded --> write out message for user");
    }
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
    PolyhedronBuilder<HalfedgeDS> triangle(mPolyhedronData.indices, mPolyhedronData.vertices);
    mPolyhedronData.P.clear();
    try {
        mPolyhedronData.P.delegate(triangle);
        mPolyhedronData.faceHandles = triangle.getFacetArray();
    } catch(CGAL::Assertion_exception assertExcept) {
        mPolyhedronData.P.clear();
        CI_LOG_E("Polyhedron not loaded. " + assertExcept.message());
        return;
    }

    assert(mPolyhedronData.P.size_of_facets() == mPolyhedronData.indices.size());
    assert(mPolyhedronData.P.size_of_vertices() == mPolyhedronData.vertices.size());

    // Use the facetsCreated from the incremental builder, set the ids linearly
    for(int facetId = 0; facetId < mPolyhedronData.faceHandles.size(); ++facetId) {
        mPolyhedronData.faceHandles[facetId]->id() = facetId;
    }

    mPolyhedronData.closeCheck = mPolyhedronData.P.is_closed();
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
            assert(static_cast<int>(triId) < mTriangles.size());
            returnValue[i] = static_cast<int>(triId);
        }
        ++edgeIter;
    }
    assert(edgeIter == edgeIteratorStart);

    return returnValue;
}

}  // namespace pepr3d
