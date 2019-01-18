#pragma once

#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/exceptions.h>
#include <CGAL/mesh_segmentation.h>
#include <cinder/Ray.h>
#include <cinder/gl/gl.h>
#include <cereal/types/array.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/vector.hpp>
#include "cinder/Log.h"

#include <cassert>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

#include "geometry/ColorManager.h"
#include "geometry/GeometryProgress.h"
#include "geometry/GlmSerialization.h"
#include "geometry/ModelImporter.h"
#include "geometry/Triangle.h"
#include "geometry/TriangleDetail.h"
#include "geometry/TrianglePrimitive.h"
#include "tools/Brush.h"

namespace pepr3d {
class Geometry {
   public:
    using Direction = pepr3d::DataTriangle::K::Direction_3;
    using Ft = pepr3d::DataTriangle::K::FT;
    using Ray = pepr3d::DataTriangle::K::Ray_3;
    using My_AABB_traits = CGAL::AABB_traits<pepr3d::DataTriangle::K, DataTriangleAABBPrimitive>;
    using Tree = CGAL::AABB_tree<My_AABB_traits>;
    using Ray_intersection = boost::optional<Tree::Intersection_and_primitive_id<Ray>::Type>;
    using BoundingBox = My_AABB_traits::Bounding_box;
    using ColorIndex = GLuint;

    struct AreaHighlight {
        // all the triangles in highlight
        std::set<size_t> triangles;
        BrushSettings settings;
        ci::Ray ray;
        glm::vec3 origin{};
        glm::vec3 direction{};
        double size{};
        bool enabled{};
        /// Do we need to upload new data to the gpu
        bool dirty{true};
    };

    struct OpenGlData {
        /// Vertex buffer with the same data as mTriangles for OpenGL to render the mesh.
        /// Contains position and color data for each vertex.
        std::vector<glm::vec3> vertexBuffer;

        /// Color buffer, keeping the invariant that every triangle has only one color - all three vertices have to have
        /// the same color. It is aligned with the vertex buffer and its size should be always equal to the vertex
        /// buffer.
        std::vector<ColorIndex> colorBuffer;

        /// Normal buffer, the triangle has same normal for its every vertex.
        /// It is aligned with the vertex buffer and its size should be always equal to the vertex buffer.
        std::vector<glm::vec3> normalBuffer;

        /// Index buffer for OpenGL frontend., specifying the same triangles as in mTriangles.
        std::vector<uint32_t> indexBuffer;

        /// boolen for each triangle that indicates if the triangle should display cursor highlight
        /// Used to limit the highlight to continuous surface
        std::vector<GLint> highlightMask;  // Possibly needs to be GLint, had problems getting GLbyte through cinder

        bool isDirty{true};

        /// Always editable struct that keeps track of changes since last frame
        /// Updates to color/highlight buffer set this flag to true
        /// ModelView resets the flag to false when it updates OpenGl data
        struct Info {
            mutable bool didColorUpdate{false};
            mutable bool didHighlightUpdate{false};

            void unsetColorFlag() const {
                didColorUpdate = false;
            }

            void unsetHighlightFlag() const {
                didHighlightUpdate = false;
            }
        } info;
    };

   private:
    /// Triangle soup of the original model mesh, containing CGAL::Triangle_3 data for AABB tree.
    std::vector<DataTriangle> mTriangles;

    /// Map of triangle details. (Detailed triangles that replace the original)
    std::map<size_t, TriangleDetail> mTriangleDetails;

    /// Map of baseTriangleId -> Index of first detail triangle color in mOgl.colorBuffer
    std::map<size_t, size_t> mTriangleDetailColorBufferStart;

    /// All open GL buffers
    OpenGlData mOgl;

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
    } mPolyhedronData;

    /// AABB tree from the CGAL library, to find intersections with rays generated by user mouse clicks and the mesh.
    std::unique_ptr<Tree> mTree;

    /// AABB tree built over all triangles, including details. This tree is invalidated on every operation that changes
    /// triangleDetail topology.
    std::unique_ptr<Tree> mTreeDetailed;

    // ----- Detailed Mesh Data ------

    /// Surface mesh with detail triangles included
    std::unique_ptr<PolyhedronData::Mesh> mMeshDetailed;

    std::unordered_map<DetailedTriangleId, PolyhedronData::face_descriptor> mMeshDetailedFaceDescs;

