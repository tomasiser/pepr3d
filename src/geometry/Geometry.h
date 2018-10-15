#pragma once

// #include <CGAL/IO/Color.h>
#include <cassert>
#include <vector>

// #include "Triangle.h" // Uses CGAL

namespace pepr3d {

struct DataTriangle  // Will be changed for DataTriangle from Triangle.h after CGAL is usable.
{
    glm::vec3 vertices[3];

    cinder::ColorA color;

    void setColor(cinder::ColorA col) {
        color = col;
    }

    DataTriangle(glm::vec3 x, glm::vec3 y, glm::vec3 z, cinder::ColorA col) {
        vertices[0] = x;
        vertices[1] = y;
        vertices[2] = z;
        color = col;
    }
};

class Geometry {
    /// Triangle soup of the model mesh, containing CGAL::Triangle_3 data for AABB tree.
    std::vector<DataTriangle> mTriangles;

    /// Vertex buffer with the same data as mTriangles for OpenGL to render the mesh.
    /// Contains position and color data for each vertex.
    std::vector<glm::vec3> mVertexBuffer;

    /// Color buffer, keeping the invariant that every triangle has only one color - all three vertices have to have the
    /// same color. It is aligned with the vertex buffer and its size should be always equal to the vertex buffer.
    std::vector<cinder::ColorA> mColorBuffer;

    /// Index buffer for OpenGL frontend., specifying the same triangles as in mTriangles.
    std::vector<uint32_t> mIndexBuffer;

   public:
    /// Empty constructor rendering a triangle to debug
    Geometry() {
        const cinder::ColorA red(1, 0, 0, 1);
        const cinder::ColorA green(0, 1, 0, 1);

        mTriangles.emplace_back(glm::vec3(-1, -1, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1), red);
        mTriangles.emplace_back(glm::vec3(-1, -1, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, -1), green);

        generateVertexBuffer();
        generateIndexBuffer();
        generateColorBuffer();

        assert(mIndexBuffer.size() == mVertexBuffer.size());
        assert(mVertexBuffer.size() == 6);
    }

    /// Returns a constant iterator to the vertex buffer
    std::vector<glm::vec3>& getVertexBuffer() {
        return mVertexBuffer;
    }

    /// Returns a constant iterator to the index buffer
    std::vector<uint32_t>& getIndexBuffer() {
        return mIndexBuffer;
    }

    std::vector<cinder::ColorA>& getColorBuffer() {
        return mColorBuffer;
    }

    /// Loads new geometry into the private data, rebuilds the vertex and index buffers
    /// automatically.
    void loadNewGeometry(const std::string fileName) {
        /// Load into mTriangles
        // TODO: Loading code goes here, just load the mTriangles, everything else is automatic

        /// Generate new vertex buffer
        generateVertexBuffer();

        /// Generate new index buffer
        generateIndexBuffer();

        /// Generate new color buffer from triangle color data
        generateColorBuffer();
    }

    /// Set new triangle color
    void setTriangleColor(const size_t triangleIndex, const cinder::ColorA newColor) {
        /// Change it in the buffer
        // Color buffer has 1 ColorA for each vertex, each triangle has 3 vertices
        const size_t vertexPosition = triangleIndex * 3;

        // Change all vertices of the triangle to the same new color
        assert(vertexPosition + 2 < mColorBuffer.size());
        mColorBuffer[vertexPosition] = newColor;
        mColorBuffer[vertexPosition + 1] = newColor;
        mColorBuffer[vertexPosition + 2] = newColor;

        /// Change it in the triangle soup
        assert(triangleIndex < mTriangles.size());
        mTriangles[triangleIndex].setColor(newColor);
    }

   private:
    /// Generates the vertex buffer linearly - adding each vertex of each triangle as a new one.
    /// We need to do this because each triangle has to be able to be colored differently, therefore no vertex sharing
    /// is possible.
    void generateVertexBuffer() {
        mVertexBuffer.clear();
        mVertexBuffer.reserve(3 * mTriangles.size());

        for(const auto& mTriangle : mTriangles) {
            mVertexBuffer.push_back(mTriangle.vertices[0]);
            mVertexBuffer.push_back(mTriangle.vertices[1]);
            mVertexBuffer.push_back(mTriangle.vertices[2]);
        }
    }

    /// Generating a linear index buffer, since we do not reuse any vertices.
    void generateIndexBuffer() {
        mIndexBuffer.clear();
        mIndexBuffer.reserve(mVertexBuffer.size());

        for(uint32_t i = 0; i < mVertexBuffer.size(); ++i) {
            mIndexBuffer.push_back(i);
        }
    }

    /// Generating triplets of colors, since we only allow a single-colored triangle.
    void generateColorBuffer() {
        mColorBuffer.clear();
        mColorBuffer.reserve(mVertexBuffer.size());

        for(const auto& mTriangle : mTriangles) {
            mColorBuffer.push_back(mTriangle.color);
            mColorBuffer.push_back(mTriangle.color);
            mColorBuffer.push_back(mTriangle.color);
        }
        assert(mColorBuffer.size() == mVertexBuffer.size());
    }
};

}  // namespace pepr3d