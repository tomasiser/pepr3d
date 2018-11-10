#pragma once

#include <iostream>
#include <vector>

#include <assimp/cimport.h>
#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure
#include <assimp/Importer.hpp>   // C++ importer interface

#include <unordered_set>
#include "geometry/ColorManager.h"
#include "geometry/Triangle.h"

#include <cinder/Log.h>
#include <boost/functional/hash.hpp>

namespace pepr3d {

class ModelImporter {
    std::string mPath;
    std::vector<aiMesh *> mMeshes;
    std::vector<DataTriangle> mTriangles;
    ColorManager mPalette;
    bool mModelLoaded = false;

   public:
    ModelImporter(std::string path) {
        this->mPath = path;
        this->mModelLoaded = loadModel(path);
    }

    std::vector<DataTriangle> getTriangles() const {
        return mTriangles;
    }

    ColorManager getColorManager() const {
        assert(!mPalette.empty());
        return mPalette;
    }

    bool isModelLoaded() {
        return mModelLoaded;
    }

   private:
    bool loadModel(const std::string &path) {
        mPalette.clear();

        /// Creates an instance of the Importer class
        Assimp::Importer importer;

        importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
        importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, aiComponent_NORMALS);

        /// Scene with some postprocessing
        const aiScene *scene =
            importer.ReadFile(path, aiProcess_FixInfacingNormals | aiProcess_Triangulate | aiProcess_SortByPType |
                                        aiProcess_GenNormals | aiProcess_RemoveComponent | aiProcess_FindDegenerates);

        // If the import failed, report it
        if(!scene) {
            CI_LOG_E(importer.GetErrorString());  // TODO: write out error message somewhere
            return false;
        }

        /// Access the file's contents
        processNode(scene->mRootNode, scene);

        mTriangles = processFirstMesh(mMeshes[0]);

        if(mPalette.empty()) {
            mPalette = ColorManager();  // create new palette with default colors
        }

        // Everything will be cleaned up by the importer destructor
        return true;
    }

    /// Processes scene tree recursively. Retrieving meshes from file.
    void processNode(aiNode *node, const aiScene *scene) {
        /// Process all the node's meshes (if any).
        for(unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            mMeshes.push_back(mesh);
        }

        /// Recursively do the same for each of its children.
        for(unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
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
            normal = calculateNormal(vertices, normals);

            /// Obtaining triangle color. Default color is set if there is no color information
            std::unordered_map<std::array<float, 3>, size_t, boost::hash<std::array<float, 3>>> colorLookup;

            glm::vec4 color;
            size_t returnColor = 0;
            if(mesh->GetNumColorChannels() > 0) {
                color.r = mesh->mColors[0][face.mIndices[0]][0];  // first color layer from first vertex of triangle
                color.g = mesh->mColors[0][face.mIndices[0]][1];
                color.b = mesh->mColors[0][face.mIndices[0]][2];
                color.a = 1;

                const std::array<float, 3> rgbArray = {color.r, color.g, color.b};
                const auto result = colorLookup.find(rgbArray);
                if(result != colorLookup.end()) {
                    assert(result->second < mPalette.size());
                    assert(result->second > 0);
                    returnColor = result->second;
                } else {
                    mPalette.addColor(color);
                    colorLookup.insert({rgbArray, mPalette.size() - 1});
                    returnColor = mPalette.size() - 1;
                    assert(colorLookup.find(rgbArray) != colorLookup.end());
                }
            }

            /// Check for degenerate triangles which we do not want in the representation
            const bool zeroAreaCheck =
                vertices[0] != vertices[1] && vertices[0] != vertices[2] && vertices[1] != vertices[2];
            if(zeroAreaCheck) {
                /// Place the constructed triangle
                triangles.emplace_back(vertices[0], vertices[1], vertices[2], normal, returnColor);
            } else {
                CI_LOG_E("Imported a triangle with zero surface area. Ommiting it from geometry data.");
            }
        }
        return triangles;
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
