#pragma once

#include <assimp/scene.h>       // Output data structure
#include <assimp/Exporter.hpp>  // C++ exporter interface

#include <glm/gtc/epsilon.hpp>

#include <array>
#include <cassert>
#include <sstream>
#include <vector>

#include "geometry/AssimpProgress.h"
#include "geometry/ExportType.h"
#include "geometry/Geometry.h"
#include "geometry/GeometryProgress.h"
#include "geometry/PolyhedronData.h"
#include "geometry/Triangle.h"
#include "geometry/TrianglePrimitive.h"

typedef size_t colorIndex;

namespace pepr3d {

class ModelExporter {
    const Geometry *mGeometry;

    bool mModelSaved = false;
    GeometryProgress *mProgress;
    std::vector<float> mExtrusioCoef;

   public:
    ModelExporter(const Geometry *geometry, GeometryProgress *progress) : mGeometry(geometry), mProgress(progress) {
        // this->mModelSaved = saveModel(filePath, fileName, fileType, exportType);
    }

    std::map<colorIndex, std::unique_ptr<aiScene>> createScenes(ExportType exportType) {
        switch(exportType) {
        case Surface: return createPolySurfaceScenes(); break;
        case NonPolySurface: return createNonPolySurfaceScenes(); break;
        case NonPolyExtrusion: return createNonPolyScenes(); break;
        case PolyExtrusion: return createPolyScenes(false); break;
        case PolyExtrusionWithSDF: return createPolyScenes(true); break;
        default: assert(false); return createNonPolySurfaceScenes();
        }
    }

    bool saveModel(const std::string filePath, const std::string fileName, const std::string fileType,
                   ExportType exportType) {
        Assimp::Exporter exporter;

        if(mProgress != nullptr) {
            mProgress->createScenePercentage = 0.0f;
        }

        std::map<colorIndex, std::unique_ptr<aiScene>> scenes = createScenes(exportType);

        if(mProgress != nullptr) {
            mProgress->createScenePercentage = 1.0f;
            mProgress->exportFilePercentage = 0.0f;
        }

        int sceneCounter = 0;
        for(auto &scene : scenes) {
            std::stringstream ss;
            ss << filePath << "/" << fileName << "_" << sceneCounter << "." << fileType;
            exporter.Export(scene.second.get(), std::string(fileType) + std::string("b"), ss.str());
            sceneCounter++;
        }

        if(mProgress != nullptr) {
            mProgress->exportFilePercentage = 1.0f;
        }

        return true;
    }

    void setExtrusionCoef(std::vector<float> extrusioCoef) {
        mExtrusioCoef = extrusioCoef;
    }

   private:
    struct IndexedEdge {
        unsigned int tri;
        unsigned int id1;
        unsigned int id2;
        colorIndex color;
        bool isBoundary = false;
    };

    std::map<colorIndex, std::unique_ptr<aiScene>> createNonPolySurfaceScenes() {
        std::map<colorIndex, std::unique_ptr<aiScene>> scenes;

        std::map<colorIndex, std::vector<unsigned int>> colorsWithIndices;

        for(unsigned int i = 0; i < mGeometry->getTriangleCount(); i++) {
            colorIndex color = mGeometry->getTriangle(i).getColor();
            colorsWithIndices[color].emplace_back(static_cast<unsigned int>(i));
        }

        for(auto &indexOfColor : colorsWithIndices) {
            scenes[indexOfColor.first] = std::move(createNewNonPolySurfaceScene(indexOfColor.second));
        }

        return scenes;
    }

    std::map<colorIndex, std::unique_ptr<aiScene>> createPolySurfaceScenes() {
        std::map<colorIndex, std::unique_ptr<aiScene>> scenes;

        std::map<colorIndex, std::vector<DetailedTriangleId>> colorsWithIndices;

        for(PolyhedronData::face_descriptor fd : mGeometry->getMeshDetailed()->faces()) {
            colorIndex color = mGeometry->getTriangle(mGeometry->getMeshDetailedIdMap()[fd]).getColor();
            colorsWithIndices[color].emplace_back(mGeometry->getMeshDetailedIdMap()[fd]);
        }

        for(auto &indexOfColor : colorsWithIndices) {
            scenes[indexOfColor.first] = std::move(createNewPolySurfaceScene(indexOfColor.second));
        }

        return scenes;
    }

