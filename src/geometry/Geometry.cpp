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

void Geometry::recomputeFromData(::ThreadPool& threadPool) {
    // We already loaded the model
    assert(mProgress->importRenderPercentage == 1.0f);
    assert(mProgress->importComputePercentage == 1.0f);

    /// Async build the polyhedron data structure
    auto buildPolyhedronFuture = threadPool.enqueue([this]() {
        assert(!mPolyhedronData.vertices.empty());
        assert(!mPolyhedronData.indices.empty());
        buildPolyhedron();
    });

    /// Async build the AABB tree
    auto buildTreeFuture = threadPool.enqueue([this]() {
        mProgress->aabbTreePercentage = 0.0f;

        mTree->rebuild(mTriangles.cbegin(), mTriangles.cend());
        assert(mTree->size() == mTriangles.size());

        /// Get the new bounding box
        if(!mTree->empty()) {
            mBoundingBox = std::make_unique<BoundingBox>(mTree->bbox());
        }

        mProgress->aabbTreePercentage = 1.0f;
    });

    mProgress->buffersPercentage = 0.0f;

    /// Generate new vertex buffer
    generateVertexBuffer();
    mProgress->buffersPercentage = 0.25f;

    /// Generate new index buffer
    generateIndexBuffer();
    mProgress->buffersPercentage = 0.50f;

    /// Generate new color buffer from triangle color data
    generateColorBuffer();
    mProgress->buffersPercentage = 0.75f;

    /// Generate new normal buffer, copying the triangle normal to each vertex
    generateNormalBuffer();
    mProgress->buffersPercentage = 1.0f;

    /// Wait for building the polyhedron and tree
    buildTreeFuture.get();
    buildPolyhedronFuture.get();
}

void Geometry::loadNewGeometry(const std::string& fileName, ::ThreadPool& threadPool) {
    // Reset progress
    mProgress->resetLoad();

    /// Import the object via Assimp
    ModelImporter modelImporter(fileName, mProgress.get(), threadPool);  // only first mesh [0]

    if(modelImporter.isModelLoaded()) {
        /// Fill triangle data to compute AABB
        mTriangles = modelImporter.getTriangles();

        /// Fill Polyhedron data to compute SurfaceMesh
        mPolyhedronData.vertices.clear();
        mPolyhedronData.indices.clear();
        mPolyhedronData.vertices = modelImporter.getVertexBuffer();
        mPolyhedronData.indices = modelImporter.getIndexBuffer();

        /// Get the generated color palette of the model, replace the current one
        mColorManager = modelImporter.getColorManager();
        assert(!mColorManager.empty());

        /// Do the computations in parallel
        recomputeFromData(threadPool);
    } else {
        throw std::runtime_error("Model loading failed.");
        CI_LOG_E("Model not loaded --> write out message for user");
    }
}

void Geometry::exportGeometry(const std::string filePath, const std::string fileName, const std::string fileType) {
    // Reset progress
    mProgress->resetSave();

    ModelExporter modelExporter(mTriangles, filePath, fileName, fileType, mProgress.get());
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
    mProgress->polyhedronPercentage = 0.0f;
    mPolyhedronData.mMesh.clear();
    mPolyhedronData.mMesh.remove_property_map(mPolyhedronData.mIdMap);
    mPolyhedronData.mMesh.remove_property_map(mPolyhedronData.sdf_property_map);
    mPolyhedronData.isSdfComputed = false;
    mPolyhedronData.valid = false;
    mPolyhedronData.mFaceDescs.clear();

    std::vector<PolyhedronData::vertex_descriptor> vertDescs;
    vertDescs.reserve(mPolyhedronData.vertices.size());
    mPolyhedronData.mMesh.reserve(static_cast<PolyhedronData::Mesh::size_type>(mPolyhedronData.vertices.size()),
                                  static_cast<PolyhedronData::Mesh::size_type>(mPolyhedronData.indices.size() * 3 / 2),
                                  static_cast<PolyhedronData::Mesh::size_type>(mPolyhedronData.indices.size()));

    for(const auto& vertex : mPolyhedronData.vertices) {
        PolyhedronData::vertex_descriptor v =
            mPolyhedronData.mMesh.add_vertex(DataTriangle::Point(vertex.x, vertex.y, vertex.z));
        vertDescs.push_back(v);
    }

    mPolyhedronData.mFaceDescs.reserve(mPolyhedronData.indices.size());
    bool meshHasNull = false;
    for(const auto& tri : mPolyhedronData.indices) {
        auto f = mPolyhedronData.mMesh.add_face(vertDescs[tri[0]], vertDescs[tri[1]], vertDescs[tri[2]]);
        if(f == PolyhedronData::Mesh::null_face()) {
            // Adding a non-valid face, the model is wrong and we stop.
            meshHasNull = true;
            break;
        } else {
            assert(f != PolyhedronData::Mesh::null_face());
            mPolyhedronData.mFaceDescs.push_back(f);
        }
    }
    // If the build failed, clear, set invalid and exit;
    if(meshHasNull) {
        mPolyhedronData.mMesh.clear();
        mPolyhedronData.valid = false;
        mProgress->polyhedronPercentage = -1.0f;
        return;
    }

    bool created;
    boost::tie(mPolyhedronData.mIdMap, created) =
        mPolyhedronData.mMesh.add_property_map<PolyhedronData::face_descriptor, size_t>(
            "f:idOfEachTriangle", std::numeric_limits<size_t>::max() - 1);
    assert(created);

    // Add the IDs
    size_t i = 0;
    for(const PolyhedronData::face_descriptor& face : mPolyhedronData.mFaceDescs) {
        mPolyhedronData.mIdMap[face] = i;
        ++i;
    }
    CI_LOG_I("Polyhedral mesh built, vertices: " + std::to_string(mPolyhedronData.vertices.size()) +
             ", faces: " + std::to_string(mPolyhedronData.indices.size()));
    mPolyhedronData.valid = true;
    mProgress->polyhedronPercentage = 1.0f;
}

