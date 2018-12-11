#include "geometry/Geometry.h"
#include "GeometryUtils.h"
#include "tools/Brush.h"

#include <CGAL/Algebraic_kernel_for_spheres_2_3.h>
#include <CGAL/Sphere_3.h>
#include <CGAL/Spherical_kernel_3.h>
#include <CGAL/Spherical_kernel_intersections.h>
#include <set>

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

    for(size_t idx = 0; idx < mTriangles.size(); ++idx) {
        const auto& triangle = mTriangles[idx];

        if(isTriangleSingleColor(idx)) {
            mVertexBuffer.push_back(triangle.getVertex(0));
            mVertexBuffer.push_back(triangle.getVertex(1));
            mVertexBuffer.push_back(triangle.getVertex(2));
        }
    }

    for(auto& it : mTriangleDetails) {
        const auto& detailTriangles = it.second.getTriangles();

        for(const auto& triangle : detailTriangles) {
            mVertexBuffer.push_back(triangle.getVertex(0));
            mVertexBuffer.push_back(triangle.getVertex(1));
            mVertexBuffer.push_back(triangle.getVertex(2));
        }
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

    for(size_t idx = 0; idx < mTriangles.size(); ++idx) {
        if(isTriangleSingleColor(idx)) {
            const auto& triangle = mTriangles[idx];
            const ColorIndex triColorIndex = static_cast<ColorIndex>(triangle.getColor());
            mColorBuffer.push_back(triColorIndex);
            mColorBuffer.push_back(triColorIndex);
            mColorBuffer.push_back(triColorIndex);
        }
    }

    for(auto& it : mTriangleDetails) {
        const auto& detailTriangles = it.second.getTriangles();

        for(const auto& triangle : detailTriangles) {
            const ColorIndex triColorIndex = static_cast<ColorIndex>(triangle.getColor());
            mColorBuffer.push_back(triColorIndex);
            mColorBuffer.push_back(triColorIndex);
            mColorBuffer.push_back(triColorIndex);
        }
    }

    assert(mColorBuffer.size() == mVertexBuffer.size());
}

void Geometry::generateNormalBuffer() {
    mNormalBuffer.clear();
    mNormalBuffer.reserve(mVertexBuffer.size());
    for(size_t idx = 0; idx < mTriangles.size(); ++idx) {
        const auto& triangle = mTriangles[idx];
        if(isTriangleSingleColor(idx)) {
            mNormalBuffer.push_back(triangle.getNormal());
            mNormalBuffer.push_back(triangle.getNormal());
            mNormalBuffer.push_back(triangle.getNormal());
        }
    }

    for(auto& it : mTriangleDetails) {
        const auto& detailTriangles = it.second.getTriangles();
        glm::vec3 normal = it.second.getOriginal().getNormal();

        for(const auto& triangle : detailTriangles) {
            mNormalBuffer.push_back(normal);
            mNormalBuffer.push_back(normal);
            mNormalBuffer.push_back(normal);
        }
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

std::vector<size_t> Geometry::getTrianglesUnderBrush(const glm::vec3& originPoint, const glm::vec3& insideDirection,
                                                     size_t startTriangle, const struct BrushSettings& settings) {
    const double sizeSquared = settings.size * settings.size;

    /// Stop when the trinagle has no intersection with the area highlight
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

    return bucket(startTriangle, stoppingCriterion);
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

        std::set<size_t> paintSet(trianglesToPaint.begin(), trianglesToPaint.end());
        trianglesToPaint.clear();

        mAreaHighlight.vertexMask.clear();
        mAreaHighlight.size = settings.size;
        mAreaHighlight.origin = intersectionPoint;
        mAreaHighlight.direction = ray.getDirection();
        mAreaHighlight.enabled = true;
        mAreaHighlight.dirty = true;

        // Mark all triangles with attribute assigned to vertex
        mAreaHighlight.vertexMask.reserve(mTriangles.size() * 3);
        for(size_t triangleIdx = 0; triangleIdx < mTriangles.size(); triangleIdx++) {
            // Fill 3 vertices of a triangle
            if(settings.continuous && paintSet.find(triangleIdx) == paintSet.end()) {
                mAreaHighlight.vertexMask.emplace_back(0);
                mAreaHighlight.vertexMask.emplace_back(0);
                mAreaHighlight.vertexMask.emplace_back(0);
            } else {
                mAreaHighlight.vertexMask.emplace_back(1);
                mAreaHighlight.vertexMask.emplace_back(1);
                mAreaHighlight.vertexMask.emplace_back(1);
            }
        }
        // TODO: Improvement: Try to avoid doing all this if we are highlighting the same triangle with the same
        // size
    } else {
        mAreaHighlight.enabled = false;
    }
}

void Geometry::paintArea(const ci::Ray& ray, const BrushSettings& settings) {
    glm::vec3 intersectionPoint{};
    auto intersectedTri = intersectMesh(ray, intersectionPoint);

    if(!intersectedTri) {
        return;
    }

    const auto trisInBrush = getTrianglesUnderBrush(intersectionPoint, ray.getDirection(), *intersectedTri, settings);

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
                updateTriangleDetail(triangleIdx, intersectionPoint, settings);
            }
        }
    }

    // TODO: Delay generating these buffers until when we need to render the geometry
    //       This will greatly speed up redo process
    //          And only when we changed triangle detail

    // Generate new buffers for OpenGL to see the new data
    generateVertexBuffer();
    generateIndexBuffer();
    generateColorBuffer();
    generateNormalBuffer();
}