    std::map<colorIndex, std::unique_ptr<aiScene>> createNonPolyScenes() {
        std::map<colorIndex, std::unique_ptr<aiScene>> scenes;

        std::map<colorIndex, std::vector<unsigned int>> colorsWithIndices;

        std::map<std::array<float, 3>, glm::vec3> summedVertexNormals;

        // key=edge, value=(tri_idx, vert_idx, vert_idx)
        std::map<std::array<std::array<float, 3>, 2>, IndexedEdge> edgeLookup;

        for(unsigned int i = 0; i < mGeometry->getTriangleCount(); i++) {
            colorIndex color = mGeometry->getTriangle(i).getColor();
            colorsWithIndices[color].emplace_back(static_cast<unsigned int>(i));

            for(unsigned int j = 0; j < 3; j++) {
                std::array<float, 3> vertex = {mGeometry->getTriangle(i).getVertex(j).x,
                                               mGeometry->getTriangle(i).getVertex(j).y,
                                               mGeometry->getTriangle(i).getVertex(j).z};

                summedVertexNormals[vertex] += mGeometry->getTriangle(i).getNormal();

                std::array<float, 3> nextVertex = {mGeometry->getTriangle(i).getVertex((j + 1) % 3).x,
                                                   mGeometry->getTriangle(i).getVertex((j + 1) % 3).y,
                                                   mGeometry->getTriangle(i).getVertex((j + 1) % 3).z};

                IndexedEdge &edge = edgeLookup[{vertex, nextVertex}];
                edge.color = mGeometry->getTriangle(i).getColor();
                edge.tri = i;
                edge.id1 = j;
                edge.id2 = (j + 1) % 3;
            }
        }

        if(colorsWithIndices.size() == 1) {
            scenes[colorsWithIndices.begin()->first] =
                std::move(createNewNonPolySurfaceScene(colorsWithIndices.begin()->second));
            return scenes;
        }

        normalizeSummedNormals(summedVertexNormals);

        computeBoundaryEdges(edgeLookup);

        for(auto &indexOfColor : colorsWithIndices) {
            auto &soloBoundary = selectBoundaryEdgesByColor(edgeLookup, indexOfColor.first);

            scenes[indexOfColor.first] = std::move(createNewNonPolyScene(
                indexOfColor.second, summedVertexNormals, soloBoundary, mExtrusioCoef[indexOfColor.first]));
        }

        return scenes;
    }

    // normalize summed vertex normals
    void normalizeSummedNormals(std::map<std::array<float, 3>, glm::vec3> &summedVertexNormals) {
        for(auto &vertexNormal : summedVertexNormals) {
            vertexNormal.second = glm::normalize(vertexNormal.second);
        }
    }

    // decide if the edge is between two colors
    void computeBoundaryEdges(std::map<std::array<std::array<float, 3>, 2>, IndexedEdge> &edgeLookup) {
        for(auto &edge : edgeLookup) {
            if(edgeLookup.count({edge.first[1], edge.first[0]})) {
                if(!edge.second.isBoundary && edgeLookup[{edge.first[1], edge.first[0]}].color != edge.second.color) {
                    edgeLookup[{edge.first[1], edge.first[0]}].isBoundary = true;
                    edge.second.isBoundary = true;
                }
            }
        }
    }

    std::vector<IndexedEdge> selectBoundaryEdgesByColor(
        std::map<std::array<std::array<float, 3>, 2>, IndexedEdge> &edgeLookup, colorIndex color) {
        std::vector<IndexedEdge> soloBoundary;
        for(auto &edge : edgeLookup) {
            if(edge.second.color == color && edge.second.isBoundary) {
                soloBoundary.emplace_back(edge.second);
            }
        }
        return soloBoundary;
    }

