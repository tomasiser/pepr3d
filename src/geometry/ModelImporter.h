#pragma once

#include <iostream>
#include <vector>

#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure
#include <assimp/Importer.hpp>   // C++ importer interface

#include <unordered_set>
#include "geometry/Triangle.h"

#include <boost/functional/hash.hpp>

namespace pepr3d {

class ModelImporter {
    const std::string path;

    std::vector<DataTriangle> triangles;
    std::vector<ci::ColorA> palette;

    std::vector<glm::vec3> vertBuffer;
    std::vector<std::array<size_t, 3>> indBuffer;

   public:
    ModelImporter(const std::string p) : path(p) {
        loadModel(path);
        loadModelWithJoinedVertices(path);
    }

    std::vector<DataTriangle> getTriangles() const {
        return triangles;
    }

    std::vector<ci::ColorA> getColorPalette() const {
        assert(!palette.empty());
        return palette;
    }

    std::vector<glm::vec3> getVertexBuffer() const {
        assert(!vertBuffer.empty());
        return vertBuffer;
    }

    std::vector<std::array<size_t, 3>> getIndexBuffer() const {
        assert(!indBuffer.empty());
        return indBuffer;
    }

   private:
    /// Pull the correct Vertex buffer (correct as in vertices are re-used for multiple triangles) from the mesh
    static std::vector<glm::vec3> calculateVertexBuffer(aiMesh *mesh) {
        std::vector<glm::vec3> vertices;
        vertices.reserve(mesh->mNumVertices);
        for(size_t i = 0; i < mesh->mNumVertices; i++) {
            vertices.emplace_back(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        }
        assert(vertices.size() == mesh->mNumVertices);
        return vertices;
    }

    /// Pull the correct index buffer (correct as in different than 0-N) from the mesh
    static std::vector<std::array<size_t, 3>> calculateIndexBuffer(aiMesh *mesh) {
        std::vector<std::array<size_t, 3>> indices;
        indices.reserve(mesh->mNumFaces);

        for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
            // We should only have triangles
            assert(mesh->mFaces[i].mNumIndices == 3);
            indices.push_back({mesh->mFaces[i].mIndices[0], mesh->mFaces[i].mIndices[1], mesh->mFaces[i].mIndices[2]});
        }
        assert(indices.size() == mesh->mNumFaces);
        return indices;
    }

    /// A method which loads the model with ALL vertex information apart from position removed. This is done to
    /// correctly merge all vertices and receive a closed mesh, which can't be done if more than one vertex per position
    /// exists.
    bool loadModelWithJoinedVertices(const std::string &path) {
        std::vector<aiMesh *> meshes;

        /// Creates an instance of the Importer class
        Assimp::Importer importer;
        importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_NORMALS | aiComponent_TANGENTS_AND_BITANGENTS |
                                                                aiComponent_COLORS | aiComponent_TEXCOORDS |
                                                                aiComponent_BONEWEIGHTS);

        /// Scene with some postprocessing
        const aiScene *scene = importer.ReadFile(path, aiProcess_RemoveComponent | aiProcess_JoinIdenticalVertices);

        // If the import failed, report it
        if(!scene) {
            std::cout << "fail: " << importer.GetErrorString() << std::endl;  // TODO: write out error somewhere
            return false;
        }

        /// Access the file's contents
        processNode(scene->mRootNode, scene, meshes);

        vertBuffer = calculateVertexBuffer(meshes[0]);
        indBuffer = calculateIndexBuffer(meshes[0]);

        meshes.clear();

        return true;
    }

    /// A method which loads the model we will use for rendering - with duplicated vertices for normals, colors, etc.
    bool loadModel(const std::string &path) {
        std::vector<aiMesh *> meshes;

        palette.clear();

        /// Creates an instance of the Importer class
        Assimp::Importer importer;

        /// Scene with some postprocessing
        const aiScene *scene =
            importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

        // If the import failed, report it
        if(!scene) {
            std::cout << "fail: " << importer.GetErrorString() << std::endl;  // TODO: write out error somewhere
            return false;
        }

        /// Access the file's contents
        processNode(scene->mRootNode, scene, meshes);

        triangles = processFirstMesh(meshes[0]);

        if(palette.empty()) {
            const ci::ColorA defaultColor = ci::ColorA::hex(0x017BDA);
            palette.push_back(defaultColor);
            palette.emplace_back(0, 1, 0, 1);
            palette.emplace_back(0, 1, 1, 1);
            palette.emplace_back(0.4, 0.4, 0.4, 1);
        }

        // Everything will be cleaned up by the importer destructor.
        // WARNING: Every ASSIMP POINTER will be DELETED beyond this point.
        meshes.clear();
        return true;
    }

