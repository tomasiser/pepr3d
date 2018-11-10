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
    std::string path;
    const aiScene *scene;
    std::vector<aiMesh *> meshes;
    std::vector<DataTriangle> triangles;
    std::vector<ci::ColorA> palette;

   public:
    ModelImporter(std::string path) {
        this->path = path;
        loadModel(path);
    }

    std::vector<DataTriangle> getTriangles() const {
        return triangles;
    }

    std::vector<ci::ColorA> getColorPalette() const {
        assert(!palette.empty());
        return palette;
    }

   private:
    bool loadModel(const std::string &path) {
        palette.clear();

        /// Creates an instance of the Importer class
        Assimp::Importer importer;

        /// Scene with some postprocessing
        scene =
            importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

        // If the import failed, report it
        if(!scene) {
            std::cout << "fail: " << importer.GetErrorString() << std::endl;  // TODO: write out error somewhere
            return false;
        }

        /// Access the file's contents
        processNode(scene->mRootNode);

        triangles = processFirstMesh(meshes[0]);

        if(palette.empty()) {
            const ci::ColorA defaultColor = ci::ColorA::hex(0x017BDA);
            palette.push_back(defaultColor);
            palette.emplace_back(0, 1, 0, 1);
            palette.emplace_back(0, 1, 1, 1);
            palette.emplace_back(0.4, 0.4, 0.4, 1);
        }

        // Everything will be cleaned up by the importer destructor
        return true;
    }

    /// Processes scene tree recursively. Retrieving meshes from file.
    void processNode(aiNode *node) {
        /// Process all the node's meshes (if any).
        for(unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(mesh);
        }

        /// Recursively do the same for each of its children.
        for(unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i]);
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

            /// Check for degenerate triangles which we do not want in the representation
            const bool zeroAreaCheck =
                vertices[0] != vertices[1] && vertices[0] != vertices[2] && vertices[1] != vertices[2];
            assert(zeroAreaCheck);
            if(zeroAreaCheck) {
                /// Place the constructed triangle
                triangles.emplace_back(vertices[0], vertices[1], vertices[2], normal, returnColor);
            }
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