    std::map<colorIndex, std::unique_ptr<aiScene>> createPolyScenes(bool withSDF) {
        std::map<colorIndex, std::unique_ptr<aiScene>> scenes;

        std::map<colorIndex, std::vector<DetailedTriangleId>> colorsWithIndices;

        std::unordered_map<PolyhedronData::vertex_descriptor, glm::vec3> summedVertexNormals;
        std::map<PolyhedronData::vertex_descriptor, float> vertexSDF;  // should be min or average?

        std::map<colorIndex, std::set<PolyhedronData::halfedge_descriptor>> borderEdges;

        for(PolyhedronData::face_descriptor fd : mGeometry->getMeshDetailed()->faces()) {
            colorIndex color = mGeometry->getTriangle(mGeometry->getMeshDetailedIdMap()[fd]).getColor();
            colorsWithIndices[color].emplace_back(mGeometry->getMeshDetailedIdMap()[fd]);
        }

        if(colorsWithIndices.size() == 1) {
            scenes[colorsWithIndices.begin()->first] =
                std::move(createNewPolySurfaceScene(colorsWithIndices.begin()->second));
            return scenes;
        }

        for(PolyhedronData::vertex_descriptor vd : mGeometry->getMeshDetailed()->vertices()) {
            int degree = mGeometry->getMeshDetailed()->degree(vd);
            std::vector<glm::vec3> vertexNormals;

            auto &halfedge = mGeometry->getMeshDetailed()->halfedge(vd);

            for(size_t i = 0; i < degree; i++) {
                auto &face = mGeometry->getMeshDetailed()->face(halfedge);

                if(face.is_valid()) {
                    DetailedTriangleId triIndex = mGeometry->getMeshDetailedIdMap()[face];

                    if(withSDF) {
                        vertexSDF[vd] += (float)mGeometry->getSdfValue(triIndex.getBaseId());
                    }

                    vertexNormals.push_back(mGeometry->getTriangle(triIndex).getNormal());

                    colorIndex faceColor = mGeometry->getTriangle(triIndex).getColor();

                    auto &oppositeHalfedge = mGeometry->getMeshDetailed()->opposite(halfedge);
                    auto &oppositeFace = mGeometry->getMeshDetailed()->face(oppositeHalfedge);

                    if(oppositeFace.is_valid()) {
                        DetailedTriangleId oppositeFaceIdx = mGeometry->getMeshDetailedIdMap()[oppositeFace];
                        colorIndex oppositeFaceColor = mGeometry->getTriangle(oppositeFaceIdx).getColor();
                        if(faceColor != oppositeFaceColor) {
                            borderEdges[faceColor].insert(halfedge);
                        }
                    } else {
                        // halfedge is border edge (hole in the model), so it is alsou boundary edge
                        borderEdges[faceColor].insert(halfedge);
                    }
                }

                halfedge = mGeometry->getMeshDetailed()->next_around_target(halfedge);
            }

            float ep = glm::epsilon<float>();

            bool isEpsSameNormal = false;
            size_t usedNormalsCount = 0;
            for(size_t i = 0; i < vertexNormals.size(); i++) {
                for(size_t j = 0; j < i; j++) {
                    isEpsSameNormal |= glm::all(glm::epsilonEqual(vertexNormals[i], vertexNormals[j], ep));
                }
                if(!isEpsSameNormal) {
                    summedVertexNormals[vd] += vertexNormals[i];
                    usedNormalsCount++;
                }
                isEpsSameNormal = false;
            }

            summedVertexNormals[vd] = glm::normalize(summedVertexNormals[vd]);

            if(withSDF) {
                vertexSDF[vd] = vertexSDF[vd] / usedNormalsCount;  // average
            }
        }

        for(auto &indexOfColor : colorsWithIndices) {
            scenes[indexOfColor.first] =
                std::move(createNewPolyScene(indexOfColor.second, summedVertexNormals, borderEdges[indexOfColor.first],
                                             vertexSDF, mExtrusioCoef[indexOfColor.first]));
        }

        return scenes;
    }

    std::unique_ptr<aiScene> createNewNonPolySurfaceScene(std::vector<unsigned int> &triangleIndices) {
        std::unique_ptr<aiScene> scene = std::make_unique<aiScene>();

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

        size_t trianglesCount = triangleIndices.size();

        pMesh->mVertices = new aiVector3D[3 * trianglesCount];
        pMesh->mNormals = new aiVector3D[3 * trianglesCount];

        pMesh->mNumVertices = (unsigned int)(3 * trianglesCount);

        pMesh->mFaces = new aiFace[trianglesCount];
        pMesh->mNumFaces = (unsigned int)(trianglesCount);

        for(unsigned int i = 0; i < trianglesCount; i++) {
            aiFace &face = pMesh->mFaces[i];
            face.mIndices = new unsigned int[3];
            face.mNumIndices = 3;

            for(unsigned int j = 0; j < face.mNumIndices; j++) {
                pMesh->mVertices[3 * i + j] = aiVector3D(mGeometry->getTriangle(triangleIndices[i]).getVertex(j).x,
                                                         mGeometry->getTriangle(triangleIndices[i]).getVertex(j).y,
                                                         mGeometry->getTriangle(triangleIndices[i]).getVertex(j).z);

                pMesh->mNormals[3 * i + j] = aiVector3D(mGeometry->getTriangle(triangleIndices[i]).getNormal().x,
                                                        mGeometry->getTriangle(triangleIndices[i]).getNormal().y,
                                                        mGeometry->getTriangle(triangleIndices[i]).getNormal().z);

                face.mIndices[j] = 3 * i + j;
            }
        }
        return scene;
    }