    /// Map converting a face_descriptor into an ID
    PolyhedronData::Mesh::Property_map<PolyhedronData::face_descriptor, DetailedTriangleId> mMeshDetailedIdMap;

    // ----- END of Detailed Mesh Data ------

    /// AABB of the whole mesh
    std::unique_ptr<BoundingBox> mBoundingBox;

    /// A vector based map mapping size_t into ci::ColorA
    ColorManager mColorManager;

    /// Struct representing a highlight around user's cursor
    AreaHighlight mAreaHighlight;

    /// Current progress of import, tree, polyhedron building, export, etc.
    std::unique_ptr<GeometryProgress> mProgress;

    struct GeometryState {
        std::vector<DataTriangle> triangles;
        std::map<size_t, TriangleDetail> triangleDetails;
        ColorManager::ColorMap colorMap;
    };

    friend class cereal::access;

   public:
    /// Empty constructor
    Geometry() : mTree(std::make_unique<Tree>()), mProgress(std::make_unique<GeometryProgress>()) {}

    Geometry(std::vector<DataTriangle>&& triangles)
        : mTriangles(std::move(triangles)), mProgress(std::make_unique<GeometryProgress>()) {
        generateVertexBuffer();
        generateIndexBuffer();
        generateColorBuffer();
        generateNormalBuffer();
        assert(mOgl.indexBuffer.size() == mOgl.vertexBuffer.size());
        buildTree();
        buildDetailedTree();

        assert(mTree->size() == mTriangles.size());
        if(!mTree->empty()) {
            mBoundingBox = std::make_unique<BoundingBox>(mTree->bbox());
        }
    }

    std::vector<glm::vec3>& getVertexBuffer() {
        return mOgl.vertexBuffer;
    }

    bool polyhedronValid() const {
        return mPolyhedronData.mMesh.is_valid() && mPolyhedronData.valid && !mPolyhedronData.mMesh.is_empty();
    }

    size_t polyVertCount() const {
        return mPolyhedronData.vertices.size();
    }

    const OpenGlData& getOpenGlData() const {
        // Since we use a new vertex for each triangle, we should have vertices == triangles
        assert(mOgl.indexBuffer.size() == mOgl.vertexBuffer.size());
        assert(mOgl.vertexBuffer.size() == mOgl.normalBuffer.size());
        assert(!mAreaHighlight.enabled || (mOgl.highlightMask.size() == mOgl.vertexBuffer.size()));
        assert(mOgl.colorBuffer.size() == mOgl.vertexBuffer.size());

        return mOgl;
    }

    /// Update buffers used by openGl. Should only be called when they are dirty
    void updateOpenGlBuffers() {
        assert(mOgl.isDirty);  // Called unnecessarily. Most likely by error.

        const auto start = std::chrono::high_resolution_clock::now();

        generateVertexBuffer();
        generateIndexBuffer();
        generateColorBuffer();
        generateNormalBuffer();
        generateHighlightBuffer();

        mOgl.isDirty = false;
        mOgl.info.didColorUpdate = false;
        mOgl.info.didHighlightUpdate = false;

        const auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> timeMs = end - start;

        CI_LOG_I("Generating buffers took " + std::to_string(timeMs.count()) + " ms");
    }

    /// Update temporary detailed data like detailed AABB tree and detailed Mesh
    /// This is a slow operation
    void updateTemporaryDetailedData();

    bool isTemporaryDetailedDataValid() const {
        return mMeshDetailed && mTreeDetailed;
    }

    glm::vec3 getBoundingBoxMin() const {
        if(!mBoundingBox) {
            return glm::vec3(0);
        }
        return glm::vec3(mBoundingBox->xmin(), mBoundingBox->ymin(), mBoundingBox->zmin());
    }

    glm::vec3 getBoundingBoxMax() const {
        if(!mBoundingBox) {
            return glm::vec3(0);
        }
        return glm::vec3(mBoundingBox->xmax(), mBoundingBox->ymax(), mBoundingBox->zmax());
    }

    const Geometry::AreaHighlight& getAreaHighlight() const {
        return mAreaHighlight;
    }

    void hideHighlight() {
        mAreaHighlight.enabled = false;
    }

    const DataTriangle& getTriangle(const size_t triangleIndex) const {
        assert(triangleIndex < mTriangles.size());
        return mTriangles[triangleIndex];
    }

