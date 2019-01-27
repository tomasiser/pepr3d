#pragma once

#include <assimp/scene.h>       // Output data structure
#include <assimp/Exporter.hpp>  // C++ exporter interface

#include <array>
#include <cassert>
#include <sstream>
#include <vector>

#include "geometry/AssimpProgress.h"
#include "geometry/Geometry.h"
#include "geometry/GeometryProgress.h"
#include "geometry/PolyhedronData.h"
#include "geometry/Triangle.h"

typedef size_t colorIndex;

namespace pepr3d {

class ModelExporter {
    const std::vector<DataTriangle> &mTriangles;
    const PolyhedronData &mPolyhedronData;
    bool mModelSaved = false;
    GeometryProgress *mProgress;

   public:
    enum ExportTypes { Surface, NonPoly, Poly, PolyWithSDF };

    ModelExporter(const std::vector<DataTriangle> &triangles, const PolyhedronData &polyhedronData,
                  const std::string filePath, const std::string fileName, const std::string fileType,
                  ExportTypes exportType, GeometryProgress *progress)
        : mTriangles(triangles), mPolyhedronData(polyhedronData), mProgress(progress) {
        this->mModelSaved = saveModel(filePath, fileName, fileType, exportType);
    }

   private:
   private:
    struct IndexedEdge {
        unsigned int tri;
        unsigned int id1;
        unsigned int id2;
        colorIndex color;
        bool isBoundary = false;
    };

    bool saveModel(const std::string filePath, const std::string fileName, const std::string fileType,
                   ModelExporter::ExportTypes exportType) {
        Assimp::Exporter exporter;

        if(mProgress != nullptr) {
            mProgress->createScenePercentage = 0.0f;
        }

        std::vector<std::unique_ptr<aiScene>> scenes = createScenes(exportType);

        if(mProgress != nullptr) {
            mProgress->createScenePercentage = 1.0f;
            mProgress->exportFilePercentage = 0.0f;
        }

        for(size_t i = 0; i < scenes.size(); i++) {
            std::stringstream ss;
            ss << filePath << "/" << fileName << "_" << i << "." << fileType;
            exporter.Export(scenes[i].get(), std::string(fileType) + std::string("b"), ss.str());
        }

        if(mProgress != nullptr) {
            mProgress->exportFilePercentage = 1.0f;
        }

        return true;
    }

    std::vector<std::unique_ptr<aiScene>> createScenes(ExportTypes exportType) {
        switch(exportType) {
        case Surface: return createSurfaceScenes(); break;
        case NonPoly: return createNonPolyScenes(); break;
        case Poly: return createPolyScenes(false); break;
        case PolyWithSDF: return createPolyScenes(true); break;
        default: assert(false); return createSurfaceScenes();
        }
    }

    std::vector<std::unique_ptr<aiScene>> createSurfaceScenes() {
        
        std::vector<std::unique_ptr<aiScene>> scenes;

        std::map<colorIndex, std::vector<unsigned int>> colorsWithIndices;

        for(unsigned int i = 0; i < mTriangles.size(); i++) {
            size_t color = mTriangles[i].getColor();
            colorsWithIndices[color].emplace_back(static_cast<unsigned int>(i));

            for(unsigned int j = 0; j < 3; j++) {
                std::array<float, 3> vertex = {mTriangles[i].getVertex(j).x, mTriangles[i].getVertex(j).y,
                                               mTriangles[i].getVertex(j).z};
            }
        }
        for(auto &indexOfColor : colorsWithIndices) {
            scenes.push_back(std::move(createNewSurfaceScene(indexOfColor.second)));
        }

        return scenes;
    }