    std::unique_ptr<aiScene> createNewPolySurfaceScene(std::vector<DetailedTriangleId> &triangleIndices) {
        std::unique_ptr<aiScene> scene = std::make_unique<aiScene>();

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

        size_t trianglesCount = triangleIndices.size();

        pMesh->mVertices = new aiVector3D[3 * trianglesCount];
        pMesh->mNormals = new aiVector3D[3 * trianglesCount];

        pMesh->mNumVertices = (unsigned int)(3 * trianglesCount);

        pMesh->mFaces = new aiFace[trianglesCount];
        pMesh->mNumFaces = (unsigned int)(trianglesCount);

        for(unsigned int i = 0; i < trianglesCount; i++) {
            aiFace &face = pMesh->mFaces[i];
            face.mIndices = new unsigned int[3];
            face.mNumIndices = 3;

            for(unsigned int j = 0; j < face.mNumIndices; j++) {
                pMesh->mVertices[3 * i + j] = aiVector3D(mGeometry->getTriangle(triangleIndices[i]).getVertex(j).x,
                                                         mGeometry->getTriangle(triangleIndices[i]).getVertex(j).y,
                                                         mGeometry->getTriangle(triangleIndices[i]).getVertex(j).z);

                pMesh->mNormals[3 * i + j] = aiVector3D(mGeometry->getTriangle(triangleIndices[i]).getNormal().x,
                                                        mGeometry->getTriangle(triangleIndices[i]).getNormal().y,
                                                        mGeometry->getTriangle(triangleIndices[i]).getNormal().z);

                face.mIndices[j] = 3 * i + j;
            }
        }
        return scene;
    }