    const DataTriangle& getTriangle(const DetailedTriangleId triangleId) const {
        const size_t baseId = triangleId.getBaseId();
        const std::optional<size_t> detailId = triangleId.getDetailId();

        assert(baseId < mTriangles.size());
        if(detailId) {
            assert(!isSimpleTriangle(baseId));
            assert(*detailId < getTriangleDetailCount(baseId));
            return mTriangleDetails.at(baseId).getTriangles()[*detailId];
        } else {
            return mTriangles[baseId];
        }
    }

    size_t getTriangleColor(const size_t triangleIndex) const {
        return getTriangle(triangleIndex).getColor();
    }

    size_t getTriangleColor(const DetailedTriangleId triangleId) const {
        return getTriangle(triangleId).getColor();
    }

    /// Return the number of triangles in the whole mesh
    size_t getTriangleCount() const {
        return mTriangles.size();
    }

    const ColorManager& getColorManager() const {
        return mColorManager;
    }

    ColorManager& getColorManager() {
        return mColorManager;
    }

    bool isSimpleTriangle(size_t triangleIdx) const {
        // Triangle is single color when it has no detail triangles
        auto it = mTriangleDetails.find(triangleIdx);
        return it == mTriangleDetails.end();
    }

    const GeometryProgress& getProgress() const {
        assert(mProgress != nullptr);
        return *mProgress;
    }

    /// Loads new geometry into the private data, rebuilds the buffers and other data structures automatically.
    void loadNewGeometry(const std::string& fileName);

    /// Exports the modified geometry to the file specified by a path, file name and file type.
    void exportGeometry(const std::string filePath, const std::string fileName, const std::string fileType);

    /// Set new triangle color.
    void setTriangleColor(const size_t triangleIndex, const size_t newColor);

    /// Set new triangle color.
    void setTriangleColor(const DetailedTriangleId triangleId, const size_t newColor);

    /// Intersects the mesh with the given ray and returns the index of the triangle intersected, if it exists.
    /// Example use: generate ray based on a mouse click, call this method, then call setTriangleColor.
    std::optional<size_t> intersectMesh(const ci::Ray& ray) const;

    /// Intersects the mesh with the given ray and returns the index of the triangle intersected, if it exists.
    /// Additionally outputs intersection point to the outPos param
    /// Example use: generate ray based on a mouse click, call this method, then call setTriangleColor.
    std::optional<size_t> intersectMesh(const ci::Ray& ray, glm::vec3& outPos) const;

    /// Intersects the detailed mesh with the given ray and returns the ID of the triangle intersected, if it exists.
    std::optional<DetailedTriangleId> intersectDetailedMesh(const ci::Ray& ray);

    /// Highlight an area around the intersection point. All points on a continuous surface closer than the size are
    /// highlighted.
    void highlightArea(const ci::Ray& ray, const struct BrushSettings& settings);

    /// Paint continuous area with a brush of specified size
    void paintArea(const ci::Ray& ray, const struct BrushSettings& settings);

    /// Save current state into a struct so that it can be restored later (CommandManager target requirement)
    GeometryState saveState() const;

    /// Load previous state from a struct (CommandManager target requirement)
    void loadState(const GeometryState&);

    /// Spreads as BFS, starting from startTriangle to wherever it can reach.
    /// Stopping is handled by the StoppingCondition functor/lambda.
    /// A vector of reached triangle indices is returned;
    template <typename StoppingCondition>
    std::vector<DetailedTriangleId> bucket(const DetailedTriangleId startTriangle,
                                           const StoppingCondition& stopFunctor);

    template <typename StoppingCondition>
    std::vector<size_t> bucket(const size_t startTriangle, const StoppingCondition& stopFunctor);

    template <typename StoppingCondition>
    std::vector<size_t> bucket(const std::vector<size_t>& startTriangles, const StoppingCondition& stopFunctor);

    /// Spread as BFS from starting triangle, until the limits of brush settings are reached
    std::vector<size_t> getTrianglesUnderBrush(const glm::vec3& originPoint, const glm::vec3& insideDirection,
                                               size_t startTriangle, const struct BrushSettings& settings);

    /// Segmentation is CPU heavy because it needs to calculate a lot of data.
    /// This method allows to pre-compute the heaviest calculation.
    void computeSdfValues() {
        computeSdf();
    }

