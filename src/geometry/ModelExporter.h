#pragma once

#include <array>
#include <sstream>
#include <vector>

#include <assimp/scene.h>       // Output data structure
#include <assimp/Exporter.hpp>  // C++ exporter interface

#include "geometry/Triangle.h"

namespace pepr3d {

class ModelExporter {
    std::vector<DataTriangle> mTriangles;
    bool mModelSaved = false;

   public:
    ModelExporter(const std::vector<DataTriangle> &triangles, const std::string filePath, const std::string fileName,
                  const std::string fileType) {
        this->mTriangles = triangles;
        this->mModelSaved = saveModel(filePath, fileName, fileType);
    }

   private:
    bool saveModel(const std::string filePath, const std::string fileName, const std::string fileType) {
        Assimp::Exporter exporter;

        std::vector<aiScene *> scenes = createScenes();

        for(size_t i = 0; i < scenes.size(); i++) {
            std::stringstream ss;
            ss << filePath << "/" << fileName << "_" << i << "." << fileType;
            exporter.Export(scenes[i], std::string(fileType) + std::string("b"), ss.str());

        }

        return true;
    }

    std::vector<aiScene *> createScenes() {
        std::vector<aiScene *> scenes;

        std::map<size_t, std::vector<unsigned int>> colorsWithIndices;

        for(size_t i = 0; i < mTriangles.size(); i++) {
            mTriangles[0].getColor();
            size_t color = mTriangles[i].getColor();
            colorsWithIndices[color].emplace_back(i);
        }
        for(auto &elem : colorsWithIndices) {
            scenes.emplace_back(newScene(elem.second));
        }

        return scenes;
    }

    aiScene *newScene(std::vector<unsigned int> &indices) {
        aiScene *scene = new aiScene();
        scene->mRootNode = new aiNode();

        scene->mMaterials = new aiMaterial *[1];
        scene->mMaterials[0] = nullptr;
        scene->mNumMaterials = 1;

        scene->mMaterials[0] = new aiMaterial();

        scene->mMeshes = new aiMesh *[1];
        scene->mMeshes[0] = nullptr;
        scene->mNumMeshes = 1;

        scene->mMeshes[0] = new aiMesh();
        scene->mMeshes[0]->mMaterialIndex = 0;

        scene->mRootNode->mMeshes = new unsigned int[1];
        scene->mRootNode->mMeshes[0] = 0;
        scene->mRootNode->mNumMeshes = 1;

        auto pMesh = scene->mMeshes[0];

        size_t trianglesCount = indices.size();

        pMesh->mVertices = new aiVector3D[3 * trianglesCount];
        pMesh->mNormals = new aiVector3D[3 * trianglesCount];

        pMesh->mNumVertices = 3 * (unsigned int)(trianglesCount);

        pMesh->mFaces = new aiFace[trianglesCount];
        pMesh->mNumFaces = (unsigned int)(trianglesCount);

        for(unsigned int i = 0; i < trianglesCount; i++) {
            aiFace &face = pMesh->mFaces[i];
            face.mIndices = new unsigned int[3];
            face.mNumIndices = 3;

            pMesh->mVertices[3 * i + 0] =
                aiVector3D(mTriangles[indices[i]].getVertex(0).x, mTriangles[indices[i]].getVertex(0).y,
                           mTriangles[indices[i]].getVertex(0).z);
            pMesh->mVertices[3 * i + 1] =
                aiVector3D(mTriangles[indices[i]].getVertex(1).x, mTriangles[indices[i]].getVertex(1).y,
                           mTriangles[indices[i]].getVertex(1).z);
            pMesh->mVertices[3 * i + 2] =
                aiVector3D(mTriangles[indices[i]].getVertex(2).x, mTriangles[indices[i]].getVertex(2).y,
                           mTriangles[indices[i]].getVertex(2).z);

            pMesh->mNormals[3 * i + 0] =
                aiVector3D(mTriangles[indices[i]].getNormal().x, mTriangles[indices[i]].getNormal().y,
                           mTriangles[indices[i]].getNormal().z);
            pMesh->mNormals[3 * i + 1] =
                aiVector3D(mTriangles[indices[i]].getNormal().x, mTriangles[indices[i]].getNormal().y,
                           mTriangles[indices[i]].getNormal().z);
            pMesh->mNormals[3 * i + 2] =
                aiVector3D(mTriangles[indices[i]].getNormal().x, mTriangles[indices[i]].getNormal().y,
                           mTriangles[indices[i]].getNormal().z);

            face.mIndices[0] = 3 * i + 0;
            face.mIndices[1] = 3 * i + 1;
            face.mIndices[2] = 3 * i + 2;
        }
        return scene;
    }
};

}  // namespace pepr3d