namespace geometry_internal {
/// Helps get intersection data without potentionally generating exception code
struct TriangleDetailVisitor : public boost::static_visitor<std::optional<DataTriangle::K::Circle_3>> {
    std::optional<DataTriangle::K::Circle_3> operator()(const DataTriangle::K::Circle_3& circle) const {
        return circle;
    }

    std::optional<DataTriangle::K::Circle_3> operator()(const DataTriangle::K::Point_3) const {
        return {};
    }
};
}  // namespace geometry_internal

TriangleDetail* Geometry::createTriangleDetail(size_t triangleIdx) {
    auto result =
        mTriangleDetails.emplace(triangleIdx, TriangleDetail(getTriangle(triangleIdx), getTriangleColor(triangleIdx)));

    return &(result.first->second);
}

void Geometry::updateTriangleDetail(size_t triangleIdx, const glm::vec3& brushOrigin,
                                    const struct BrushSettings& settings) {
    using Point = DataTriangle::K::Point_3;
    using Point2D = DataTriangle::K::Point_2;
    using Sphere = DataTriangle::K::Sphere_3;
    using Plane = DataTriangle::K::Plane_3;
    using Triangle = DataTriangle::Triangle;

    const Triangle& tri = getTriangle(triangleIdx).getTri();
    const Plane triPlane = tri.supporting_plane();

    const Sphere brushShape(Point(brushOrigin.x, brushOrigin.y, brushOrigin.z), settings.size * settings.size);

    auto intersection = CGAL::intersection(brushShape, triPlane);
    assert(intersection);

    auto circleIntersection = boost::apply_visitor(geometry_internal::TriangleDetailVisitor{}, *intersection);
    if(!circleIntersection)
        return;  // Intersection is a point, consider this triangle not hit

    const Point2D circleOrigin = triPlane.to_2d(circleIntersection->center());
    // Transform the edge as a point, because this transformation does not preserve distances
    const Point2D circleEdge = triPlane.to_2d(circleIntersection->center() +
                                              triPlane.base1() * CGAL::sqrt(circleIntersection->squared_radius()));

    getTriangleDetail(triangleIdx)->addCircle(circleOrigin, circleEdge, settings.color);
}

void Geometry::setTriangleColor(const size_t triangleIndex, const size_t newColor) {
    // Change it in the buffer
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

    // If the triangle had a detail remove it
    if(!isTriangleSingleColor(triangleIndex)) {
        mTriangleDetails.erase(triangleIndex);
    }
}

void Geometry::buildPolyhedron() {
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

    return returnValue;
}

}  // namespace pepr3d
