#pragma once

// #include <CGAL/IO/Color.h>
#include <cassert>
#include <vector>

// #include "Triangle.h" // Uses CGAL

// using Color = CGAL::Color;

struct DataTriangle  // Will be changed for DataTriangle from Triangle.h after CGAL
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
    std::vector<glm::vec3> getVertexBuffer() const {
        return mVertexBuffer;
    }

    /// Returns a constant iterator to the index buffer
    std::vector<uint32_t> getIndexBuffer() const {
        return mIndexBuffer;
    }

    std::vector<cinder::ColorA> getColorBuffer() const {
        return mColorBuffer;
    }

    /// Loads new geometry into the private data, rebuilds the vertex and index buffers
    /// automatically.
    void loadNewGeometry() {
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
    void generateVertexBuffer() {
        mVertexBuffer.clear();
        mVertexBuffer.reserve(3 * mTriangles.size());

        for(size_t i = 0; i < mTriangles.size(); ++i) {
            mVertexBuffer.emplace_back(mTriangles[i].vertices[0].x, mTriangles[i].vertices[0].y,
                                       mTriangles[i].vertices[0].z);
            mVertexBuffer.emplace_back(mTriangles[i].vertices[1].x, mTriangles[i].vertices[1].y,
                                       mTriangles[i].vertices[1].z);
            mVertexBuffer.emplace_back(mTriangles[i].vertices[2].x, mTriangles[i].vertices[2].y,
                                       mTriangles[i].vertices[2].z);
        }
    }

    void generateIndexBuffer() {
        mIndexBuffer.clear();
        mIndexBuffer.reserve(mVertexBuffer.size());

        for(size_t i = 0; i < mVertexBuffer.size(); ++i) {
            mIndexBuffer.push_back(i);
        }
    }

    void generateColorBuffer() {
        mColorBuffer.clear();
        mColorBuffer.reserve(mVertexBuffer.size());

        for(size_t k = 0; k < mTriangles.size(); k++) {
            mColorBuffer.push_back(mTriangles[k].color);
            mColorBuffer.push_back(mTriangles[k].color);
            mColorBuffer.push_back(mTriangles[k].color);
        }
        assert(mColorBuffer.size() == mVertexBuffer.size());
    }
};