    std::unique_ptr<aiScene> createNewNonPolyScene(std::vector<unsigned int> &triangleIndices,
                                                   std::map<std::array<float, 3>, glm::vec3> &vertexNormalLookup,
                                                   std::vector<IndexedEdge> &borderEdges, float userCoef) {
        size_t borderTriangleCount = 2 * borderEdges.size();

        float extrusionCoef = glm::length(mGeometry->getBoundingBoxMax() - mGeometry->getBoundingBoxMin()) * userCoef;

        std::unique_ptr<aiScene> scene = std::make_unique<aiScene>();
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

        size_t trianglesCount = triangleIndices.size();

        pMesh->mVertices = new aiVector3D[3 * (trianglesCount * 2 + borderTriangleCount)];
        pMesh->mNormals = new aiVector3D[3 * (trianglesCount * 2 + borderTriangleCount)];

        pMesh->mNumVertices = (unsigned int)(3 * (trianglesCount * 2 + borderTriangleCount));

        pMesh->mFaces = new aiFace[trianglesCount * 2 + borderTriangleCount];
        pMesh->mNumFaces = (unsigned int)(trianglesCount * 2 + borderTriangleCount);

        for(unsigned int i = 0; i < trianglesCount; i++) {
            aiFace &face = pMesh->mFaces[i];
            face.mIndices = new unsigned int[3];
            face.mNumIndices = 3;

            for(unsigned int j = 0; j < face.mNumIndices; j++) {
                pMesh->mVertices[3 * i + j] = aiVector3D(mGeometry->getTriangle(triangleIndices[i]).getVertex(j).x,
                                                         mGeometry->getTriangle(triangleIndices[i]).getVertex(j).y,
                                                         mGeometry->getTriangle(triangleIndices[i]).getVertex(j).z);

                pMesh->mNormals[3 * i + j] = aiVector3D(mGeometry->getTriangle(triangleIndices[i]).getNormal().x,
                                                        mGeometry->getTriangle(triangleIndices[i]).getNormal().y,
                                                        mGeometry->getTriangle(triangleIndices[i]).getNormal().z);

                face.mIndices[j] = 3 * i + j;
            }
        }

        for(unsigned int i = 0; i < trianglesCount; i++) {
            aiFace &face = pMesh->mFaces[i + trianglesCount];
            face.mIndices = new unsigned int[3];
            face.mNumIndices = 3;

            for(unsigned int j = 0; j < face.mNumIndices; j++) {
                unsigned int jRevert = 2 - j;

                glm::vec3 &vertex = mGeometry->getTriangle(triangleIndices[i]).getVertex(j);

                glm::vec3 vertexNormal = extrusionCoef * vertexNormalLookup[{vertex.x, vertex.y, vertex.z}];

                pMesh->mVertices[3 * (i + trianglesCount) + j] =
                    aiVector3D(vertex.x - vertexNormal.x, vertex.y - vertexNormal.y, vertex.z - vertexNormal.z);

                pMesh->mNormals[3 * (i + trianglesCount) + j] =
                    aiVector3D(-mGeometry->getTriangle(triangleIndices[i]).getNormal().x,
                               -mGeometry->getTriangle(triangleIndices[i]).getNormal().y,
                               -mGeometry->getTriangle(triangleIndices[i]).getNormal().z);

                face.mIndices[jRevert] = (unsigned int)(3 * (i + trianglesCount) + j);
            }
        }

        for(size_t i = 0; i < borderEdges.size(); i++) {
            aiFace &face1 = pMesh->mFaces[(2 * trianglesCount) + (2 * i)];
            face1.mIndices = new unsigned int[3];
            face1.mNumIndices = 3;

            aiFace &face2 = pMesh->mFaces[(2 * trianglesCount) + (2 * i + 1)];
            face2.mIndices = new unsigned int[3];
            face2.mNumIndices = 3;

            glm::vec3 vertex1 = mGeometry->getTriangle(borderEdges[i].tri).getVertex(borderEdges[i].id1);
            glm::vec3 vertex2 = mGeometry->getTriangle(borderEdges[i].tri).getVertex(borderEdges[i].id2);

            glm::vec3 vertexNormal1 = extrusionCoef * vertexNormalLookup[{vertex1.x, vertex1.y, vertex1.z}];
            glm::vec3 vertexNormal2 = extrusionCoef * vertexNormalLookup[{vertex2.x, vertex2.y, vertex2.z}];

            pMesh->mVertices[3 * 2 * (trianglesCount + i) + 0] = aiVector3D(vertex1.x, vertex1.y, vertex1.z);
            pMesh->mVertices[3 * 2 * (trianglesCount + i) + 1] =
                aiVector3D(vertex1.x - vertexNormal1.x, vertex1.y - vertexNormal1.y, vertex1.z - vertexNormal1.z);
            pMesh->mVertices[3 * 2 * (trianglesCount + i) + 2] = aiVector3D(vertex2.x, vertex2.y, vertex2.z);

            pMesh->mVertices[3 * 2 * (trianglesCount + i) + 3] = aiVector3D(vertex2.x, vertex2.y, vertex2.z);
            pMesh->mVertices[3 * 2 * (trianglesCount + i) + 4] =
                aiVector3D(vertex1.x - vertexNormal1.x, vertex1.y - vertexNormal1.y, vertex1.z - vertexNormal1.z);
            pMesh->mVertices[3 * 2 * (trianglesCount + i) + 5] =
                aiVector3D(vertex2.x - vertexNormal2.x, vertex2.y - vertexNormal2.y, vertex2.z - vertexNormal2.z);

            aiVector3D normal = calculateNormal({vertex2, vertex1, vertex1 - vertexNormal1});

            for(size_t j = 0; j < 6; j++) {
                pMesh->mNormals[3 * 2 * (trianglesCount + i) + j] = normal;
            }

            face1.mIndices[0] = (unsigned int)(3 * 2 * (trianglesCount + i) + 0);
            face1.mIndices[1] = (unsigned int)(3 * 2 * (trianglesCount + i) + 1);
            face1.mIndices[2] = (unsigned int)(3 * 2 * (trianglesCount + i) + 2);

            face2.mIndices[0] = (unsigned int)(3 * 2 * (trianglesCount + i) + 3);
            face2.mIndices[1] = (unsigned int)(3 * 2 * (trianglesCount + i) + 4);
            face2.mIndices[2] = (unsigned int)(3 * 2 * (trianglesCount + i) + 5);
        }

        return scene;
    }