    /// Processes scene tree recursively. Retrieving meshes from file.
    static void processNode(aiNode *node, const aiScene *scene, std::vector<aiMesh *> &meshes) {
        /// Process all the node's meshes (if any).
        for(unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(mesh);
        }

        /// Recursively do the same for each of its children.
        for(unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene, meshes);
        }
    }

    /// Obtains model information only from first of the meshes.
    std::vector<DataTriangle> processFirstMesh(aiMesh *mesh) {
        std::vector<DataTriangle> triangles;

        for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];

            assert(face.mNumIndices == 3);

            glm::vec3 vertices[3];
            glm::vec3 normals[3];
            glm::vec3 normal;

            /// Loading triangle vertices and normals (if it has them)
            for(unsigned int j = 0; j < face.mNumIndices; j++) {
                vertices[j].x = mesh->mVertices[face.mIndices[j]].x;
                vertices[j].y = mesh->mVertices[face.mIndices[j]].y;
                vertices[j].z = mesh->mVertices[face.mIndices[j]].z;

                if(mesh->HasNormals()) {
                    normals[j].x = mesh->mNormals[face.mIndices[j]].x;
                    normals[j].y = mesh->mNormals[face.mIndices[j]].y;
                    normals[j].z = mesh->mNormals[face.mIndices[j]].z;
                }
            }

            /// Calculation of surface normals from vertices and vertex normals or only from vertices.
            if(mesh->HasNormals()) {
                normal = calculateNormal(vertices, normals);
            } else {
                normal = calculateNormal(vertices);
            }

            /// Obtaining triangle color. Default color is set if there is no color information
            std::unordered_map<std::array<float, 3>, size_t, boost::hash<std::array<float, 3>>> colorLookup;

            cinder::ColorA color;
            size_t returnColor = 0;
            if(mesh->GetNumColorChannels() > 0) {
                color.r = mesh->mColors[0][face.mIndices[0]][0];  // first color layer from first vertex of triangle
                color.g = mesh->mColors[0][face.mIndices[0]][1];
                color.b = mesh->mColors[0][face.mIndices[0]][2];
                color.a = 1;

                const std::array<float, 3> rgbArray = {color.r, color.g, color.b};
                const auto result = colorLookup.find(rgbArray);
                if(result != colorLookup.end()) {
                    assert(result->second < palette.size());
                    assert(result->second > 0);
                    returnColor = result->second;
                } else {
                    palette.push_back(color);
                    colorLookup.insert({rgbArray, palette.size() - 1});
                    returnColor = palette.size() - 1;
                    assert(colorLookup.find(rgbArray) != colorLookup.end());
                }
            }

            /// Place the constructed triangle
            triangles.emplace_back(vertices[0], vertices[1], vertices[2], normal, returnColor);
        }
        return triangles;
    }

    /// Calculates triangle normal from its vertices.
    static glm::vec3 calculateNormal(glm::vec3 *v) {  // hoping for correct direction
        const glm::vec3 V1 = (v[1] - v[0]);
        const glm::vec3 V2 = (v[2] - v[0]);
        glm::vec3 faceNormal;
        faceNormal.x = (V1.y * V2.z) - (V1.z - V2.y);
        faceNormal.y = -((V2.z * V1.x) - (V2.x * V1.z));
        faceNormal.z = (V1.x - V2.y) - (V1.y - V2.x);

        faceNormal = glm::normalize(faceNormal);
        return faceNormal;
    }

    /// Calculates triangle normal from its vertices with orientation of original vertex normals.
    static glm::vec3 calculateNormal(const glm::vec3 vertices[3], const glm::vec3 normals[3]) {
        const glm::vec3 p0 = vertices[1] - vertices[0];
        const glm::vec3 p1 = vertices[2] - vertices[0];
        const glm::vec3 faceNormal = glm::cross(p0, p1);

        const glm::vec3 vertexNormal = glm::normalize(normals[0] + normals[1] + normals[2]);
        const float dot = glm::dot(faceNormal, vertexNormal);

        return (dot < 0.0f) ? -faceNormal : faceNormal;
    }

    bool sameColor() {}
};

}  // namespace pepr3d
