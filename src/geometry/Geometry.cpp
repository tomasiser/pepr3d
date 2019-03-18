#include "geometry/Geometry.h"
#include "GeometryUtils.h"
#include "tools/Brush.h"
#include "ui/MainApplication.h"

#include <CGAL/Sphere_3.h>
#include <CGAL/Spherical_kernel_3.h>
#include <functional>
#include <set>
#include <unordered_map>
#include "geometry/SdfValuesException.h"

namespace pepr3d {

/* -------------------- Commands -------------------- */

Geometry::GeometryState Geometry::saveState() const {
    // Save only necessary data to keep snapshot size low
    return GeometryState{mTriangles, mTriangleDetails, ColorManager::ColorMap(mColorManager.getColorMap())};
}

void Geometry::loadState(const GeometryState& state) {
    mTriangles = state.triangles;
    mTriangleDetails = state.triangleDetails;

    mColorManager.replaceColors(state.colorMap.begin(), state.colorMap.end());
    P_ASSERT(!mColorManager.empty());

    // TODO Improve: Can we reuse something?

    // Set opengl state to dirty so it gets updated eventually
    // Note: Updating straight away would hide this change from ModelView
    mOgl.isDirty = true;

    // Tree needs to be rebuilt to match loaded triangles
    buildTree();
    P_ASSERT(mTree->size() == mTriangles.size());

    invalidateTemporaryDetailedData();
}

/* -------------------- Mesh loading -------------------- */

void Geometry::recomputeFromData() {
    ::ThreadPool& threadPool = MainApplication::getThreadPool();
    // We already loaded the model
    P_ASSERT(mProgress->importRenderPercentage == 1.0f);
    P_ASSERT(mProgress->importComputePercentage == 1.0f);

    /// Async build the polyhedron data structure
    auto buildPolyhedronFuture = threadPool.enqueue([this]() {
        P_ASSERT(!mPolyhedronData.vertices.empty());
        P_ASSERT(!mPolyhedronData.indices.empty());
        buildPolyhedron();
    });

    /// Async build the AABB tree
    auto buildTreeFuture = threadPool.enqueue([this]() {
        mProgress->aabbTreePercentage = 0.0f;

        buildTree();
        P_ASSERT(mTree->size() == mTriangles.size());

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

    generateTriangleBounds();

    /// Wait for building the polyhedron and tree
    buildTreeFuture.get();
    buildPolyhedronFuture.get();
}

void Geometry::buildTree() {
    mTree = std::make_unique<Tree>();

    for(size_t triIdx = 0; triIdx < mTriangles.size(); triIdx++) {
        mTree->insert(DataTriangleAABBPrimitive(this, DetailedTriangleId(triIdx)));
    }

    mTree->build();
}

void Geometry::loadNewGeometry(const std::string& fileName) {
    // Reset progress
    mProgress->resetLoad();

    /// Import the object via Assimp
    ModelImporter modelImporter(fileName, mProgress.get(), MainApplication::getThreadPool());  // only first mesh [0]

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
        P_ASSERT(!mColorManager.empty());

        /// Do the computations in parallel
        recomputeFromData();
    } else {
        throw std::runtime_error("Model loading failed.");
        CI_LOG_E("Model not loaded --> write out message for user");
    }
}

void Geometry::generateVertexBuffer() {
    mOgl.vertexBuffer.clear();
    mOgl.vertexBuffer.reserve(3 * mTriangles.size());

    for(size_t idx = 0; idx < mTriangles.size(); ++idx) {
        const auto& triangle = mTriangles[idx];

        if(isSimpleTriangle(idx)) {
            mOgl.vertexBuffer.push_back(triangle.getVertex(0));
            mOgl.vertexBuffer.push_back(triangle.getVertex(1));
            mOgl.vertexBuffer.push_back(triangle.getVertex(2));
        } else {
            // Pass dummy triangle to keep triangleIdx consistent with array position
            mOgl.vertexBuffer.push_back(glm::vec3{0, 0, 0});
            mOgl.vertexBuffer.push_back(glm::vec3{0, 0, 0});
            mOgl.vertexBuffer.push_back(glm::vec3{0, 0, 0});
        }
    }

    for(auto& it : mTriangleDetails) {
        const auto& detailTriangles = it.second.getTriangles();

        for(const auto& triangle : detailTriangles) {
            mOgl.vertexBuffer.push_back(triangle.getVertex(0));
            mOgl.vertexBuffer.push_back(triangle.getVertex(1));
            mOgl.vertexBuffer.push_back(triangle.getVertex(2));
        }
    }
}

void Geometry::generateIndexBuffer() {
    mOgl.indexBuffer.clear();
    mOgl.indexBuffer.reserve(mOgl.vertexBuffer.size());

    for(uint32_t i = 0; i < mOgl.vertexBuffer.size(); ++i) {
        mOgl.indexBuffer.push_back(i);
    }
}

void Geometry::generateColorBuffer() {
    mOgl.colorBuffer.clear();
    mOgl.colorBuffer.reserve(mOgl.vertexBuffer.size());
    mTriangleDetailColorBufferStart.clear();

    for(size_t idx = 0; idx < mTriangles.size(); ++idx) {
        const auto& triangle = mTriangles[idx];
        const ColorIndex triColorIndex = static_cast<ColorIndex>(triangle.getColor());
        mOgl.colorBuffer.push_back(triColorIndex);
        mOgl.colorBuffer.push_back(triColorIndex);
        mOgl.colorBuffer.push_back(triColorIndex);
    }

    for(auto& it : mTriangleDetails) {
        const auto& detailTriangles = it.second.getTriangles();

        mTriangleDetailColorBufferStart[it.first] = mOgl.colorBuffer.size();
        for(const auto& triangle : detailTriangles) {
            const ColorIndex triColorIndex = static_cast<ColorIndex>(triangle.getColor());
            mOgl.colorBuffer.push_back(triColorIndex);
            mOgl.colorBuffer.push_back(triColorIndex);
            mOgl.colorBuffer.push_back(triColorIndex);
        }
    }

    P_ASSERT(mOgl.colorBuffer.size() == mOgl.vertexBuffer.size());
}

void Geometry::generateNormalBuffer() {
    mOgl.normalBuffer.clear();
    mOgl.normalBuffer.reserve(mOgl.vertexBuffer.size());
    for(size_t idx = 0; idx < mTriangles.size(); ++idx) {
        const auto& triangle = mTriangles[idx];
        mOgl.normalBuffer.push_back(triangle.getNormal());
        mOgl.normalBuffer.push_back(triangle.getNormal());
        mOgl.normalBuffer.push_back(triangle.getNormal());
    }

    for(auto& it : mTriangleDetails) {
        const auto& detailTriangles = it.second.getTriangles();
        glm::vec3 normal = it.second.getOriginal().getNormal();

        for(const auto& triangle : detailTriangles) {
            mOgl.normalBuffer.push_back(normal);
            mOgl.normalBuffer.push_back(normal);
            mOgl.normalBuffer.push_back(normal);
        }
    }
    P_ASSERT(mOgl.normalBuffer.size() == mOgl.vertexBuffer.size());
}

void Geometry::generateHighlightBuffer() {
    std::set<size_t>& paintSet = mAreaHighlight.triangles;
    const BrushSettings& settings = mAreaHighlight.settings;

    mOgl.highlightMask.clear();
    // Mark all triangles with attribute assigned to vertex
    mOgl.highlightMask.reserve(mTriangles.size() * 3);
    for(size_t triangleIdx = 0; triangleIdx < mTriangles.size(); triangleIdx++) {
        // Fill 3 vertices of a triangle
        if(settings.continuous && paintSet.find(triangleIdx) == paintSet.end()) {
            mOgl.highlightMask.emplace_back(0);
            mOgl.highlightMask.emplace_back(0);
            mOgl.highlightMask.emplace_back(0);

        } else {
            mOgl.highlightMask.emplace_back(1);
            mOgl.highlightMask.emplace_back(1);
            mOgl.highlightMask.emplace_back(1);
        }
    }

    // If the original triangle has highlight enabled also enable for detail
    for(auto& it : mTriangleDetails) {
        const size_t triangleIdx = it.first;
        const auto& detailTriangles = it.second.getTriangles();

        const bool enableHighlight = !settings.continuous || paintSet.find(triangleIdx) != paintSet.end();

        for(const auto& triangle : detailTriangles) {
            if(enableHighlight) {
                mOgl.highlightMask.emplace_back(1);
                mOgl.highlightMask.emplace_back(1);
                mOgl.highlightMask.emplace_back(1);
            } else {
                mOgl.highlightMask.emplace_back(0);
                mOgl.highlightMask.emplace_back(0);
                mOgl.highlightMask.emplace_back(0);
            }
        }
    }

    P_ASSERT(mOgl.highlightMask.size() == mOgl.vertexBuffer.size());

    mOgl.info.didHighlightUpdate = true;
}

void Geometry::generateTriangleBounds() {
    mTriangleBounds.clear();
    for(const DataTriangle& dataTri : mTriangles) {
        const auto& tri = dataTri.getTri();
        mTriangleBounds.push_back(GeometryUtils::getBoundingSphere(tri));
    }
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
        const DetailedTriangleId triangleId = boost::get<DataTriangleAABBPrimitive::Id>(intersection->second).second;

        P_ASSERT(triangleId.getBaseId() < mTriangles.size());
        P_ASSERT(!triangleId.getDetailId());  // We are intersecting base mesh, there should be no detail

        return triangleId.getBaseId();
    }

    /// No intersection detected.
    return {};
}

std::optional<size_t> Geometry::intersectMesh(const ci::Ray& ray, glm::vec3& outPos) const {
    auto intersection = intersectMesh(ray);

    if(intersection) {
        // Calculate intersection point of the ray with the triangle
        const DataTriangle& tri = getTriangle(*intersection);

        auto intersectionPoint = GeometryUtils::triangleRayIntersection(tri, ray);

        if(intersectionPoint)
            outPos = *intersectionPoint;
    }

    return intersection;
}

std::optional<DetailedTriangleId> Geometry::intersectDetailedMesh(const ci::Ray& ray) {
    if(mTree->empty()) {
        return {};
    }

    if(!isTemporaryDetailedDataValid()) {
        updateTemporaryDetailedData();
        P_ASSERT(mTreeDetailed);
    }

    const glm::vec3 source = ray.getOrigin();
    const glm::vec3 direction = ray.getDirection();

    const pepr3d::Geometry::Ray rayQuery(pepr3d::DataTriangle::Point(source.x, source.y, source.z),
                                         pepr3d::Geometry::Direction(direction.x, direction.y, direction.z));

    // Find the two intersection parameters - place and triangle
    Ray_intersection intersection = mTreeDetailed->first_intersection(rayQuery);
    if(intersection) {
        // The intersected triangle
        const DetailedTriangleId triangleId = boost::get<DataTriangleAABBPrimitive::Id>(intersection->second).second;

        P_ASSERT(triangleId.getBaseId() < mTriangles.size());
        P_ASSERT(!(triangleId.getDetailId() && isSimpleTriangle(triangleId.getBaseId())));

        if(!isSimpleTriangle(triangleId.getBaseId())) {
            P_ASSERT(triangleId.getDetailId());
            P_ASSERT(triangleId.getDetailId() < getTriangleDetailCount(triangleId.getBaseId()));
        }

        return triangleId;
    }

    /// No intersection detected.
    return {};
}

std::vector<size_t> Geometry::getTrianglesUnderBrush(const glm::vec3& originPoint, const glm::vec3& insideDirection,
                                                     size_t startTriangle, const struct BrushSettings& settings) {
    const double sizeSquared = settings.size * settings.size;

    /// Stop when the triangle has no intersection with the area highlight
    auto stoppingCriterionSingleTri = [this, originPoint, sizeSquared, insideDirection, startTriangle,
                                       settings](const size_t triId) -> bool {
        // Always accept the first triangle
        if(triId == startTriangle)
            return true;

        const auto& tri = getTriangle(triId);
        const auto a = tri.getVertex(0);
        const auto b = tri.getVertex(1);
        const auto c = tri.getVertex(2);

        if(!settings.paintBackfaces && glm::dot(tri.getNormal(), insideDirection) > 0.f)
            return false;  // stop on triangles facing away from the ray

        // If triangle's bounding sphere is out of range no need to test further
        if(!isTriangleInRadius(Point3(originPoint.x, originPoint.y, originPoint.z), settings.size, triId)) {
            return false;
        }

        // If any side has intersection with the brush keep the triangle
        if(GeometryUtils::segmentPointDistanceSquared(a, b, originPoint) < sizeSquared)
            return true;
        if(GeometryUtils::segmentPointDistanceSquared(b, c, originPoint) < sizeSquared)
            return true;
        if(GeometryUtils::segmentPointDistanceSquared(c, a, originPoint) < sizeSquared)
            return true;

        return false;
    };

    const auto stoppingCriterion = [&stoppingCriterionSingleTri, this](const size_t a, const size_t b) -> bool {
        return stoppingCriterionSingleTri(a) && stoppingCriterionSingleTri(b);
    };

    if(settings.continuous) {
        return bucket(startTriangle, stoppingCriterion);
    } else {
        std::vector<size_t> trianglesInRadius =
            getTrianglesInRadius(Point3(originPoint.x, originPoint.y, originPoint.z), settings.size);

        std::vector<size_t> result;
        std::copy_if(trianglesInRadius.begin(), trianglesInRadius.end(), std::back_inserter(result),
                     stoppingCriterionSingleTri);

        return result;
    }
}

void Geometry::highlightArea(const ci::Ray& ray, const BrushSettings& settings) {
    const glm::vec3 source = ray.getOrigin();
    const glm::vec3 rayDirection = ray.getDirection();

    glm::vec3 intersectionPoint{};
    auto intersectedTri = intersectMesh(ray, intersectionPoint);
    if(intersectedTri) {
        std::vector<size_t> trianglesToPaint;

        if(settings.continuous) {
            trianglesToPaint = getTrianglesUnderBrush(intersectionPoint, rayDirection, *intersectedTri, settings);
        }

        mAreaHighlight.triangles = std::set<size_t>(trianglesToPaint.begin(), trianglesToPaint.end());
        trianglesToPaint.clear();
        mAreaHighlight.settings = settings;
        mAreaHighlight.size = settings.size;
        mAreaHighlight.origin = intersectionPoint;
        mAreaHighlight.direction = ray.getDirection();
        mAreaHighlight.enabled = true;
        mAreaHighlight.dirty = true;

        // Generate highlight buffer only if our openGlBuffers are valid
        // Otherwise delay until everything is generated again
        if(!mOgl.isDirty) {
            generateHighlightBuffer();
        }

        // TODO: Improvement: Try to avoid doing all this if we are highlighting the same triangle with the same
        // size
    } else {
        mAreaHighlight.enabled = false;
    }
}

void Geometry::paintWithShape(const ci::Ray& ray, const std::vector<Point3>& shape, size_t color, bool paintBackfaces) {
    const std::pair<Point3, double> shapeBounds = GeometryUtils::getBoundingSphere(shape);
    const auto rd = ray.getDirection();
    const Line3 rayLine(shapeBounds.first, Vector3(rd.x, rd.y, rd.z));

    std::vector<size_t> trianglesInCylinder = getTrianglesInRadius(rayLine, shapeBounds.second);

    // Gather all the TriangleDetails that we want to update
    std::vector<size_t> detailsToUpdate;
    for(size_t triIdx : trianglesInCylinder) {
        const auto& cgalTri = getTriangle(triIdx).getTri();

        if(glm::dot(rd, getTriangle(triIdx).getNormal()) > 0 && !paintBackfaces) {
            continue;  // Skip triangles facing away
        }

        if(isSimpleTriangle(triIdx) && getTriangleColor(triIdx) == color) {
            continue;  // Do not paint simple triangles of the same color
        }

        detailsToUpdate.emplace_back(triIdx);
        getTriangleDetail(triIdx);  // Make sure triangle detail is created
    }

    if(!detailsToUpdate.empty()) {
        invalidateTemporaryDetailedData();
    }

    // Update in parallel
    auto& threadPool = MainApplication::getThreadPool();
    threadPool.parallel_for(detailsToUpdate.begin(), detailsToUpdate.end(),
                            [this, &shape, color, &rayLine](size_t triIdx) {
                                getTriangleDetail(triIdx)->paintShape(shape, rayLine.direction().vector(), color);
                            });

    mOgl.isDirty = true;
}

void Geometry::paintWithShape(const ci::Ray& ray, const std::vector<DataTriangle::Triangle>& triangles, size_t color) {
    CI_LOG_I(std::string("Shape of ") + std::to_string(triangles.size()) + std::string(" triangles"));

    const std::pair<Point3, double> shapeBounds = GeometryUtils::getBoundingSphere(triangles);

    CI_LOG_I(std::string("Bounds size: ") + std::to_string(shapeBounds.second));

    const auto rd = ray.getDirection();
    const Line3 rayLine(shapeBounds.first, Vector3(rd.x, rd.y, rd.z));

    std::vector<size_t> trianglesInCylinder = getTrianglesInRadius(rayLine, shapeBounds.second);
    CI_LOG_I(std::string("Triangles in radius: ") + std::to_string(trianglesInCylinder.size()));

    // Gather all the TriangleDetails that we want to update
    std::vector<size_t> detailsToUpdate;
    for(size_t triIdx : trianglesInCylinder) {
        const auto& cgalTri = getTriangle(triIdx).getTri();

        if(glm::dot(rd, getTriangle(triIdx).getNormal()) >= 0) {
            continue;  // Skip triangles facing away
        }

        if(isSimpleTriangle(triIdx) && getTriangleColor(triIdx) == color) {
            continue;  // Do not paint simple triangles of the same color
        }

        detailsToUpdate.emplace_back(triIdx);
        getTriangleDetail(triIdx);  // Make sure triangle detail is created
    }
    CI_LOG_I(std::string("Triangles to paint: ") + std::to_string(detailsToUpdate.size()));
    if(!detailsToUpdate.empty()) {
        invalidateTemporaryDetailedData();
    }

    // Update in parallel
    try {
        auto& threadPool = MainApplication::getThreadPool();
        threadPool.parallel_for(
            detailsToUpdate.begin(), detailsToUpdate.end(), [this, &triangles, color, &rayLine](size_t triIdx) {
                getTriangleDetail(triIdx)->paintShape(triangles, rayLine.direction().vector(), color);
            });

    } catch(const std::exception& e) {
        CI_LOG_E(e.what());
        throw;
    }

    mOgl.isDirty = true;
}

void Geometry::paintAreaWithSphere(const ci::Ray& ray, const BrushSettings& settings) {
    glm::vec3 intersectionPoint{};
    auto intersectedTri = intersectMesh(ray, intersectionPoint);

    if(!intersectedTri) {
        return;
    }

    const auto trisInBrush = getTrianglesUnderBrush(intersectionPoint, ray.getDirection(), *intersectedTri, settings);

    std::vector<size_t> detailsToUpdate;

    for(const size_t triangleIdx : trisInBrush) {
        const auto& cgalTri = getTriangle(triangleIdx).getTri();

        if(GeometryUtils::isFullyInsideASphere(cgalTri, intersectionPoint, settings.size)) {
            // Triangles fully inside are colored whole
            setTriangleColor(triangleIdx, settings.color);
        } else {
            if(settings.respectOriginalTriangles) {
                if(settings.paintOuterRing) {
                    setTriangleColor(triangleIdx, settings.color);
                }

            } else {
                // Do not paint triangles that are already the same color
                if(!isSimpleTriangle(triangleIdx) || getTriangle(triangleIdx).getColor() != settings.color) {
                    detailsToUpdate.emplace_back(triangleIdx);
                    getTriangleDetail(triangleIdx);  // Create triangle detail so that we dont modify
                }
            }
        }
    }

    if(!detailsToUpdate.empty()) {
        invalidateTemporaryDetailedData();
    }

    const Sphere brushShape(Point3(intersectionPoint.x, intersectionPoint.y, intersectionPoint.z),
                            settings.size * settings.size);

    try {
        auto& threadPool = MainApplication::getThreadPool();
        threadPool.parallel_for(
            detailsToUpdate.begin(), detailsToUpdate.end(), [this, &brushShape, &settings](size_t triIdx) {
                getTriangleDetail(triIdx)->paintSphere(brushShape, settings.segments, settings.color);
            });
    } catch(const std::exception& e) {
        CI_LOG_E(e.what());
        throw;
    }

    mOgl.isDirty = true;
}

TriangleDetail* Geometry::createTriangleDetail(size_t triangleIdx) {
    auto result = mTriangleDetails.emplace(triangleIdx, TriangleDetail(getTriangle(triangleIdx)));

    return &(result.first->second);
}

void Geometry::removeTriangleDetail(const size_t triangleIndex) {
    mOgl.isDirty = true;
    mTriangleDetails.erase(triangleIndex);

    // Chaning triangle detail invalidates detailed tree and mesh
    invalidateTemporaryDetailedData();
}

void Geometry::setTriangleColor(const size_t triangleIndex, const size_t newColor) {
    if(isSimpleTriangle(triangleIndex)) {
        if(!mOgl.isDirty) {
            // Change it in the buffer
            // Color buffer has 1 ColorA for each vertex, each triangle has 3 vertices
            const size_t vertexPosition = triangleIndex * 3;

            // Change all vertices of the triangle to the same new color
            P_ASSERT(vertexPosition + 2 < mOgl.colorBuffer.size());

            ColorIndex newColorIndex = static_cast<ColorIndex>(newColor);
            mOgl.colorBuffer[vertexPosition] = newColorIndex;
            mOgl.colorBuffer[vertexPosition + 1] = newColorIndex;
            mOgl.colorBuffer[vertexPosition + 2] = newColorIndex;
            mOgl.info.didColorUpdate = true;
        }
    } else {
        removeTriangleDetail(triangleIndex);
    }

    /// Change it in the triangle soup
    P_ASSERT(triangleIndex < mTriangles.size());
    mTriangles[triangleIndex].setColor(newColor);
}

void Geometry::setTriangleColor(const DetailedTriangleId triangleId, const size_t newColor) {
    const size_t baseId = triangleId.getBaseId();
    P_ASSERT(baseId < mTriangles.size());

    if(triangleId.getDetailId()) {
        const size_t detailId = *triangleId.getDetailId();
        P_ASSERT(!isSimpleTriangle(baseId));
        P_ASSERT(detailId < getTriangleDetailCount(baseId));

        TriangleDetail* detail = getTriangleDetail(baseId);
        detail->setColor(detailId, newColor);

        if(!mOgl.isDirty) {
            const size_t vertexPosition =
                mTriangleDetailColorBufferStart[triangleId.getBaseId()] + 3 * (*triangleId.getDetailId());
            ColorIndex newColorIndex = static_cast<ColorIndex>(newColor);
            mOgl.colorBuffer[vertexPosition] = newColorIndex;
            mOgl.colorBuffer[vertexPosition + 1] = newColorIndex;
            mOgl.colorBuffer[vertexPosition + 2] = newColorIndex;
            mOgl.info.didColorUpdate = true;
        }
    } else {
        // No detail ID, do the baseID behavior
        setTriangleColor(triangleId.getBaseId(), newColor);
    }
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
            P_ASSERT(f != PolyhedronData::Mesh::null_face());
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
    P_ASSERT(created);

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

void Geometry::buildDetailedTree() {
    mTreeDetailed = std::make_unique<Tree>();

    for(size_t triangleIdx = 0; triangleIdx < mTriangles.size(); triangleIdx++) {
        if(isSimpleTriangle(triangleIdx)) {
            // Insert Original triangle
            mTreeDetailed->insert(DataTriangleAABBPrimitive(this, DetailedTriangleId(triangleIdx)));
        } else {
            // Insert all detail triangles for this tri
            const TriangleDetail* detail = getTriangleDetail(triangleIdx);
            P_ASSERT(detail);
            const auto& detailTriangles = detail->getTriangles();
            for(size_t detailIdx = 0; detailIdx < detailTriangles.size(); detailIdx++) {
                mTreeDetailed->insert(DataTriangleAABBPrimitive(this, DetailedTriangleId(triangleIdx, detailIdx)));
            }
        }
    }

    mTreeDetailed->build();
}

void Geometry::buildDetailedMesh() {
    if(!mPolyhedronData.valid) {
        CI_LOG_E("Attempted to build detailed mesh when basic mash is not available");
        return;
    }

    mMeshDetailed = std::make_unique<PolyhedronData::Mesh>();
    mMeshDetailedFaceDescs.clear();
    mMeshDetailedIdMap.reset();
    bool created;
    boost::tie(mMeshDetailedIdMap, created) =
        mMeshDetailed->add_property_map<PolyhedronData::face_descriptor, DetailedTriangleId>("f:idOfEachTriangle",
                                                                                             DetailedTriangleId());
    /**
     *  Yes, we are about to hash floating point values.
     *  These values come from CGAL exact kernel, so they should be bit-equal and safe to hash.
     *  There is no betters way to get indices before this, as different color parts are stored in different polygons.
     */
    auto hashFunc = [](const glm::vec3& vec) {
        return std::hash<float>{}(vec.x) ^ std::hash<float>{}(vec.y) ^ std::hash<float>{}(vec.z);
    };

    std::unordered_map<glm::vec3, PolyhedronData::vertex_descriptor, decltype(hashFunc)> ptToVertexDesc(
        3 * mTriangles.size(), hashFunc);

    P_ASSERT(created);
    std::vector<PolyhedronData::vertex_descriptor> vertDescs;
    vertDescs.reserve(mPolyhedronData.vertices.size());

    // Add original vertices
    for(const auto& vertex : mPolyhedronData.vertices) {
        PolyhedronData::vertex_descriptor v =
            mMeshDetailed->add_vertex(DataTriangle::Point(vertex.x, vertex.y, vertex.z));
        ptToVertexDesc[vertex] = v;
        vertDescs.push_back(v);
    }

    // Add original simple faces
    for(size_t i = 0; i < mPolyhedronData.indices.size(); i++) {
        const auto& tri = mPolyhedronData.indices[i];

        if(isSimpleTriangle(i)) {
            auto f = mMeshDetailed->add_face(vertDescs[tri[0]], vertDescs[tri[1]], vertDescs[tri[2]]);
            if(f == PolyhedronData::Mesh::null_face()) {
                // Adding a non-valid face, the model is wrong and we stop.
                mMeshDetailed.reset();
                return;
            } else {
                P_ASSERT(f != PolyhedronData::Mesh::null_face());
                mMeshDetailedFaceDescs[DetailedTriangleId(i)] = f;
                mMeshDetailedIdMap[f] = DetailedTriangleId(i);
            }
        }
    }

    // Add detailed faces
    for(const auto& triDetailIt : mTriangleDetails) {
        const size_t triangleId = triDetailIt.first;
        const auto& detailTriangles = triDetailIt.second.getTriangles();

        // Add detail triangles while combining common vertices
        for(size_t detailTriangleIdx = 0; detailTriangleIdx < detailTriangles.size(); detailTriangleIdx++) {
            const DataTriangle& detail = detailTriangles[detailTriangleIdx];

            P_ASSERT(!detail.getTri().is_degenerate());

            // Vertex descriptors of current detail triangle
            std::array<PolyhedronData::vertex_descriptor, 3> vertDescriptors;

            for(int i = 0; i < 3; i++) {
                // Get iterator to vertex ID for current vertex
                auto it = ptToVertexDesc.find(detail.getVertex(i));
                if(it == ptToVertexDesc.end()) {
                    auto vertexDesc = mMeshDetailed->add_vertex(detail.getTri().vertex(i));
                    P_ASSERT(vertexDesc != PolyhedronData::Mesh::null_vertex());
                    it = ptToVertexDesc.insert(std::make_pair(detail.getVertex(i), vertexDesc)).first;
                }

                vertDescriptors[i] = it->second;
            }

            auto faceDesc = mMeshDetailed->add_face(vertDescriptors);
            P_ASSERT(faceDesc != PolyhedronData::Mesh::null_face());
            if(faceDesc != PolyhedronData::Mesh::null_face()) {
                mMeshDetailedFaceDescs.insert(
                    std::make_pair(DetailedTriangleId(triangleId, detailTriangleIdx), faceDesc));
                mMeshDetailedIdMap[faceDesc] = DetailedTriangleId(triangleId, detailTriangleIdx);
            } else {
                const double sqrdArea = detail.getTri().squared_area();
                CI_LOG_E("A null face was generated in the detailed mesh. This should not happen");
                CI_LOG_E(std::to_string(sqrdArea));
                mMeshDetailed.reset();
                return;
            }
        }
    }
}

void Geometry::correctSharedVertices() {
    if(!mPolyhedronData.valid) {
        CI_LOG_E("Cannot correct shared vertices when original polyhedron is unavailable");
        return;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // We must fix every edge that connects from a TriangleDetail to other triangle

    // Array of details that will need to be converted back to triangles
    // This can be done in parallel
    std::set<size_t> detailsToTriangulate;

    const auto& mesh = mPolyhedronData.mMesh;

    for(const PolyhedronData::edge_descriptor edge : mesh.edges()) {
        const auto firstHalfEdge = mesh.halfedge(edge, 0);
        const auto secondHalfEdge = mesh.halfedge(edge, 1);

        if(firstHalfEdge == PolyhedronData::Mesh::null_halfedge() ||
           secondHalfEdge == PolyhedronData::Mesh::null_halfedge()) {
            continue;
        }

        const auto firstFace = mesh.face(firstHalfEdge);
        const auto secondFace = mesh.face(secondHalfEdge);
        if(firstFace == PolyhedronData::Mesh::null_face() || secondFace == PolyhedronData::Mesh::null_face()) {
            continue;
        }

        const size_t firstTriIdx = mPolyhedronData.mIdMap[firstFace];
        const size_t secondTriIdx = mPolyhedronData.mIdMap[secondFace];

        if(!isSimpleTriangle(firstTriIdx) || !isSimpleTriangle(secondTriIdx)) {
            std::pair<bool, bool> didAdd =
                getTriangleDetail(firstTriIdx)->correctSharedVertices(*getTriangleDetail(secondTriIdx));

            // Mark to triangulate later if any points added
            if(didAdd.first) {
                detailsToTriangulate.insert(firstTriIdx);
            }

            if(didAdd.second) {
                detailsToTriangulate.insert(secondTriIdx);
            }
        }
    }

    // Triangulate details in parallel
    std::vector<std::future<void>> tasks;
    ThreadPool& threadPool = MainApplication::getThreadPool();
    for(size_t triIdx : detailsToTriangulate) {
        tasks.emplace_back(threadPool.enqueue([](TriangleDetail* detail) { detail->updateTrianglesFromPolygons(); },
                                              getTriangleDetail(triIdx)));
    }

    std::for_each(tasks.begin(), tasks.end(), [](auto& t) { t.get(); });

    const auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> timeMs = endTime - startTime;

    mOgl.isDirty = true;
    CI_LOG_I("Correcting shared vertices took " + std::to_string(timeMs.count()) + " ms");
}

void Geometry::updateTemporaryDetailedData() {
    const auto start = std::chrono::high_resolution_clock::now();

    correctSharedVertices();
    // Important! Do this in a single thread. Epeck kernel used by TriangleDetail
    // is not thread safe even for read-only access
    buildDetailedMesh();
    buildDetailedTree();

    const auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> timeMs = end - start;
    CI_LOG_I("Updating temporary detailed data took " + std::to_string(timeMs.count()) + " ms");
}

void Geometry::invalidateTemporaryDetailedData() {
    mTreeDetailed.reset();
    mMeshDetailed.reset();
}

std::array<int, 3> Geometry::gatherNeighbours(const size_t triIndex) const {
    const auto& faceDescriptors = mPolyhedronData.mFaceDescs;
    const auto& mesh = mPolyhedronData.mMesh;
    P_ASSERT(triIndex < faceDescriptors.size());
    std::array<int, 3> returnValue = {-1, -1, -1};
    const PolyhedronData::face_descriptor face = faceDescriptors[triIndex];
    const auto edge = mesh.halfedge(face);
    auto itEdge = edge;

    for(int i = 0; i < 3; ++i) {
        const auto oppositeEdge = mesh.opposite(itEdge);
        if(oppositeEdge.is_valid() && !mesh.is_border(oppositeEdge)) {
            const PolyhedronData::Mesh::Face_index neighbourFace = mesh.face(oppositeEdge);
            const size_t neighbourFaceId = mPolyhedronData.mIdMap[neighbourFace];
            P_ASSERT(neighbourFaceId < mTriangles.size());
            returnValue[i] = static_cast<int>(neighbourFaceId);
        }

        itEdge = mesh.next(itEdge);
    }
    P_ASSERT(edge == itEdge);

    return returnValue;
}
std::array<std::optional<DetailedTriangleId>, 3> Geometry::gatherNeighbours(const DetailedTriangleId triId) const {
    P_ASSERT(mMeshDetailed);

    const auto& faceDescriptors = mMeshDetailedFaceDescs;
    const auto& mesh = *mMeshDetailed;
    P_ASSERT(faceDescriptors.find(triId) != faceDescriptors.end());
    std::array<std::optional<DetailedTriangleId>, 3> returnValue = {};
    const PolyhedronData::face_descriptor face = faceDescriptors.find(triId)->second;
    const auto edge = mesh.halfedge(face);
    auto itEdge = edge;

    for(int i = 0; i < 3; ++i) {
        const auto oppositeEdge = mesh.opposite(itEdge);
        if(oppositeEdge.is_valid() && !mesh.is_border(oppositeEdge)) {
            const PolyhedronData::Mesh::Face_index neighbourFace = mesh.face(oppositeEdge);
            const DetailedTriangleId neighbourFaceId = mMeshDetailedIdMap[neighbourFace];
            returnValue[i] = neighbourFaceId;
        }

        itEdge = mesh.next(itEdge);
    }
    P_ASSERT(edge == itEdge);

    return returnValue;
}

void Geometry::computeSdf() {
    mProgress->sdfPercentage = 0.0f;
    mPolyhedronData.isSdfComputed = false;
    mPolyhedronData.sdfValuesValid = true;
    mPolyhedronData.mMesh.remove_property_map(mPolyhedronData.sdf_property_map);
    bool created;
    boost::tie(mPolyhedronData.sdf_property_map, created) =
        mPolyhedronData.mMesh.add_property_map<PolyhedronData::face_descriptor, double>("f:sdf");
    P_ASSERT(created);

    if(created) {
        std::pair<double, double> minMaxSdf;
        try {
            minMaxSdf = CGAL::sdf_values(mPolyhedronData.mMesh, mPolyhedronData.sdf_property_map, 2.0 / 3.0 * CGAL_PI,
                                         25, true);
        } catch(...) {
            mPolyhedronData.sdfValuesValid = false;
            mProgress->resetSdf();
            throw std::runtime_error("Computation of the SDF values failed internally in CGAL.");
        }
        if(minMaxSdf.first == minMaxSdf.second) {
            mPolyhedronData.sdfValuesValid = false;
            // This happens when the object is flat and thus has no volume
            mProgress->resetSdf();
            throw SdfValuesException("The SDF computation returned a non-valid result. The values were both equal to " +
                                     std::to_string(minMaxSdf.first) + ".");
        }
        mPolyhedronData.isSdfComputed = true;
        mProgress->sdfPercentage = 1.0f;
        CI_LOG_I("SDF values computed.");
    } else {
        mPolyhedronData.isSdfComputed = false;
        mProgress->resetSdf();
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

    P_ASSERT(created);

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
            P_ASSERT(id < mTriangles.size());
            P_ASSERT(color < numberOfSegments);
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