    std::vector<std::unique_ptr<aiScene>> createNonPolyScenes() {
        std::vector<std::unique_ptr<aiScene>> scenes;

        std::map<colorIndex, std::vector<unsigned int>> colorsWithIndices;

        std::map<std::array<float, 3>, glm::vec3> summedVertexNormals;

        // key=edge, value=(tri_idx, vert_idx, vert_idx)
        std::map<std::array<std::array<float, 3>, 2>, IndexedEdge> edgeLookup;

        glm::vec3 maxVertexValues(std::numeric_limits<float>::min());
        glm::vec3 minVertexValues(std::numeric_limits<float>::max());

        for(unsigned int i = 0; i < mTriangles.size(); i++) {
            size_t color = mTriangles[i].getColor();
            colorsWithIndices[color].emplace_back(static_cast<unsigned int>(i));

            for(unsigned int j = 0; j < 3; j++) {
                std::array<float, 3> vertex = {mTriangles[i].getVertex(j).x, mTriangles[i].getVertex(j).y,
                                               mTriangles[i].getVertex(j).z};

                summedVertexNormals[vertex] += mTriangles[i].getNormal();

                std::array<float, 3> nextVertex = {mTriangles[i].getVertex((j + 1) % 3).x,
                                                   mTriangles[i].getVertex((j + 1) % 3).y,
                                                   mTriangles[i].getVertex((j + 1) % 3).z};

                IndexedEdge &edge = edgeLookup[{vertex, nextVertex}];
                edge.color = mTriangles[i].getColor();
                edge.tri = i;
                edge.id1 = j;
                edge.id2 = (j + 1) % 3;

                if(maxVertexValues.x < vertex[0]) {
                    maxVertexValues.x = vertex[0];
                }
                if(minVertexValues.x > vertex[0]) {
                    minVertexValues.x = vertex[0];
                }
                if(maxVertexValues.y < vertex[1]) {
                    maxVertexValues.y = vertex[1];
                }
                if(minVertexValues.y > vertex[1]) {
                    minVertexValues.y = vertex[1];
                }
                if(maxVertexValues.z < vertex[2]) {
                    maxVertexValues.z = vertex[2];
                }
                if(minVertexValues.z > vertex[2]) {
                    minVertexValues.z = vertex[2];
                }
            }
        }

        if(colorsWithIndices.size() == 1) {
            scenes.push_back(std::move(createNewSurfaceScene(colorsWithIndices.begin()->second)));
        }

        // minimal axis model size
        float modelDiameter =
            std::min(std::min((maxVertexValues.x - minVertexValues.x), (maxVertexValues.x - minVertexValues.x)),
                     (maxVertexValues.x - minVertexValues.x));

        normalizeSummedNormals(summedVertexNormals);

        computeBoundaryEdges(edgeLookup);

        for(auto &indexOfColor : colorsWithIndices) {
            auto &soloBoundary = selectBoundaryEdgesByColor(edgeLookup, indexOfColor.first);
            scenes.push_back(std::move(
                createNewNonPolyScene(indexOfColor.second, summedVertexNormals, soloBoundary, modelDiameter)));
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

    std::vector<std::unique_ptr<aiScene>> createPolyScenes(bool withSDF) {
        std::vector<std::unique_ptr<aiScene>> scenes;

        std::map<colorIndex, std::vector<unsigned int>> colorsWithIndices;

        std::map<PolyhedronData::vertex_descriptor, glm::vec3> summedVertexNormals;
        std::map<PolyhedronData::vertex_descriptor, float> vertexSDF;  // should be min or average?

        std::map<colorIndex, std::set<PolyhedronData::halfedge_descriptor>> borderEdges;

        glm::vec3 maxVertexValues(std::numeric_limits<float>::min());
        glm::vec3 minVertexValues(std::numeric_limits<float>::max());

        for(unsigned int i = 0; i < mTriangles.size(); i++) {
            size_t color = mTriangles[i].getColor();
            colorsWithIndices[color].emplace_back(static_cast<unsigned int>(i));
        }

        if(colorsWithIndices.size() == 1) {
            scenes.push_back(std::move(createNewSurfaceScene(colorsWithIndices.begin()->second)));
            return scenes;
        }

        for(PolyhedronData::vertex_descriptor vd : mPolyhedronData.mMesh.vertices()) {
            int degree = mPolyhedronData.mMesh.degree(vd);

            auto &halfedge = mPolyhedronData.mMesh.halfedge(vd);

            for(size_t i = 0; i < degree; i++) {
                auto &face = mPolyhedronData.mMesh.face(halfedge);

                if(face.is_valid()) {
                    if(withSDF) {
                        vertexSDF[vd] += (float)mPolyhedronData.sdf_property_map[face];
                    }

                    size_t triIndex = mPolyhedronData.mIdMap[face];
                    summedVertexNormals[vd] += mTriangles[triIndex].getNormal();

                    size_t faceIdx = mPolyhedronData.mIdMap[face];
                    colorIndex faceColor = mTriangles[faceIdx].getColor();

                    auto &oppositeHalfedge = mPolyhedronData.mMesh.opposite(halfedge);
                    auto &oppositeFace = mPolyhedronData.mMesh.face(oppositeHalfedge);

                    if(oppositeFace.is_valid()) {
                        size_t oppositeFaceIdx = mPolyhedronData.mIdMap[oppositeFace];
                        colorIndex oppositeFaceColor = mTriangles[oppositeFaceIdx].getColor();
                        if(faceColor != oppositeFaceColor) {
                            borderEdges[faceColor].insert(halfedge);
                        }
                    } else {
                        // halfedge is border edge (hole in the  model), so it is alsou boundary edge
                        borderEdges[faceColor].insert(halfedge);
                    }
                }

                halfedge = mPolyhedronData.mMesh.next_around_target(halfedge);
            }

            auto &polyVertex = mPolyhedronData.mMesh.point(vd);

            if(maxVertexValues.x < polyVertex.x()) {
                maxVertexValues.x = (float)polyVertex.x();
            }
            if(minVertexValues.x > polyVertex.x()) {
                minVertexValues.x = (float)polyVertex.x();
            }
            if(maxVertexValues.y < polyVertex.y()) {
                maxVertexValues.y = (float)polyVertex.y();
            }
            if(minVertexValues.y > polyVertex.y()) {
                minVertexValues.y = (float)polyVertex.y();
            }
            if(maxVertexValues.z < polyVertex.z()) {
                maxVertexValues.z = (float)polyVertex.z();
            }
            if(minVertexValues.z > polyVertex.z()) {
                minVertexValues.z = (float)polyVertex.z();
            }

            // vertex normals norzmalization
            summedVertexNormals[vd] = glm::normalize(summedVertexNormals[vd]);

            if(withSDF) {
                vertexSDF[vd] = vertexSDF[vd] / degree;  // average
            }
        }

        // minimal axis model size
        float modelDiameter =
            std::min(std::min((maxVertexValues.x - minVertexValues.x), (maxVertexValues.x - minVertexValues.x)),
                     (maxVertexValues.x - minVertexValues.x));

        for(auto &indexOfColor : colorsWithIndices) {
            scenes.push_back(std::move(createNewPolyScene(indexOfColor.second, summedVertexNormals,
                                                          borderEdges[indexOfColor.first], vertexSDF, modelDiameter)));
        }

        return scenes;
    }

    std::unique_ptr<aiScene> createNewSurfaceScene(std::vector<unsigned int> &triangleIndices) {
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
                pMesh->mVertices[3 * i + j] = aiVector3D(mTriangles[triangleIndices[i]].getVertex(j).x,
                                                         mTriangles[triangleIndices[i]].getVertex(j).y,
                                                         mTriangles[triangleIndices[i]].getVertex(j).z);

                pMesh->mNormals[3 * i + j] = aiVector3D(mTriangles[triangleIndices[i]].getNormal().x,
                                                        mTriangles[triangleIndices[i]].getNormal().y,
                                                        mTriangles[triangleIndices[i]].getNormal().z);

                face.mIndices[j] = 3 * i + j;
            }
        }
        return scene;
    }