    /// Segmentation algorithms will not work if SDF values are not pre-computed
    bool isSdfComputed() const {
        if(mPolyhedronData.sdf_property_map == nullptr) {
            return false;
        } else {
            assert(mPolyhedronData.indices.size() ==
                   (mPolyhedronData.sdf_property_map.end() - mPolyhedronData.sdf_property_map.begin()));
            return mPolyhedronData.isSdfComputed;
        }
    }

    /// Once SDF is computed, segment the whole SurfaceMesh automatically
    size_t segmentation(const int numberOfClusters, const float smoothingLambda,
                        std::map<size_t, std::vector<size_t>>& segmentToTriangleIds,
                        std::unordered_map<size_t, size_t>& triangleToSegmentMap) {
        return segment(numberOfClusters, smoothingLambda, segmentToTriangleIds, triangleToSegmentMap);
    }

    double getSdfValue(const size_t triangleIndex) const {
        assert(triangleIndex < mPolyhedronData.mFaceDescs.size());
        assert(triangleIndex < mTriangles.size());
        PolyhedronData::face_descriptor faceDescForTri = mPolyhedronData.mFaceDescs[triangleIndex];
        return mPolyhedronData.sdf_property_map[faceDescForTri];
    }

    void recomputeFromData();

    /// Get number of detailed triangles for this baseId
    size_t getTriangleDetailCount(const DetailedTriangleId triangleIndex) const {
        return getTriangleDetailCount(triangleIndex.getBaseId());
    }

    /// Get number of detailed triangles for this baseId
    size_t getTriangleDetailCount(const size_t triangleIndex) const {
        auto& it = mTriangleDetails.find(triangleIndex);
        if(it == mTriangleDetails.end()) {
            return 0;
        } else {
            return it->second.getTriangles().size();
        }
    }

    const bool* sdfValuesValid() const {
        return &mPolyhedronData.sdfValuesValid;
    }

   private:
    /// Generates the vertex buffer linearly - adding each vertex of each triangle as a new one.
    /// We need to do this because each triangle has to be able to be colored differently, therefore no vertex
    /// sharing is possible.
    void generateVertexBuffer();

    /// Generating a linear index buffer, since we do not reuse any vertices.
    void generateIndexBuffer();

    /// Generating triplets of colors, since we only allow a single-colored triangle.
    void generateColorBuffer();

    /// Generate a buffer of normals. Generates only "triangle normals" - all three vertices have the same normal.
    void generateNormalBuffer();

    /// Generate a buffer of highlight information. Saves per-triangle data to each vertex
    void generateHighlightBuffer();

    /// Build the CGAL Polyhedron construct in mPolyhedronData. Takes a bit of time to rebuild.
    void buildPolyhedron();

    /// Builds AABB tree over the original mesh
    void buildTree();

    /// Build AABB Tree over all triangles including details.
    void buildDetailedTree();

    /// Build a CGAL mesh over detailed triangles
    void buildDetailedMesh();

    /// Fixes T-junctions and unmatched vertices on edges of TriangleDetails
    /// by creating a matching vertex on the neighbouring triangle
    void correctSharedVertices();

    /// Invalidate temporary detailed data like detailed AABB tree and mesh.
    void invalidateTemporaryDetailedData();

    void updateTriangleDetail(size_t triangleIdx, const glm::vec3& brushOrigin, const struct BrushSettings& settings);
    void removeTriangleDetail(size_t triangleIndex);

    TriangleDetail* createTriangleDetail(size_t triangleIdx);

    TriangleDetail* getTriangleDetail(const size_t triangleIndex) {
        auto& it = mTriangleDetails.find(triangleIndex);
        if(it == mTriangleDetails.end()) {
            return createTriangleDetail(triangleIndex);
        } else {
            return &(it->second);
        }
    }

    /// Used by BFS in bucket painting. Aggregates the neighbours of the triangle at triIndex by looking
    /// into the CGAL Polyhedron construct.
    std::array<int, 3> gatherNeighbours(const size_t triIndex) const;

    /// Used by BFS in bucket painting. Aggregates the neighbours of the triangle at triIndex by looking
    /// into the CGAL Polyhedron construct.
    std::array<std::optional<DetailedTriangleId>, 3> gatherNeighbours(const DetailedTriangleId triIndex) const;

    /// Used by BFS in bucket painting. Manages the queue used to search through the graph.
    template <typename StoppingCondition>
    void addNeighboursToQueue(const size_t currentVertex, std::unordered_set<size_t>& alreadyVisited,
                              std::deque<size_t>& toVisit, const StoppingCondition& stopFunctor) const;