std::array<int, 3> Geometry::gatherNeighbours(const size_t triIndex) const {
    const auto& faceDescriptors = mPolyhedronData.mFaceDescs;
    const auto& mesh = mPolyhedronData.mMesh;
    assert(triIndex < faceDescriptors.size());
    std::array<int, 3> returnValue = {-1, -1, -1};
    const PolyhedronData::face_descriptor face = faceDescriptors[triIndex];
    const auto edge = mesh.halfedge(face);
    auto itEdge = edge;

    for(int i = 0; i < 3; ++i) {
        const auto oppositeEdge = mesh.opposite(itEdge);
        if(oppositeEdge.is_valid() && !mesh.is_border(oppositeEdge)) {
            const PolyhedronData::Mesh::Face_index neighbourFace = mesh.face(oppositeEdge);
            const size_t neighbourFaceId = mPolyhedronData.mIdMap[neighbourFace];
            assert(neighbourFaceId < mTriangles.size());
            returnValue[i] = static_cast<int>(neighbourFaceId);
        }

        itEdge = mesh.next(itEdge);
    }
    assert(edge == itEdge);

    return returnValue;
}

void Geometry::computeSdf() {
    mPolyhedronData.isSdfComputed = false;
    mPolyhedronData.mMesh.remove_property_map(mPolyhedronData.sdf_property_map);
    bool created;
    boost::tie(mPolyhedronData.sdf_property_map, created) =
        mPolyhedronData.mMesh.add_property_map<PolyhedronData::face_descriptor, double>("f:sdf");
    assert(created);

    if(created) {
        try {
            CGAL::sdf_values(mPolyhedronData.mMesh, mPolyhedronData.sdf_property_map, 2.0 / 3.0 * CGAL_PI, 25, true);
        } catch(...) {
            throw std::runtime_error("Computation of the SDF values failed internally in CGAL.");
        }
        mPolyhedronData.isSdfComputed = true;
        CI_LOG_I("SDF values computed.");
    } else {
        mPolyhedronData.isSdfComputed = false;
        throw std::runtime_error("Computation of the SDF values failed, a new property map could not be tied");
    }
}

size_t Geometry::segment(const int numberOfClusters, const float smoothingLambda,
                         std::map<size_t, std::vector<size_t>>& segmentToTriangleIds,
                         std::unordered_map<size_t, size_t>& triangleToSegmentMap) {
    if(!mPolyhedronData.isSdfComputed) {
        throw std::runtime_error("Cannot calculate the segmentation - SDF values not computed.");
        return 0;
    }
    bool created;
    PolyhedronData::Mesh::Property_map<PolyhedronData::face_descriptor, std::size_t> segment_property_map;
    boost::tie(segment_property_map, created) =
        mPolyhedronData.mMesh.add_property_map<PolyhedronData::face_descriptor, std::size_t>("f:sid");

    assert(created);

    if(created) {
        std::size_t numberOfSegments = std::numeric_limits<size_t>::min();
        try {
            numberOfSegments =
                CGAL::segmentation_from_sdf_values(mPolyhedronData.mMesh, mPolyhedronData.sdf_property_map,
                                                   segment_property_map, numberOfClusters, smoothingLambda);
        } catch(...) {
            throw std::runtime_error("Computation of the segmentation failed internally in CGAL.");
        }

        if(numberOfSegments > PEPR3D_MAX_PALETTE_COLORS) {
            mPolyhedronData.mMesh.remove_property_map(segment_property_map);
            return 0;
        }

        for(size_t seg = 0; seg < numberOfSegments; ++seg) {
            segmentToTriangleIds.insert({seg, {}});
        }

        // Assign the colors to the triangles
        for(const auto& face : mPolyhedronData.mFaceDescs) {
            const size_t id = mPolyhedronData.mIdMap[face];
            const size_t color = segment_property_map[face];
            assert(id < mTriangles.size());
            assert(color < numberOfSegments);
            triangleToSegmentMap.insert({id, color});
            segmentToTriangleIds[color].push_back(id);
        }

        CI_LOG_I("Segmentation finished. Number of segments: " + std::to_string(numberOfSegments));

        // End, clean up
        mPolyhedronData.mMesh.remove_property_map(segment_property_map);

        return numberOfSegments;
    } else {
        throw std::runtime_error("Computation of the segmentation failed, a new property map could not be tied.");
    }
}

}  // namespace pepr3d