    std::unique_ptr<aiScene> createNewNonPolyScene(std::vector<unsigned int> &triangleIndices,
                                   std::map<std::array<float, 3>, glm::vec3> &vertexNormalLookup,
                                   std::vector<IndexedEdge> &borderEdges, float modelDiameter) {
        size_t borderTriangleCount = 2 * borderEdges.size();

        float extrusionCoef = modelDiameter / 10;

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
                pMesh->mVertices[3 * i + j] = aiVector3D(mTriangles[triangleIndices[i]].getVertex(j).x,
                                                         mTriangles[triangleIndices[i]].getVertex(j).y,
                                                         mTriangles[triangleIndices[i]].getVertex(j).z);

                pMesh->mNormals[3 * i + j] = aiVector3D(mTriangles[triangleIndices[i]].getNormal().x,
                                                        mTriangles[triangleIndices[i]].getNormal().y,
                                                        mTriangles[triangleIndices[i]].getNormal().z);

                face.mIndices[j] = 3 * i + j;
            }
        }

        for(unsigned int i = 0; i < trianglesCount; i++) {
            aiFace &face = pMesh->mFaces[i + trianglesCount];
            face.mIndices = new unsigned int[3];
            face.mNumIndices = 3;

            for(unsigned int j = 0; j < face.mNumIndices; j++) {
                unsigned int jRevert = 2 - j;

                glm::vec3 &vertex = mTriangles[triangleIndices[i]].getVertex(j);

                glm::vec3 vertexNormal = extrusionCoef * vertexNormalLookup[{vertex.x, vertex.y, vertex.z}];

                pMesh->mVertices[3 * (i + trianglesCount) + j] =
                    aiVector3D(vertex.x - vertexNormal.x, vertex.y - vertexNormal.y, vertex.z - vertexNormal.z);

                pMesh->mNormals[3 * (i + trianglesCount) + j] = aiVector3D(
                    -mTriangles[triangleIndices[i]].getNormal().x, -mTriangles[triangleIndices[i]].getNormal().y,
                    -mTriangles[triangleIndices[i]].getNormal().z);

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

            glm::vec3 vertex1 = mTriangles[borderEdges[i].tri].getVertex(borderEdges[i].id1);
            glm::vec3 vertex2 = mTriangles[borderEdges[i].tri].getVertex(borderEdges[i].id2);

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

    std::unique_ptr<aiScene> createNewPolyScene(std::vector<unsigned int> &triangleIndices,
                                std::map<PolyhedronData::vertex_descriptor, glm::vec3> &vertexNormals,
                                std::set<PolyhedronData::halfedge_descriptor> &borderEdges,
                                std::map<PolyhedronData::vertex_descriptor, float> &vertexSDF, float modelDiameter) {
        size_t borderTriangleCount = 2 * borderEdges.size();

        float extrusionCoef = modelDiameter / 4.0f;

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
                pMesh->mVertices[3 * i + j] = aiVector3D(mTriangles[triangleIndices[i]].getVertex(j).x,
                                                         mTriangles[triangleIndices[i]].getVertex(j).y,
                                                         mTriangles[triangleIndices[i]].getVertex(j).z);

                pMesh->mNormals[3 * i + j] = aiVector3D(mTriangles[triangleIndices[i]].getNormal().x,
                                                        mTriangles[triangleIndices[i]].getNormal().y,
                                                        mTriangles[triangleIndices[i]].getNormal().z);

                face.mIndices[j] = 3 * i + j;
            }
        }

        for(unsigned int i = 0; i < trianglesCount; i++) {
            aiFace &face = pMesh->mFaces[i + trianglesCount];
            face.mIndices = new unsigned int[3];
            face.mNumIndices = 3;

            const PolyhedronData::face_descriptor polyFace = mPolyhedronData.mFaceDescs[triangleIndices[i]];

            const auto halfedge = mPolyhedronData.mMesh.halfedge(polyFace);
            auto itHalfedge = halfedge;

            for(unsigned int j = 0; j < face.mNumIndices; j++) {
                unsigned int jRevert = 2 - j;

                auto &polyVertex = mPolyhedronData.mMesh.target(itHalfedge);

                auto &p = mPolyhedronData.mMesh.point(polyVertex);
                glm::vec3 vertex(p.x(), p.y(), p.z());
                glm::vec3 vertexNormal = extrusionCoef * vertexNormals[polyVertex];

                if(!vertexSDF.empty()) {
                    vertexNormal *= vertexSDF[polyVertex];
                }

                pMesh->mVertices[3 * (i + trianglesCount) + j] =
                    aiVector3D(vertex.x - vertexNormal.x, vertex.y - vertexNormal.y, vertex.z - vertexNormal.z);

                pMesh->mNormals[3 * (i + trianglesCount) + j] = aiVector3D(
                    -mTriangles[triangleIndices[i]].getNormal().x, -mTriangles[triangleIndices[i]].getNormal().y,
                    -mTriangles[triangleIndices[i]].getNormal().z);

                face.mIndices[jRevert] = (unsigned int)(3 * (i + trianglesCount) + j);

                itHalfedge = mPolyhedronData.mMesh.next(itHalfedge);
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

            auto &polyVertex1 = mPolyhedronData.mMesh.source(edge);
            auto &polyVertex2 = mPolyhedronData.mMesh.target(edge);

            auto &p1 = mPolyhedronData.mMesh.point(polyVertex1);
            auto &p2 = mPolyhedronData.mMesh.point(polyVertex2);

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