    /// Used by BFS in bucket painting. Manages the queue used to search through the graph.
    template <typename StoppingCondition>
    void addNeighboursToQueue(const DetailedTriangleId currentVertex,
                              std::unordered_set<DetailedTriangleId>& alreadyVisited,
                              std::deque<DetailedTriangleId>& toVisit, const StoppingCondition& stopFunctor) const;

    void computeSdf();

    size_t segment(const int numberOfClusters, const float smoothingLambda,
                   std::map<size_t, std::vector<size_t>>& segmentToTriangleIds,
                   std::unordered_map<size_t, size_t>& triangleToSegmentMap);

    /// Method to allow the Cereal library to serialize this class. Used for saving a .p3d project.
    template <class Archive>
    void save(Archive& saveArchive) const;

    /// Method to allow the Cereal library to deserialize this class. Used for loading a .p3d project.
    template <class Archive>
    void load(Archive& loadArchive);

    template <typename StoppingCondition>
    std::vector<size_t> bucketSpread(const StoppingCondition& stopFunctor, std::deque<size_t>& toVisit,
                                     std::unordered_set<size_t>& alreadyVisited);

    template <typename StoppingCondition>
    std::vector<DetailedTriangleId> bucketSpread(const StoppingCondition& stopFunctor,
                                                 std::deque<DetailedTriangleId>& toVisit,
                                                 std::unordered_set<DetailedTriangleId>& alreadyVisited);
};

template <typename StoppingCondition>
std::vector<size_t> Geometry::bucketSpread(const StoppingCondition& stopFunctor, std::deque<size_t>& toVisit,
                                           std::unordered_set<size_t>& alreadyVisited) {
    std::vector<size_t> trianglesToColor;
    assert(mPolyhedronData.indices.size() == mTriangles.size());

    while(!toVisit.empty()) {
        // Remove yourself from queue and mark visited
        const size_t currentVertex = toVisit.front();
        toVisit.pop_front();
        assert(alreadyVisited.find(currentVertex) != alreadyVisited.end());
        assert(currentVertex < mTriangles.size());
        assert(toVisit.size() < mTriangles.size());

        // Catching because of unpredictable CGAL errors
        try {
            // Manage neighbours and grow the queue
            addNeighboursToQueue(currentVertex, alreadyVisited, toVisit, stopFunctor);
        } catch(CGAL::Assertion_exception& excp) {
            CI_LOG_E("Exception caught. Returning immediately. " + excp.expression() + " " + excp.message());
            throw std::runtime_error("Bucket spread failed inside the CGAL library.");
        }

        // Add the triangle to the list
        trianglesToColor.push_back(currentVertex);
    }
    return trianglesToColor;
}

template <typename StoppingCondition>
std::vector<DetailedTriangleId> Geometry::bucketSpread(const StoppingCondition& stopFunctor,
                                                       std::deque<DetailedTriangleId>& toVisit,
                                                       std::unordered_set<DetailedTriangleId>& alreadyVisited) {
    assert(mMeshDetailed);
    std::vector<DetailedTriangleId> trianglesToColor;

    while(!toVisit.empty()) {
        // Remove yourself from queue and mark visited
        const DetailedTriangleId currentVertex = toVisit.front();
        toVisit.pop_front();
        assert(alreadyVisited.find(currentVertex) != alreadyVisited.end());
        assert(currentVertex.getBaseId() < mTriangles.size());
        assert(currentVertex.getDetailId() < getTriangleDetailCount(currentVertex));

        // Catching because of unpredictable CGAL errors
        try {
            // Manage neighbours and grow the queue
            addNeighboursToQueue(currentVertex, alreadyVisited, toVisit, stopFunctor);
        } catch(CGAL::Assertion_exception* excp) {
            CI_LOG_E("Exception caught. Returning immediately. " + excp->expression() + " " + excp->message());
            return {};
        }

        // Add the triangle to the list
        trianglesToColor.push_back(currentVertex);
    }
    return trianglesToColor;
}