    std::unique_ptr<aiScene> createNewPolyScene(
        std::vector<DetailedTriangleId> &triangleIndices,
        std::unordered_map<PolyhedronData::vertex_descriptor, glm::vec3> &vertexNormals,
        std::set<PolyhedronData::halfedge_descriptor> &borderEdges,
        std::map<PolyhedronData::vertex_descriptor, float> &vertexSDF, float userCoef) {
        size_t borderTriangleCount = 2 * borderEdges.size();

        float extrusionCoef = glm::length(mGeometry->getBoundingBoxMax() - mGeometry->getBoundingBoxMin()) * userCoef;

        bool withSDF = !vertexSDF.empty();

        float maxSdfValue = 0.0f;
        if(withSDF) {
            for(auto &v : vertexSDF) {
                if(v.second > maxSdfValue) {
                    maxSdfValue = v.second;
                }
            }
        }

        std::unique_ptr<aiScene> scene = std::make_unique<aiScene>();
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

        size_t trianglesCount = triangleIndices.size();

        pMesh->mVertices = new aiVector3D[3 * (trianglesCount * 2 + borderTriangleCount)];
        pMesh->mNormals = new aiVector3D[3 * (trianglesCount * 2 + borderTriangleCount)];

        pMesh->mNumVertices = (unsigned int)(3 * (trianglesCount * 2 + borderTriangleCount));

        pMesh->mFaces = new aiFace[trianglesCount * 2 + borderTriangleCount];
        pMesh->mNumFaces = (unsigned int)(trianglesCount * 2 + borderTriangleCount);

        for(unsigned int i = 0; i < trianglesCount; i++) {
            aiFace &face = pMesh->mFaces[i];
            face.mIndices = new unsigned int[3];
            face.mNumIndices = 3;

            for(unsigned int j = 0; j < face.mNumIndices; j++) {
                pMesh->mVertices[3 * i + j] = aiVector3D(mGeometry->getTriangle(triangleIndices[i]).getVertex(j).x,
                                                         mGeometry->getTriangle(triangleIndices[i]).getVertex(j).y,
                                                         mGeometry->getTriangle(triangleIndices[i]).getVertex(j).z);

                pMesh->mNormals[3 * i + j] = aiVector3D(mGeometry->getTriangle(triangleIndices[i]).getNormal().x,
                                                        mGeometry->getTriangle(triangleIndices[i]).getNormal().y,
                                                        mGeometry->getTriangle(triangleIndices[i]).getNormal().z);

                face.mIndices[j] = 3 * i + j;
            }
        }

        auto &detailedFaceDescs = mGeometry->getMeshDetailedFaceDescs();

        for(unsigned int i = 0; i < trianglesCount; i++) {
            aiFace &face = pMesh->mFaces[i + trianglesCount];
            face.mIndices = new unsigned int[3];
            face.mNumIndices = 3;

            const PolyhedronData::face_descriptor polyFace = detailedFaceDescs[triangleIndices[i]];

            const auto halfedge = mGeometry->getMeshDetailed()->halfedge(polyFace);
            auto itHalfedge = halfedge;

            for(unsigned int j = 0; j < face.mNumIndices; j++) {
                unsigned int jRevert = 2 - j;

                auto &polyVertex = mGeometry->getMeshDetailed()->target(itHalfedge);

                auto &p = mGeometry->getMeshDetailed()->point(polyVertex);
                glm::vec3 vertex(p.x(), p.y(), p.z());
                glm::vec3 vertexNormal = extrusionCoef * vertexNormals[polyVertex];

                if(withSDF) {
                    vertexNormal *= vertexSDF[polyVertex] / maxSdfValue;
                }

                pMesh->mVertices[3 * (i + trianglesCount) + j] =
                    aiVector3D(vertex.x - vertexNormal.x, vertex.y - vertexNormal.y, vertex.z - vertexNormal.z);

                pMesh->mNormals[3 * (i + trianglesCount) + j] =
                    aiVector3D(-mGeometry->getTriangle(triangleIndices[i]).getNormal().x,
                               -mGeometry->getTriangle(triangleIndices[i]).getNormal().y,
                               -mGeometry->getTriangle(triangleIndices[i]).getNormal().z);

                face.mIndices[jRevert] = (unsigned int)(3 * (i + trianglesCount) + j);

                itHalfedge = mGeometry->getMeshDetailed()->next(itHalfedge);
            }
            assert(halfedge == itHalfedge);
        }

        size_t i = 0;
        for(auto &edge : borderEdges) {
            aiFace &face1 = pMesh->mFaces[(2 * trianglesCount) + (2 * i)];
            face1.mIndices = new unsigned int[3];
            face1.mNumIndices = 3;

            aiFace &face2 = pMesh->mFaces[(2 * trianglesCount) + (2 * i + 1)];
            face2.mIndices = new unsigned int[3];
            face2.mNumIndices = 3;

            auto &polyVertex1 = mGeometry->getMeshDetailed()->source(edge);
            auto &polyVertex2 = mGeometry->getMeshDetailed()->target(edge);

            auto &p1 = mGeometry->getMeshDetailed()->point(polyVertex1);
            auto &p2 = mGeometry->getMeshDetailed()->point(polyVertex2);

            glm::vec3 vertex1(p1.x(), p1.y(), p1.z());
            glm::vec3 vertex2(p2.x(), p2.y(), p2.z());

            glm::vec3 vertexNormal1 = extrusionCoef * vertexNormals[polyVertex1];
            glm::vec3 vertexNormal2 = extrusionCoef * vertexNormals[polyVertex2];

            if(!vertexSDF.empty()) {
                vertexNormal1 *= vertexSDF[polyVertex1];
                vertexNormal2 *= vertexSDF[polyVertex2];
            }

            pMesh->mVertices[3 * 2 * (trianglesCount + i) + 0] = aiVector3D(vertex1.x, vertex1.y, vertex1.z);
            pMesh->mVertices[3 * 2 * (trianglesCount + i) + 1] =
                aiVector3D(vertex1.x - vertexNormal1.x, vertex1.y - vertexNormal1.y, vertex1.z - vertexNormal1.z);
            pMesh->mVertices[3 * 2 * (trianglesCount + i) + 2] = aiVector3D(vertex2.x, vertex2.y, vertex2.z);

            pMesh->mVertices[3 * 2 * (trianglesCount + i) + 3] = aiVector3D(vertex2.x, vertex2.y, vertex2.z);
            pMesh->mVertices[3 * 2 * (trianglesCount + i) + 4] =
                aiVector3D(vertex1.x - vertexNormal1.x, vertex1.y - vertexNormal1.y, vertex1.z - vertexNormal1.z);
            pMesh->mVertices[3 * 2 * (trianglesCount + i) + 5] =
                aiVector3D(vertex2.x - vertexNormal2.x, vertex2.y - vertexNormal2.y, vertex2.z - vertexNormal2.z);

            aiVector3D normal = calculateNormal({vertex2, vertex1, vertex1 - vertexNormal1});

            for(size_t j = 0; j < 6; j++) {
                pMesh->mNormals[3 * 2 * (trianglesCount + i) + j] = normal;
            }

            face1.mIndices[0] = (unsigned int)(3 * 2 * (trianglesCount + i) + 0);
            face1.mIndices[1] = (unsigned int)(3 * 2 * (trianglesCount + i) + 1);
            face1.mIndices[2] = (unsigned int)(3 * 2 * (trianglesCount + i) + 2);

            face2.mIndices[0] = (unsigned int)(3 * 2 * (trianglesCount + i) + 3);
            face2.mIndices[1] = (unsigned int)(3 * 2 * (trianglesCount + i) + 4);
            face2.mIndices[2] = (unsigned int)(3 * 2 * (trianglesCount + i) + 5);

            i++;
        }

        return scene;
    }

    static aiVector3D calculateNormal(const std::array<glm::vec3, 3> vertices) {
        const glm::vec3 p0 = vertices[1] - vertices[0];
        const glm::vec3 p1 = vertices[2] - vertices[0];
        const glm::vec3 faceNormal = glm::normalize(glm::cross(p0, p1));

        return aiVector3D(faceNormal.x, faceNormal.y, faceNormal.z);
    }
};

}  // namespace pepr3d