template <typename StoppingCondition>
std::vector<DetailedTriangleId> Geometry::bucket(const DetailedTriangleId startTriangle,
                                                 const StoppingCondition& stopFunctor) {
    if(mPolyhedronData.mMesh.is_empty()) {
        return {};
    }

    if(!isTemporaryDetailedDataValid()) {
        updateTemporaryDetailedData();
        assert(mMeshDetailed);
    }

    std::deque<DetailedTriangleId> toVisit;
    const DetailedTriangleId startingFace = startTriangle;
    toVisit.push_back(startingFace);

    std::unordered_set<DetailedTriangleId> alreadyVisited;
    alreadyVisited.insert(startingFace);

    return bucketSpread(stopFunctor, toVisit, alreadyVisited);
}

template <typename StoppingCondition>
std::vector<size_t> Geometry::bucket(const size_t startTriangle, const StoppingCondition& stopFunctor) {
    if(mPolyhedronData.mMesh.is_empty()) {
        return {};
    }

    std::deque<size_t> toVisit;
    const size_t startingFace = startTriangle;
    toVisit.push_back(startingFace);

    std::unordered_set<size_t> alreadyVisited;
    alreadyVisited.insert(startingFace);

    return bucketSpread(stopFunctor, toVisit, alreadyVisited);
}

template <typename StoppingCondition>
std::vector<size_t> Geometry::bucket(const std::vector<size_t>& startingTriangles,
                                     const StoppingCondition& stopFunctor) {
    if(mPolyhedronData.mMesh.is_empty()) {
        return {};
    }

    std::deque<size_t> toVisit;
    std::unordered_set<size_t> alreadyVisited;

    for(const size_t startTriangle : startingTriangles) {
        toVisit.push_back(startTriangle);
        alreadyVisited.insert(startTriangle);
    }

    return bucketSpread(stopFunctor, toVisit, alreadyVisited);
}

template <typename StoppingCondition>
void Geometry::addNeighboursToQueue(const size_t currentVertex, std::unordered_set<size_t>& alreadyVisited,
                                    std::deque<size_t>& toVisit, const StoppingCondition& stopFunctor) const {
    const std::array<int, 3> neighbours = gatherNeighbours(currentVertex);
    for(int i = 0; i < 3; ++i) {
        if(neighbours[i] == -1) {
            continue;
        } else {
            if(alreadyVisited.find(neighbours[i]) == alreadyVisited.end()) {
                // New vertex -> visit it.
                if(stopFunctor(neighbours[i], currentVertex)) {
                    toVisit.push_back(neighbours[i]);
                    alreadyVisited.insert(neighbours[i]);
                }
            }
        }
    }
}

template <typename StoppingCondition>
void Geometry::addNeighboursToQueue(const DetailedTriangleId currentVertex,
                                    std::unordered_set<DetailedTriangleId>& alreadyVisited,
                                    std::deque<DetailedTriangleId>& toVisit,
                                    const StoppingCondition& stopFunctor) const {
    std::array<std::optional<DetailedTriangleId>, 3> neighbours = gatherNeighbours(currentVertex);
    for(int i = 0; i < 3; ++i) {
        if(!neighbours[i]) {
            continue;
        } else {
            if(alreadyVisited.find(*neighbours[i]) == alreadyVisited.end()) {
                // New vertex -> visit it.
                if(stopFunctor(*neighbours[i], currentVertex)) {
                    toVisit.push_back(*neighbours[i]);
                    alreadyVisited.insert(*neighbours[i]);
                }
            }
        }
    }
}

/* -------------------- Serialization -------------------- */

template <class Archive>
void Geometry::save(Archive& saveArchive) const {
    saveArchive(mColorManager);
    saveArchive(mTriangles);
    saveArchive(mTriangleDetails);
    saveArchive(mPolyhedronData.vertices);
    saveArchive(mPolyhedronData.indices);
}

template <class Archive>
void Geometry::load(Archive& loadArchive) {
    loadArchive(mColorManager);
    loadArchive(mTriangles);
    loadArchive(mTriangleDetails);
    loadArchive(mPolyhedronData.vertices);
    loadArchive(mPolyhedronData.indices);

    // Reset progress
    mProgress->resetLoad();

    // Geometry loaded via Cereal
    mProgress->importRenderPercentage = 1.0f;
    mProgress->importComputePercentage = 1.0f;

    updateTemporaryDetailedData();

    assert(!mTriangles.empty());
    assert(!mColorManager.empty());
    assert(!mPolyhedronData.vertices.empty());
    assert(!mPolyhedronData.indices.empty());
}

}  // namespace pepr3d